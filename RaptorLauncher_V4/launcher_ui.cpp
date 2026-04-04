#include <Arduino.h>
#include <vector>
#include "launcher_ui.h"
#include "display_manager.h"
#include "touch_manager.h"
#include "audio_manager.h"
#include "storage_manager.h"
#include "settings_manager.h"
#include "wifi_manager.h"
#include "types.h"

static bool gNeedsRedraw = true;
static int gLastTouchX = -1;
static int gLastTouchY = -1;

static std::vector<GameInfo> gameList;
static int currentPage = 0;
static int currentInfoGameIndex = -1;

static bool gTouchLiveActive = false;
static int gTouchLiveX = 0;
static int gTouchLiveY = 0;

static bool gWifiBusy = false;
static String gWifiStatusText = "";

enum LauncherScreen {
  SCREEN_HOME,
  SCREEN_SETTINGS,
  SCREEN_GAME_INFO,
  SCREEN_TOUCH_TEST,
  SCREEN_TOUCH_CALIB,
  SCREEN_CONFIRM_RESET
};

static LauncherScreen currentScreen = SCREEN_HOME;

// calibration par croix simplifiée avec coordonnées mappées
struct CalTarget {
  int sx;
  int sy;
};

static const int CAL_POINT_COUNT = 5;
static CalTarget calTargets[CAL_POINT_COUNT] = {
  {20, 20},
  {300, 20},
  {300, 220},
  {20, 220},
  {160, 120}
};

static int calStep = 0;
static int calTouchX[CAL_POINT_COUNT];
static int calTouchY[CAL_POINT_COUNT];
static bool calWasTouching = false;

// --------------------------------------------------
// Couleurs
// --------------------------------------------------
static const uint16_t COLOR_INFO_BOX   = 0x7D7C;
static const uint16_t COLOR_INFO_TEXT  = 0xFFFF;
static const uint16_t COLOR_PLAY_BOX   = 0x07E0;
static const uint16_t COLOR_PLAY_TEXT  = 0xFFFF;

// --------------------------------------------------
// Home
// --------------------------------------------------
static const int ICON_W = 50;
static const int ICON_H = 50;
static const int SLOT_COUNT_PER_PAGE = 10;

static const int ROW1_Y = 12;
static const int ROW2_Y = 80;
static const int ROW3_Y = 140;

static const int COL1_X = 15;
static const int COL2_X = 90;
static const int COL3_X = 165;
static const int COL4_X = 240;

static const int ROW3_COL1_X = 72;
static const int ROW3_COL2_X = 197;

static const int LABEL_OFFSET_Y_LINE1 = 52;
static const int LABEL_OFFSET_Y_LINE2 = 60;

static const int INFO_BOX_SIZE = 10;
static const int INFO_HITBOX_SIZE = 20;

static const int FOOTER_Y = 202;
static const int BTN_LEFT_X = 8;
static const int BTN_LEFT_Y = FOOTER_Y;
static const int BTN_LEFT_W = 110;
static const int BTN_LEFT_H = 34;

static const int BTN_RIGHT_X = 202;
static const int BTN_RIGHT_Y = FOOTER_Y;
static const int BTN_RIGHT_W = 110;
static const int BTN_RIGHT_H = 34;

static const int INFO_PLAY_X = 8;
static const int INFO_PLAY_Y = 202;
static const int INFO_PLAY_W = 90;
static const int INFO_PLAY_H = 34;

static const int INFO_BACK_X = 222;
static const int INFO_BACK_Y = 202;
static const int INFO_BACK_W = 90;
static const int INFO_BACK_H = 34;

// --------------------------------------------------
// Settings fixed layout
// --------------------------------------------------
static const int SET_ROW1_Y  = 22;
static const int SET_ROW2_Y  = 40;
static const int SET_ROW3_Y  = 58;
static const int SET_ROW4_Y  = 76;
static const int SET_ROW5_Y  = 94;
static const int SET_ROW6_Y  = 112;
static const int SET_ROW7_Y  = 130;
static const int SET_ROW8_Y  = 148;
static const int SET_ROW9_Y  = 166;
static const int SET_ROW10_Y = 184;

static const int SET_MINUS_X = 200;
static const int SET_VALUE_X = 242;
static const int SET_PLUS_X  = 286;
static const int SET_BTN_Y_OFFSET = -2;
static const int SET_BTN_W = 24;
static const int SET_BTN_H = 16;
static const int SET_TEXT_Y_OFFSET = 3;

