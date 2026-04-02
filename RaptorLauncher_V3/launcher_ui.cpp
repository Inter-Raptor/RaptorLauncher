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
static int gSelected = -1;

static bool inGameMenu = false;
static std::vector<GameInfo> gameList;
static int gameIndex = -1;

// -------------------------
// Menu principal
// -------------------------
static const int MAIN_TITLE_Y   = 10;
static const int MAIN_MENU_Y0   = 50;
static const int MAIN_MENU_STEP = 30;

// -------------------------
// Grille jeux 2x2
// -------------------------
static const int GRID_COLS = 2;
static const int GRID_ROWS = 2;
static const int GRID_COUNT = GRID_COLS * GRID_ROWS;

static const int GRID_START_X = 30;
static const int GRID_START_Y = 45;

static const int CELL_W = 130;
static const int CELL_H = 85;

static const int ICON_DRAW_W = 60;
static const int ICON_DRAW_H = 60;

static const int BACK_BTN_X = 10;
static const int BACK_BTN_Y = 10;
static const int BACK_BTN_W = 80;
static const int BACK_BTN_H = 24;

// -------------------------
// Ecran détail jeu
// -------------------------
static bool inGameDetail = false;

// =====================================================
// HIT TEST
// =====================================================
static int menuHitTest(int x, int y) {
  (void)x;

  if (y >= 45 && y <= 65)   return 0; // Jeux
  if (y >= 75 && y <= 95)   return 1; // Parametres
  if (y >= 105 && y <= 125) return 2; // Test tactile
  if (y >= 135 && y <= 155) return 3; // Test audio

  return -1;
}

static bool backButtonHitTest(int x, int y) {
  return (x >= BACK_BTN_X && x < BACK_BTN_X + BACK_BTN_W &&
          y >= BACK_BTN_Y && y < BACK_BTN_Y + BACK_BTN_H);
}

static int gameGridHitTest(int x, int y) {
  for (int row = 0; row < GRID_ROWS; row++) {
    for (int col = 0; col < GRID_COLS; col++) {
      int idx = row * GRID_COLS + col;
      if (idx >= (int)gameList.size()) return -1;

      int cellX = GRID_START_X + col * CELL_W;
      int cellY = GRID_START_Y + row * CELL_H;

      if (x >= cellX && x < cellX + CELL_W &&
          y >= cellY && y < cellY + CELL_H) {
        return idx;
      }
    }
  }
  return -1;
}

// =====================================================
// DRAW
// =====================================================
static void drawMainMenu() {
  displayClear();

  displayDrawText(10, MAIN_TITLE_Y, "Raptor Launcher");

  displayDrawText(10, MAIN_MENU_Y0 + 0 * MAIN_MENU_STEP, gSelected == 0 ? "> Jeux" : "  Jeux");
  displayDrawText(10, MAIN_MENU_Y0 + 1 * MAIN_MENU_STEP, gSelected == 1 ? "> Parametres" : "  Parametres");
  displayDrawText(10, MAIN_MENU_Y0 + 2 * MAIN_MENU_STEP, gSelected == 2 ? "> Test tactile" : "  Test tactile");
  displayDrawText(10, MAIN_MENU_Y0 + 3 * MAIN_MENU_STEP, gSelected == 3 ? "> Test audio" : "  Test audio");

  char buf[64];
  snprintf(buf, sizeof(buf), "Touch: %d , %d   ", gTouchX, gTouchY);
  displayDrawText(10, 210, buf);
}

static void drawGameGrid() {
  displayClear();

  displayDrawText(BACK_BTN_X, BACK_BTN_Y, "< Retour");
  displayDrawText(110, 10, "Jeux");

  if (gameList.empty()) {
    displayDrawText(10, 60, "Aucun jeu trouve");
    return;
  }

  for (int row = 0; row < GRID_ROWS; row++) {
    for (int col = 0; col < GRID_COLS; col++) {
      int idx = row * GRID_COLS + col;
      if (idx >= (int)gameList.size()) return;

      int cellX = GRID_START_X + col * CELL_W;
      int cellY = GRID_START_Y + row * CELL_H;

      int iconX = cellX + 10;
      int iconY = cellY + 2;

      int textY = cellY + 66;

      if (gameList[idx].icon.length() > 0 &&
          gameList[idx].iconW > 0 &&
          gameList[idx].iconH > 0) {

        String path = "/games" + gameList[idx].folder + "/" + gameList[idx].icon;

        if (gameList[idx].icon.endsWith(".raw")) {
          displayDrawRAW(path.c_str(), iconX, iconY, gameList[idx].iconW, gameList[idx].iconH);
        } else if (gameList[idx].icon.endsWith(".bmp")) {
          displayDrawBMP(path.c_str(), iconX, iconY);
        }
      }

      String label = gameList[idx].name;
      if ((int)label.length() > 14) {
        label = label.substring(0, 14);
      }

      displayDrawText(cellX, textY, label.c_str());

      if (idx == gSelected) {
        displayDrawText(cellX + 90, cellY + 20, "<");
      }
    }
  }
}

