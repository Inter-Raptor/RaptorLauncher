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

enum LauncherScreen {
  SCREEN_HOME,
  SCREEN_SETTINGS
};

static LauncherScreen currentScreen = SCREEN_HOME;

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

static const int LABEL_OFFSET_Y = 52;

// Footer boutons
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

// Settings
static const int SETTINGS_BACK_X = 8;
static const int SETTINGS_BACK_Y = 202;
static const int SETTINGS_BACK_W = 110;
static const int SETTINGS_BACK_H = 34;

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
  h = ICON_H + 12;

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

static String shortenLabel(const String& s) {
  if (s.length() <= 10) return s;
  return s.substring(0, 10) + ".";
}

static void drawFooterButton(int x, int y, const char* text) {
  displayDrawText(x, y + 8, text);
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

// --------------------------------------------------
// Dessin écrans
// --------------------------------------------------
static void drawHomeScreen() {
  displayClear();

  int pageCount = totalPages();

  if (pageCount > 1) {
    char pageBuf[32];
    snprintf(pageBuf, sizeof(pageBuf), "%d/%d", currentPage + 1, pageCount);
    displayDrawText(270, 4, pageBuf);
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
      displayDrawText(x, y + 18, "KO");
    }

    String label = shortenLabel(game.name);
    displayDrawText(x, y + LABEL_OFFSET_Y, label.c_str());
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

  drawFooterButton(SETTINGS_BACK_X, SETTINGS_BACK_Y, "Retour");
}

// --------------------------------------------------
// Hit tests
// --------------------------------------------------
static int hitTestHomeGame(int x, int y) {
  int start = pageStartIndex();

  for (int slot = 0; slot < SLOT_COUNT_PER_PAGE; slot++) {
    int gameIndex = start + slot;
    if (gameIndex >= (int)gameList.size()) break;

    int rx, ry, rw, rh;
    getSlotRect(slot, rx, ry, rw, rh);

    if (pointInRect(x, y, rx, ry, rw, rh)) {
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
      int hitGame = hitTestHomeGame(gTouchX, gTouchY);
      if (hitGame >= 0 && hitGame < (int)gameList.size()) {
        Serial.print("[GAME] selected = ");
        Serial.println(gameList[hitGame].name);
        showGameTitleScreen(gameList[hitGame]);
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
    else if (currentScreen == SCREEN_SETTINGS) {
      if (pointInRect(gTouchX, gTouchY, SETTINGS_BACK_X, SETTINGS_BACK_Y, SETTINGS_BACK_W, SETTINGS_BACK_H)) {
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
  } else {
    drawSettingsScreen();
  }
}