static const int RESET_POPUP_X = 20;
static const int RESET_POPUP_Y = 62;
static const int RESET_POPUP_W = 280;
static const int RESET_POPUP_H = 116;
static const int RESET_CANCEL_X = 38;
static const int RESET_BTN_Y = 142;
static const int RESET_BTN_W = 110;
static const int RESET_BTN_H = 24;
static const int RESET_CONFIRM_X = 172;

// --------------------------------------------------
// Outils
// --------------------------------------------------
static int totalPages() {
  if (gameList.empty()) return 1;
  return (gameList.size() + SLOT_COUNT_PER_PAGE - 1) / SLOT_COUNT_PER_PAGE;
}

static int pageStartIndex() {
  return currentPage * SLOT_COUNT_PER_PAGE;
}

static bool pointInRect(int x, int y, int rx, int ry, int rw, int rh) {
  return (x >= rx && x < rx + rw && y >= ry && y < ry + rh);
}

static int clampValue(int v, int minV, int maxV) {
  if (v < minV) return minV;
  if (v > maxV) return maxV;
  return v;
}

static void applyBrightnessNow() {
  int pwm = map(settingsGet().brightness, 0, 100, 10, 255);
  displaySetBrightness((uint8_t)pwm);
}

static void applyTouchCalibrationFromSettings() {
  touch_x_min = settingsGet().touch_x_min;
  touch_x_max = settingsGet().touch_x_max;
  touch_y_min = settingsGet().touch_y_min;
  touch_y_max = settingsGet().touch_y_max;
}

static void saveSettingsNow() {
  settingsSave();
}

static void drawCross(int cx, int cy) {
  displayFillRect(cx - 10, cy, 21, 1, 0xFFFF);
  displayFillRect(cx, cy - 10, 1, 21, 0xFFFF);
}

static void getSlotRect(int slot, int &x, int &y, int &w, int &h) {
  w = ICON_W;
  h = ICON_H + 18;

  switch (slot) {
    case 0: x = COL1_X; y = ROW1_Y; break;
    case 1: x = COL2_X; y = ROW1_Y; break;
    case 2: x = COL3_X; y = ROW1_Y; break;
    case 3: x = COL4_X; y = ROW1_Y; break;
    case 4: x = COL1_X; y = ROW2_Y; break;
    case 5: x = COL2_X; y = ROW2_Y; break;
    case 6: x = COL3_X; y = ROW2_Y; break;
    case 7: x = COL4_X; y = ROW2_Y; break;
    case 8: x = ROW3_COL1_X; y = ROW3_Y; break;
    case 9: x = ROW3_COL2_X; y = ROW3_Y; break;
    default: x = 0; y = 0; break;
  }
}

static void getIconRectForSlot(int slot, int &x, int &y, int &w, int &h) {
  int slotX, slotY, slotW, slotH;
  getSlotRect(slot, slotX, slotY, slotW, slotH);
  x = slotX; y = slotY; w = ICON_W; h = ICON_H;
}

static void getLabelRectForSlot(int slot, int &x, int &y, int &w, int &h) {
  int slotX, slotY, slotW, slotH;
  getSlotRect(slot, slotX, slotY, slotW, slotH);
  x = slotX; y = slotY + LABEL_OFFSET_Y_LINE1 - 1; w = ICON_W; h = 18;
}

static void getInfoBoxDrawRectForSlot(int slot, int &x, int &y, int &w, int &h) {
  int iconX, iconY, iconW, iconH;
  getIconRectForSlot(slot, iconX, iconY, iconW, iconH);
  x = iconX;
  y = iconY + iconH - INFO_BOX_SIZE;
  w = INFO_BOX_SIZE;
  h = INFO_BOX_SIZE;
}

static void getInfoBoxHitRectForSlot(int slot, int &x, int &y, int &w, int &h) {
  int drawX, drawY, drawW, drawH;
  getInfoBoxDrawRectForSlot(slot, drawX, drawY, drawW, drawH);
  x = drawX - 4;
  y = drawY - 4;
  w = INFO_HITBOX_SIZE;
  h = INFO_HITBOX_SIZE;
}

static void drawFooterButton(int x, int y, const char* text) {
  displayDrawText(x, y + 8, text);
}

static void drawPlayButton(int x, int y, int w, int h, const char* text) {
  displayFillRect(x, y, w, h, COLOR_PLAY_BOX);
  displayDrawRect(x, y, w, h, COLOR_INFO_TEXT);
  displayDrawSmallTextColor(x + 28, y + 12, text, COLOR_PLAY_TEXT, COLOR_PLAY_BOX);
}

