#include <Arduino.h>
#include <vector>
#include "launcher_ui.h"
#include "display_manager.h"
#include "touch_manager.h"
#include "audio_manager.h"
#include "storage_manager.h"
#include "types.h"

static bool gNeedsRedraw = true;
static int gTouchX = -1;
static int gTouchY = -1;

static std::vector<GameInfo> gameList;
static int currentPage = 0;
static int currentInfoGameIndex = -1;

enum LauncherScreen {
  SCREEN_HOME,
  SCREEN_SETTINGS,
  SCREEN_GAME_INFO
};

static LauncherScreen currentScreen = SCREEN_HOME;

// --------------------------------------------------
// Couleurs
// --------------------------------------------------
static const uint16_t COLOR_INFO_BOX   = 0x7D7C; // bleu clair
static const uint16_t COLOR_INFO_TEXT  = 0x0000; // noir
static const uint16_t COLOR_PLAY_BOX   = 0x07E0; // vert
static const uint16_t COLOR_PLAY_TEXT  = 0xFFFF; // blanc

// --------------------------------------------------
// Mise en page
// --------------------------------------------------
static const int ICON_W = 50;
static const int ICON_H = 50;

static const int SLOT_COUNT_PER_PAGE = 10;

// Rangée 1
static const int ROW1_Y = 12;
static const int ROW2_Y = 80;
static const int ROW3_Y = 140;

// Colonnes 4x
static const int COL1_X = 15;
static const int COL2_X = 90;
static const int COL3_X = 165;
static const int COL4_X = 240;

// Rangée 3 centrée sur 2 icônes
static const int ROW3_COL1_X = 72;
static const int ROW3_COL2_X = 197;

static const int LABEL_OFFSET_Y_LINE1 = 52;
static const int LABEL_OFFSET_Y_LINE2 = 60;

// Bouton info visible sur l’icône
static const int INFO_BOX_SIZE = 10;

// Zone tactile du ? plus grande que le carré visible
static const int INFO_HITBOX_SIZE = 20;

// Footer home
static const int FOOTER_Y = 202;
static const int FOOTER_H = 34;

static const int BTN_LEFT_X = 8;
static const int BTN_LEFT_Y = FOOTER_Y;
static const int BTN_LEFT_W = 110;
static const int BTN_LEFT_H = FOOTER_H;

static const int BTN_RIGHT_X = 202;
static const int BTN_RIGHT_Y = FOOTER_Y;
static const int BTN_RIGHT_W = 110;
static const int BTN_RIGHT_H = FOOTER_H;

// Game info / settings buttons
static const int INFO_PLAY_X = 8;
static const int INFO_PLAY_Y = 202;
static const int INFO_PLAY_W = 90;
static const int INFO_PLAY_H = 34;

static const int INFO_BACK_X = 222;
static const int INFO_BACK_Y = 202;
static const int INFO_BACK_W = 90;
static const int INFO_BACK_H = 34;

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

  x = slotX;
  y = slotY;
  w = ICON_W;
  h = ICON_H;
}

static void getLabelRectForSlot(int slot, int &x, int &y, int &w, int &h) {
  int slotX, slotY, slotW, slotH;
  getSlotRect(slot, slotX, slotY, slotW, slotH);

  x = slotX;
  y = slotY + LABEL_OFFSET_Y_LINE1 - 1;
  w = ICON_W;
  h = 18;
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
    Serial.print("[GAME] title path = ");
    Serial.println(path);

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

  Serial.print("[GAME] selected = ");
  Serial.println(gameList[index].name);

  showGameTitleScreen(gameList[index]);
}

// --------------------------------------------------
// Dessin écrans
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

  displayDrawText(8, 8, "Parametres");
  displayDrawText(8, 40, "Volume : plus tard");
  displayDrawText(8, 64, "Luminosite : plus tard");
  displayDrawText(8, 88, "Test tactile : plus tard");
  displayDrawText(8, 112, "Test audio : plus tard");
  displayDrawText(8, 136, "Credits : plus tard");

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

    if (pointInRect(x, y, qx, qy, qw, qh)) {
      continue;
    }

    if (pointInRect(x, y, ix, iy, iw, ih)) {
      return gameIndex;
    }
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

    if (pointInRect(x, y, qx, qy, qw, qh)) {
      return gameIndex;
    }

    int lx, ly, lw, lh;
    getLabelRectForSlot(slot, lx, ly, lw, lh);

    if (pointInRect(x, y, lx, ly, lw, lh)) {
      return gameIndex;
    }
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
  gNeedsRedraw = true;
}

void launcherUpdate() {
  static bool wasTouching = false;

  int x, y;
  bool touching = touchPressed(x, y);

  if (touching) {
    gTouchX = x;
    gTouchY = y;
  }
  else if (wasTouching) {
    if (currentScreen == SCREEN_HOME) {
      int hitInfo = hitTestHomeInfo(gTouchX, gTouchY);
      if (hitInfo >= 0 && hitInfo < (int)gameList.size()) {
        currentInfoGameIndex = hitInfo;
        currentScreen = SCREEN_GAME_INFO;
        gNeedsRedraw = true;
      }
      else {
        int hitGame = hitTestHomeGameImage(gTouchX, gTouchY);
        if (hitGame >= 0 && hitGame < (int)gameList.size()) {
          launchGameFromIndex(hitGame);
        }
        else if (pointInRect(gTouchX, gTouchY, BTN_LEFT_X, BTN_LEFT_Y, BTN_LEFT_W, BTN_LEFT_H)) {
          currentScreen = SCREEN_SETTINGS;
          gNeedsRedraw = true;
        }
        else if (pointInRect(gTouchX, gTouchY, BTN_RIGHT_X, BTN_RIGHT_Y, BTN_RIGHT_W, BTN_RIGHT_H)) {
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
      if (pointInRect(gTouchX, gTouchY, INFO_BACK_X, INFO_BACK_Y, INFO_BACK_W, INFO_BACK_H)) {
        currentScreen = SCREEN_HOME;
        gNeedsRedraw = true;
      }
    }
    else if (currentScreen == SCREEN_GAME_INFO) {
      if (pointInRect(gTouchX, gTouchY, INFO_PLAY_X, INFO_PLAY_Y, INFO_PLAY_W, INFO_PLAY_H)) {
        launchGameFromIndex(currentInfoGameIndex);
      }
      else if (pointInRect(gTouchX, gTouchY, INFO_BACK_X, INFO_BACK_Y, INFO_BACK_W, INFO_BACK_H)) {
        currentScreen = SCREEN_HOME;
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
  } else {
    drawGameInfoScreen();
  }
}