static void drawGameDetail() {
  displayClear();

  displayDrawText(10, 10, "< Retour");
  displayDrawText(10, 40, gameList[gameIndex].name.c_str());

  if (gameList[gameIndex].author.length() > 0) {
    String authorLine = "Par: " + gameList[gameIndex].author;
    displayDrawText(10, 70, authorLine.c_str());
  }

  if (gameList[gameIndex].description.length() > 0) {
    displayDrawText(10, 100, gameList[gameIndex].description.c_str());
  }

  bool imageOk = false;

  if (gameList[gameIndex].icon.length() > 0 &&
      gameList[gameIndex].iconW > 0 &&
      gameList[gameIndex].iconH > 0) {

    String path = "/games" + gameList[gameIndex].folder + "/" + gameList[gameIndex].icon;

    if (gameList[gameIndex].icon.endsWith(".raw")) {
      imageOk = displayDrawRAW(
        path.c_str(),
        200,
        20,
        gameList[gameIndex].iconW,
        gameList[gameIndex].iconH
      );
    } else if (gameList[gameIndex].icon.endsWith(".bmp")) {
      imageOk = displayDrawBMP(path.c_str(), 200, 20);
    }
  }

  displayDrawText(10, 180, imageOk ? "Image OK" : "Image KO");

  if (gameList[gameIndex].title.length() > 0) {
    Serial.print("[GAME] title path = /games");
    Serial.print(gameList[gameIndex].folder);
    Serial.print("/");
    Serial.println(gameList[gameIndex].title);
  }
}

// =====================================================
// API
// =====================================================
void launcherInit() {
  Serial.println("[LAUNCHER] init");
  gNeedsRedraw = true;
}

void launcherUpdate() {
  static bool wasTouching = false;

  int x, y;
  bool touching = touchPressed(x, y);

  if (touching) {
    gTouchX = x;
    gTouchY = y;

    if (!inGameMenu) {
      int hit = menuHitTest(x, y);
      if (hit != gSelected) {
        gSelected = hit;
        gNeedsRedraw = true;
      }
    } else if (!inGameDetail) {
      int hit = gameGridHitTest(x, y);
      if (hit != gSelected) {
        gSelected = hit;
        gNeedsRedraw = true;
      }
    }
  }
  else if (wasTouching) {
    if (!inGameMenu) {
      if (gSelected == 0) {
        gameList = storageListGames();
        gSelected = -1;
        gameIndex = -1;
        inGameMenu = true;
        inGameDetail = false;
        gNeedsRedraw = true;
      }
      else if (gSelected == 1) {
        displayClear();
        displayDrawText(10, 10, "Parametres");
        displayDrawText(10, 50, "Pas encore implemente");
        delay(800);
        gNeedsRedraw = true;
      }
      else if (gSelected == 2) {
        displayClear();
        displayDrawText(10, 10, "Test tactile");
        displayDrawText(10, 50, "Touchez l'ecran");
        delay(800);
        gNeedsRedraw = true;
      }
      else if (gSelected == 3) {
        displayClear();
        displayDrawText(10, 10, "Test audio");
        displayDrawText(10, 50, "Message serie seulement");
        audioTestBeep();
        delay(800);
        gNeedsRedraw = true;
      }
    } else if (!inGameDetail) {
      if (backButtonHitTest(gTouchX, gTouchY)) {
        inGameMenu = false;
        gSelected = -1;
        gNeedsRedraw = true;
      } else {
        int hit = gameGridHitTest(gTouchX, gTouchY);
        if (hit >= 0 && hit < (int)gameList.size()) {
          gameIndex = hit;
          inGameDetail = true;
          gNeedsRedraw = true;
        }
      }
    } else {
      if (backButtonHitTest(gTouchX, gTouchY)) {
        inGameDetail = false;
        gNeedsRedraw = true;
      }
    }
  }

  wasTouching = touching;
}

void launcherRender() {
  if (!gNeedsRedraw) return;

  gNeedsRedraw = false;

  if (!inGameMenu) {
    drawMainMenu();
  } else if (!inGameDetail) {
    drawGameGrid();
  } else {
    drawGameDetail();
  }
}