static void drawMiniButton(int x, int y, const char* text) {
  displayDrawRect(x, y, SET_BTN_W, SET_BTN_H, COLOR_INFO_TEXT);
  displayDrawSmallText(x + 8, y + 5, text);
}

static void drawSettingsRowLabel(int rowY, const char* label, const char* action = nullptr, int actionX = 220) {
  displayDrawSmallText(8, rowY + SET_TEXT_Y_OFFSET, label);
  if (action != nullptr) {
    displayDrawSmallText(actionX, rowY + SET_TEXT_Y_OFFSET, action);
  }
}

static void splitLabelTwoLines(const String& input, String& line1, String& line2) {
  const int MAX_CHARS = 10;

  String s = input;
  s.trim();

  if (s.length() <= MAX_CHARS) {
    line1 = s;
    line2 = "";
    return;
  }

  int splitPos = -1;
  for (int i = MAX_CHARS; i >= 0; --i) {
    if (i < (int)s.length() && s[i] == ' ') {
      splitPos = i;
      break;
    }
  }

  if (splitPos <= 0) {
    line1 = s.substring(0, MAX_CHARS);
    s = s.substring(MAX_CHARS);
  } else {
    line1 = s.substring(0, splitPos);
    s = s.substring(splitPos + 1);
  }

  s.trim();

  if (s.length() <= MAX_CHARS) {
    line2 = s;
    return;
  }

  int splitPos2 = -1;
  for (int i = MAX_CHARS; i >= 0; --i) {
    if (i < (int)s.length() && s[i] == ' ') {
      splitPos2 = i;
      break;
    }
  }

  if (splitPos2 <= 0) {
    line2 = s.substring(0, MAX_CHARS);
  } else {
    line2 = s.substring(0, splitPos2);
  }

  line2.trim();

  if (line2.length() > MAX_CHARS) {
    line2 = line2.substring(0, MAX_CHARS);
  }

  if (s.length() > line2.length()) {
    if (line2.length() >= MAX_CHARS) {
      line2 = line2.substring(0, MAX_CHARS - 1) + ".";
    } else {
      line2 += ".";
    }
  }
}

static void showGameTitleScreen(const GameInfo& game) {
  displayClear();

  bool titleOk = false;

  if (game.title.length() > 0 && game.titleW > 0 && game.titleH > 0) {
    String path = "/games" + game.folder + "/" + game.title;

    if (game.title.endsWith(".raw")) {
      titleOk = displayDrawRAW(path.c_str(), 0, 0, game.titleW, game.titleH);
    } else if (game.title.endsWith(".bmp")) {
      titleOk = displayDrawBMP(path.c_str(), 0, 0);
    }
  }

  if (!titleOk) {
    displayDrawCenteredText(90, game.name.c_str());
  }

  delay(3000);
  gNeedsRedraw = true;
}

static void launchGameFromIndex(int index) {
  if (index < 0 || index >= (int)gameList.size()) return;
  showGameTitleScreen(gameList[index]);
}

// --------------------------------------------------
// Draw
// --------------------------------------------------
static void drawHomeScreen() {
  displayClear();

  int pageCount = totalPages();

  if (pageCount > 1) {
    char pageBuf[32];
    snprintf(pageBuf, sizeof(pageBuf), "%d/%d", currentPage + 1, pageCount);
    displayDrawSmallText(286, 4, pageBuf);
  }

  int start = pageStartIndex();

  for (int slot = 0; slot < SLOT_COUNT_PER_PAGE; slot++) {
    int gameIndex = start + slot;
    if (gameIndex >= (int)gameList.size()) break;

    int x, y, w, h;
    getSlotRect(slot, x, y, w, h);

    bool imageOk = false;
    const GameInfo& game = gameList[gameIndex];

    if (game.icon.length() > 0 && game.iconW > 0 && game.iconH > 0) {
      String path = "/games" + game.folder + "/" + game.icon;

      if (game.icon.endsWith(".raw")) {
        imageOk = displayDrawRAW(path.c_str(), x, y, game.iconW, game.iconH);
      } else if (game.icon.endsWith(".bmp")) {
        imageOk = displayDrawBMP(path.c_str(), x, y);
      }
    }

    if (!imageOk) {
      displayDrawSmallText(x + 8, y + 20, "KO");
    }

    int qx, qy, qw, qh;
    getInfoBoxDrawRectForSlot(slot, qx, qy, qw, qh);
    displayFillRect(qx, qy, qw, qh, COLOR_INFO_BOX);
    displayDrawRect(qx, qy, qw, qh, COLOR_INFO_TEXT);
    displayDrawSmallTextColor(qx + 2, qy + 1, "?", COLOR_INFO_TEXT, COLOR_INFO_BOX);

    String line1, line2;
    splitLabelTwoLines(game.name, line1, line2);

    displayDrawSmallText(x, y + LABEL_OFFSET_Y_LINE1, line1.c_str());
    if (line2.length() > 0) {
      displayDrawSmallText(x, y + LABEL_OFFSET_Y_LINE2, line2.c_str());
    }
  }

  drawFooterButton(BTN_LEFT_X, BTN_LEFT_Y, "Parametres");

  if (pageCount > 1) {
    if (currentPage < pageCount - 1) {
      drawFooterButton(BTN_RIGHT_X, BTN_RIGHT_Y, "Page suivante");
    } else {
      drawFooterButton(BTN_RIGHT_X, BTN_RIGHT_Y, "Page 1");
    }
  }
}

static void drawSettingsScreen() {
  displayClear();

  char buf[80];

  displayDrawSmallText(8, 4, "Parametres");

  // Volume
  displayDrawRect(4, SET_ROW1_Y - 2, 312, 16, COLOR_INFO_TEXT);
  drawSettingsRowLabel(SET_ROW1_Y, "Volume");
  drawMiniButton(SET_MINUS_X, SET_ROW1_Y + SET_BTN_Y_OFFSET, "-");
  snprintf(buf, sizeof(buf), "%d", settingsGet().volume);
  displayDrawSmallText(SET_VALUE_X, SET_ROW1_Y + SET_TEXT_Y_OFFSET, buf);
  drawMiniButton(SET_PLUS_X, SET_ROW1_Y + SET_BTN_Y_OFFSET, "+");

  // Luminosite
  displayDrawRect(4, SET_ROW2_Y - 2, 312, 16, COLOR_INFO_TEXT);
  drawSettingsRowLabel(SET_ROW2_Y, "Luminosite");
  drawMiniButton(SET_MINUS_X, SET_ROW2_Y + SET_BTN_Y_OFFSET, "-");
  snprintf(buf, sizeof(buf), "%d", settingsGet().brightness);
  displayDrawSmallText(SET_VALUE_X, SET_ROW2_Y + SET_TEXT_Y_OFFSET, buf);
  drawMiniButton(SET_PLUS_X, SET_ROW2_Y + SET_BTN_Y_OFFSET, "+");

  // Test audio
  displayDrawRect(4, SET_ROW3_Y - 2, 312, 16, COLOR_INFO_TEXT);
  drawSettingsRowLabel(SET_ROW3_Y, "Test audio", "Taper");

  // Test tactile
  displayDrawRect(4, SET_ROW4_Y - 2, 312, 16, COLOR_INFO_TEXT);
  drawSettingsRowLabel(SET_ROW4_Y, "Test tactile", "Ouvrir");

  // Calibration tactile
  displayDrawRect(4, SET_ROW5_Y - 2, 312, 16, COLOR_INFO_TEXT);
  drawSettingsRowLabel(SET_ROW5_Y, "Calibration tactile", "Ouvrir");

  // Reset tactile
  displayDrawRect(4, SET_ROW6_Y - 2, 312, 16, COLOR_INFO_TEXT);
  drawSettingsRowLabel(SET_ROW6_Y, "Reset tactile", "Reset");

  // WiFi
  displayDrawRect(4, SET_ROW7_Y - 2, 312, 16, COLOR_INFO_TEXT);
  drawSettingsRowLabel(SET_ROW7_Y, "WiFi");
  if (gWifiBusy) {
    displayDrawSmallText(190, SET_ROW7_Y + SET_TEXT_Y_OFFSET, "Connexion...");
  } else {
    displayDrawSmallText(220, SET_ROW7_Y + SET_TEXT_Y_OFFSET, wifiManagerIsActive() ? "ON" : "OFF");
  }

  // SSID
  displayDrawRect(4, SET_ROW8_Y - 2, 312, 16, COLOR_INFO_TEXT);
  drawSettingsRowLabel(SET_ROW8_Y, "SSID", nullptr);
  if (settingsGet().wifi_ssid.length() > 0) {
    displayDrawSmallText(80, SET_ROW8_Y + SET_TEXT_Y_OFFSET, settingsGet().wifi_ssid.c_str());
  } else {
    displayDrawSmallText(80, SET_ROW8_Y + SET_TEXT_Y_OFFSET, "non configure");
  }

  // IP
  displayDrawRect(4, SET_ROW9_Y - 2, 312, 16, COLOR_INFO_TEXT);
  drawSettingsRowLabel(SET_ROW9_Y, "IP", nullptr);
  if (wifiManagerIsActive()) {
    displayDrawSmallText(80, SET_ROW9_Y + SET_TEXT_Y_OFFSET, wifiManagerGetIP().c_str());
  } else {
    displayDrawSmallText(80, SET_ROW9_Y + SET_TEXT_Y_OFFSET, "-");
  }

  // Reset reglages
  displayDrawRect(4, SET_ROW10_Y - 2, 312, 16, COLOR_INFO_TEXT);
  drawSettingsRowLabel(SET_ROW10_Y, "Reset reglages", "Reset");

  // Statut WiFi
  if (gWifiStatusText.length() > 0) {
    displayDrawSmallText(8, 204, gWifiStatusText.c_str());
  }

  // Bouton retour
  displayDrawRect(INFO_BACK_X, INFO_BACK_Y, INFO_BACK_W, INFO_BACK_H, COLOR_INFO_TEXT);
  drawFooterButton(INFO_BACK_X, INFO_BACK_Y, "Retour");
}

static void drawGameInfoScreen() {
  displayClear();

  if (currentInfoGameIndex < 0 || currentInfoGameIndex >= (int)gameList.size()) {
    displayDrawText(8, 8, "Infos indisponibles");
    drawFooterButton(INFO_BACK_X, INFO_BACK_Y, "Retour");
    return;
  }

  const GameInfo& game = gameList[currentInfoGameIndex];

  displayDrawText(8, 8, game.name.c_str());

  String authorLine = "Auteur: " + game.author;
  String typeLine   = "Type: " + game.type;
  String binLine    = "Bin: " + game.bin;
  String saveLine   = "Save: " + game.save;

  displayDrawSmallText(8, 40, authorLine.c_str());
  displayDrawSmallText(8, 52, typeLine.c_str());
  displayDrawSmallText(8, 64, binLine.c_str());
  displayDrawSmallText(8, 76, saveLine.c_str());

  displayDrawSmallText(8, 98, "Description:");
  displayDrawSmallText(8, 110, game.description.c_str());

  drawPlayButton(INFO_PLAY_X, INFO_PLAY_Y, INFO_PLAY_W, INFO_PLAY_H, "Play");
  drawFooterButton(INFO_BACK_X, INFO_BACK_Y, "Retour");
}

static void drawTouchTestScreen() {
  displayClear();
  displayDrawSmallText(8, 8, "Test tactile");
  displayDrawSmallText(8, 22, "Touchez / balayez");
  drawFooterButton(INFO_BACK_X, INFO_BACK_Y, "Retour");

  char buf[64];
  snprintf(buf, sizeof(buf), "MAP X:%d", gTouchLiveX);
  displayDrawSmallText(8, 50, buf);
  snprintf(buf, sizeof(buf), "MAP Y:%d", gTouchLiveY);
  displayDrawSmallText(8, 62, buf);

  if (gTouchLiveActive) {
    displayFillRect(gTouchLiveX, gTouchLiveY, 5, 5, 0xFFFF);
  }
}

static void drawTouchCalibScreen() {
  displayClear();

  displayDrawSmallText(60, 8, "Calibration");
  displayDrawSmallText(34, 22, "Touchez les 5 croix");

  char buf[32];
  snprintf(buf, sizeof(buf), "Point %d/5", calStep + 1);
  displayDrawSmallText(110, 40, buf);

  int cx = calTargets[calStep].sx;
  int cy = calTargets[calStep].sy;
  drawCross(cx, cy);
}

static void drawResetConfirmScreen() {
  drawSettingsScreen();

  displayFillRect(RESET_POPUP_X, RESET_POPUP_Y, RESET_POPUP_W, RESET_POPUP_H, 0x0000);
  displayDrawRect(RESET_POPUP_X, RESET_POPUP_Y, RESET_POPUP_W, RESET_POPUP_H, COLOR_INFO_TEXT);
  displayDrawSmallText(40, 82, "Confirmer le reset");
  displayDrawSmallText(34, 96, "de tous les reglages ?");
  displayDrawSmallText(34, 110, "Cette action est");
  displayDrawSmallText(34, 124, "irreversible.");

  displayDrawRect(RESET_CANCEL_X, RESET_BTN_Y, RESET_BTN_W, RESET_BTN_H, COLOR_INFO_TEXT);
  displayDrawSmallText(56, RESET_BTN_Y + 8, "Annuler");

  displayFillRect(RESET_CONFIRM_X, RESET_BTN_Y, RESET_BTN_W, RESET_BTN_H, COLOR_PLAY_BOX);
  displayDrawRect(RESET_CONFIRM_X, RESET_BTN_Y, RESET_BTN_W, RESET_BTN_H, COLOR_INFO_TEXT);
  displayDrawSmallTextColor(184, RESET_BTN_Y + 8, "Confirmer", COLOR_PLAY_TEXT, COLOR_PLAY_BOX);
}

// --------------------------------------------------
// Hit tests
// --------------------------------------------------
static int hitTestHomeGameImage(int x, int y) {
  int start = pageStartIndex();

  for (int slot = 0; slot < SLOT_COUNT_PER_PAGE; slot++) {
    int gameIndex = start + slot;
    if (gameIndex >= (int)gameList.size()) break;

    int ix, iy, iw, ih;
    getIconRectForSlot(slot, ix, iy, iw, ih);

    int qx, qy, qw, qh;
    getInfoBoxHitRectForSlot(slot, qx, qy, qw, qh);

    if (pointInRect(x, y, qx, qy, qw, qh)) continue;
    if (pointInRect(x, y, ix, iy, iw, ih)) return gameIndex;
  }

  return -1;
}

static int hitTestHomeInfo(int x, int y) {
  int start = pageStartIndex();

  for (int slot = 0; slot < SLOT_COUNT_PER_PAGE; slot++) {
    int gameIndex = start + slot;
    if (gameIndex >= (int)gameList.size()) break;

    int qx, qy, qw, qh;
    getInfoBoxHitRectForSlot(slot, qx, qy, qw, qh);
    if (pointInRect(x, y, qx, qy, qw, qh)) return gameIndex;

    int lx, ly, lw, lh;
    getLabelRectForSlot(slot, lx, ly, lw, lh);
    if (pointInRect(x, y, lx, ly, lw, lh)) return gameIndex;
  }

  return -1;
}

// --------------------------------------------------
// API
// --------------------------------------------------
void launcherInit() {
  Serial.println("[LAUNCHER] init");
  gameList = storageListGames();
  currentPage = 0;
  currentScreen = SCREEN_HOME;
  currentInfoGameIndex = -1;
  gTouchLiveActive = false;
  calStep = 0;
  calWasTouching = false;
  gWifiBusy = false;
  gWifiStatusText = "";
  gNeedsRedraw = true;
}

void launcherUpdate() {
  static bool wasTouching = false;

  bool touching = gTouchPressed;

  gTouchLiveActive = false;

  if (touching) {
    gLastTouchX = gTouchX;
    gLastTouchY = gTouchY;
    gTouchLiveX = gTouchX;
    gTouchLiveY = gTouchY;
    gTouchLiveActive = true;

    if (currentScreen == SCREEN_TOUCH_TEST) {
      gNeedsRedraw = true;
    }
  }

  // calibration par croix avec coordonnées mappées
  if (currentScreen == SCREEN_TOUCH_CALIB) {
    if (touching && !calWasTouching) {
      calTouchX[calStep] = gTouchRawX;
      calTouchY[calStep] = gTouchRawY;

      Serial.printf("[CAL] point %d raw -> X=%d Y=%d\n", calStep, gTouchRawX, gTouchRawY);

      calStep++;
      delay(250);

      if (calStep >= CAL_POINT_COUNT) {
        int xMin = (calTouchX[0] + calTouchX[3]) / 2;
        int xMax = (calTouchX[1] + calTouchX[2]) / 2;
        int yMin = (calTouchY[0] + calTouchY[1]) / 2;
        int yMax = (calTouchY[2] + calTouchY[3]) / 2;

        if (xMax - xMin < 400 || yMax - yMin < 400) {
          Serial.println("[CAL] valeurs invalides, calibration ignoree");
          currentScreen = SCREEN_SETTINGS;
          calStep = 0;
          gNeedsRedraw = true;
          calWasTouching = touching;
          return;
        }

        settingsGet().touch_x_min = xMin;
        settingsGet().touch_x_max = xMax;
        settingsGet().touch_y_min = yMin;
        settingsGet().touch_y_max = yMax;

        settingsGet().touch_x_min = clampValue(settingsGet().touch_x_min, 0, 4095);
        settingsGet().touch_x_max = clampValue(settingsGet().touch_x_max, 0, 4095);
        settingsGet().touch_y_min = clampValue(settingsGet().touch_y_min, 0, 4095);
        settingsGet().touch_y_max = clampValue(settingsGet().touch_y_max, 0, 4095);

        applyTouchCalibrationFromSettings();
        saveSettingsNow();

        Serial.println("[CAL] DONE");

        currentScreen = SCREEN_SETTINGS;
        calStep = 0;
      }

      gNeedsRedraw = true;
    }

    calWasTouching = touching;
  } else {
    calWasTouching = false;
  }

  if (!touching && wasTouching) {
    if (currentScreen == SCREEN_HOME) {
      int hitInfo = hitTestHomeInfo(gLastTouchX, gLastTouchY);
      if (hitInfo >= 0 && hitInfo < (int)gameList.size()) {
        currentInfoGameIndex = hitInfo;
        currentScreen = SCREEN_GAME_INFO;
        gNeedsRedraw = true;
      }
      else {
        int hitGame = hitTestHomeGameImage(gLastTouchX, gLastTouchY);
        if (hitGame >= 0 && hitGame < (int)gameList.size()) {
          launchGameFromIndex(hitGame);
        }
        else if (pointInRect(gLastTouchX, gLastTouchY, BTN_LEFT_X, BTN_LEFT_Y, BTN_LEFT_W, BTN_LEFT_H)) {
          currentScreen = SCREEN_SETTINGS;
          gNeedsRedraw = true;
        }
        else if (pointInRect(gLastTouchX, gLastTouchY, BTN_RIGHT_X, BTN_RIGHT_Y, BTN_RIGHT_W, BTN_RIGHT_H)) {
          int pageCount = totalPages();
          if (pageCount > 1) {
            currentPage++;
            if (currentPage >= pageCount) currentPage = 0;
            gNeedsRedraw = true;
          }
        }
      }
    }
    else if (currentScreen == SCREEN_SETTINGS) {
      if (pointInRect(gLastTouchX, gLastTouchY, SET_MINUS_X, SET_ROW1_Y + SET_BTN_Y_OFFSET, SET_BTN_W, SET_BTN_H)) {
        settingsGet().volume = clampValue(settingsGet().volume - 5, 0, 100);
        audioSetVolume(settingsGet().volume);
        saveSettingsNow();
        gNeedsRedraw = true;
      }
      else if (pointInRect(gLastTouchX, gLastTouchY, SET_PLUS_X, SET_ROW1_Y + SET_BTN_Y_OFFSET, SET_BTN_W, SET_BTN_H)) {
        settingsGet().volume = clampValue(settingsGet().volume + 5, 0, 100);
        audioSetVolume(settingsGet().volume);
        saveSettingsNow();
        gNeedsRedraw = true;
      }
      else if (pointInRect(gLastTouchX, gLastTouchY, SET_MINUS_X, SET_ROW2_Y + SET_BTN_Y_OFFSET, SET_BTN_W, SET_BTN_H)) {
        settingsGet().brightness = clampValue(settingsGet().brightness - 5, 0, 100);
        applyBrightnessNow();
        saveSettingsNow();
        gNeedsRedraw = true;
      }
      else if (pointInRect(gLastTouchX, gLastTouchY, SET_PLUS_X, SET_ROW2_Y + SET_BTN_Y_OFFSET, SET_BTN_W, SET_BTN_H)) {
        settingsGet().brightness = clampValue(settingsGet().brightness + 5, 0, 100);
        applyBrightnessNow();
        saveSettingsNow();
        gNeedsRedraw = true;
      }
      else if (pointInRect(gLastTouchX, gLastTouchY, 8, SET_ROW3_Y, 312, 14)) {
        audioTestBeep();
        gNeedsRedraw = true;
      }
      else if (pointInRect(gLastTouchX, gLastTouchY, 8, SET_ROW4_Y, 312, 14)) {
        currentScreen = SCREEN_TOUCH_TEST;
        gNeedsRedraw = true;
      }
      else if (pointInRect(gLastTouchX, gLastTouchY, 8, SET_ROW5_Y, 312, 14)) {
        calStep = 0;
        currentScreen = SCREEN_TOUCH_CALIB;
        gNeedsRedraw = true;
      }
      else if (pointInRect(gLastTouchX, gLastTouchY, 8, SET_ROW6_Y, 312, 14)) {
        settingsGet().touch_x_min = 200;
        settingsGet().touch_x_max = 3800;
        settingsGet().touch_y_min = 200;
        settingsGet().touch_y_max = 3800;
        applyTouchCalibrationFromSettings();
        saveSettingsNow();
        gNeedsRedraw = true;
      }
      else if (pointInRect(gLastTouchX, gLastTouchY, 8, SET_ROW7_Y, 312, 14)) {
        if (settingsGet().wifi_ssid.length() == 0) {
          Serial.println("[WIFI] SSID non configure dans settings.json");
          gWifiStatusText = "SSID non configure";
          gNeedsRedraw = true;
        } else {
          if (wifiManagerIsActive()) {
            wifiManagerStop();
            gWifiBusy = false;
            gWifiStatusText = "WiFi coupe";
            gNeedsRedraw = true;
          } else {
            gWifiBusy = true;
            gWifiStatusText = "Connexion WiFi...";
            gNeedsRedraw = true;
            launcherRender();

            bool ok = wifiManagerStart(settingsGet().wifi_ssid, settingsGet().wifi_pass);

            gWifiBusy = false;
            if (ok) {
              gWifiStatusText = "Connecte";
            } else {
              gWifiStatusText = "Echec connexion";
            }
            gNeedsRedraw = true;
          }
        }
      }
      else if (pointInRect(gLastTouchX, gLastTouchY, 8, SET_ROW10_Y, 312, 14)) {
        currentScreen = SCREEN_CONFIRM_RESET;
        gNeedsRedraw = true;
      }
      else if (pointInRect(gLastTouchX, gLastTouchY, INFO_BACK_X, INFO_BACK_Y, INFO_BACK_W, INFO_BACK_H)) {
        if (wifiManagerIsActive()) {
          wifiManagerStop();
        }
        gWifiBusy = false;
        gWifiStatusText = "";
        currentScreen = SCREEN_HOME;
        gNeedsRedraw = true;
      }
    }
    else if (currentScreen == SCREEN_GAME_INFO) {
      if (pointInRect(gLastTouchX, gLastTouchY, INFO_PLAY_X, INFO_PLAY_Y, INFO_PLAY_W, INFO_PLAY_H)) {
        launchGameFromIndex(currentInfoGameIndex);
      }
      else if (pointInRect(gLastTouchX, gLastTouchY, INFO_BACK_X, INFO_BACK_Y, INFO_BACK_W, INFO_BACK_H)) {
        currentScreen = SCREEN_HOME;
        gNeedsRedraw = true;
      }
    }
    else if (currentScreen == SCREEN_TOUCH_TEST) {
      if (pointInRect(gLastTouchX, gLastTouchY, INFO_BACK_X, INFO_BACK_Y, INFO_BACK_W, INFO_BACK_H)) {
        currentScreen = SCREEN_SETTINGS;
        gNeedsRedraw = true;
      }
    }
    else if (currentScreen == SCREEN_TOUCH_CALIB) {
      // aucun bouton retour pendant la calibration
    }
    else if (currentScreen == SCREEN_CONFIRM_RESET) {
      if (pointInRect(gLastTouchX, gLastTouchY, RESET_CANCEL_X, RESET_BTN_Y, RESET_BTN_W, RESET_BTN_H)) {
        currentScreen = SCREEN_SETTINGS;
        gNeedsRedraw = true;
      } else if (pointInRect(gLastTouchX, gLastTouchY, RESET_CONFIRM_X, RESET_BTN_Y, RESET_BTN_W, RESET_BTN_H)) {
        settingsSetDefaults();
        applyBrightnessNow();
        audioSetVolume(settingsGet().volume);
        applyTouchCalibrationFromSettings();
        saveSettingsNow();
        currentScreen = SCREEN_SETTINGS;
        gNeedsRedraw = true;
      }
    }
  }

  wasTouching = touching;
}

void launcherRender() {
  if (!gNeedsRedraw) return;
  gNeedsRedraw = false;

  if (currentScreen == SCREEN_HOME) {
    drawHomeScreen();
  } else if (currentScreen == SCREEN_SETTINGS) {
    drawSettingsScreen();
  } else if (currentScreen == SCREEN_GAME_INFO) {
    drawGameInfoScreen();
  } else if (currentScreen == SCREEN_TOUCH_TEST) {
    drawTouchTestScreen();
  } else if (currentScreen == SCREEN_TOUCH_CALIB) {
    drawTouchCalibScreen();
  } else {
    drawResetConfirmScreen();
  }
}