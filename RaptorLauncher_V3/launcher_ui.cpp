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
static int gameIndex = 0;

static const int MAIN_TITLE_Y   = 10;
static const int MAIN_MENU_Y0   = 50;
static const int MAIN_MENU_STEP = 30;

// Galerie 2x2 temporaire
static const int GRID_COLS = 2;
static const int GRID_ROWS = 2;
static const int GRID_CELL_W = 160;
static const int GRID_CELL_H = 95;

static const int ICON_X_OFFSET = 30;
static const int ICON_Y_OFFSET = 8;
static const int LABEL_Y_OFFSET = 76;

static int menuHitTest(int x, int y) {
  (void)x;

  if (y >= 45 && y <= 65)   return 0;
  if (y >= 75 && y <= 95)   return 1;
  if (y >= 105 && y <= 125) return 2;
  if (y >= 135 && y <= 155) return 3;

  return -1;
}

static int gameGridHitTest(int x, int y) {
  // zone bouton retour
  if (x >= 0 && x <= 90 && y >= 0 && y <= 30) {
    return -2;
  }

  // grille 2x2 sous le bandeau haut
  if (y < 35) return -1;

  int gridY = y - 35;
  int col = x / GRID_CELL_W;
  int row = gridY / GRID_CELL_H;

  if (col < 0 || col >= GRID_COLS || row < 0 || row >= GRID_ROWS) {
    return -1;
  }

  int index = row * GRID_COLS + col;

  if (index >= (int)gameList.size()) {
    return -1;
  }

  return index;
}

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

static void drawGameMenu() {
  displayClear();

  displayDrawText(10, 10, "< Retour");
  displayDrawText(120, 10, "Jeux");

  for (int i = 0; i < 4 && i < (int)gameList.size(); i++) {
    int col = i % GRID_COLS;
    int row = i / GRID_COLS;

    int cellX = col * GRID_CELL_W;
    int cellY = 35 + row * GRID_CELL_H;

    int iconX = cellX + ICON_X_OFFSET;
    int iconY = cellY + ICON_Y_OFFSET;

    int labelY = cellY + LABEL_Y_OFFSET;

    // petit cadre simple avec le nom marque si selectionne
    if (i == gameIndex) {
      displayDrawText(cellX + 5, cellY + 2, ">");
    }

    bool imageOk = false;

    if (gameList[i].icon.length() > 0 &&
        gameList[i].iconW > 0 &&
        gameList[i].iconH > 0) {

      String path = "/games" + gameList[i].folder + "/" + gameList[i].icon;

      if (gameList[i].icon.endsWith(".raw")) {
        imageOk = displayDrawRAW(
          path.c_str(),
          iconX,
          iconY,
          gameList[i].iconW,
          gameList[i].iconH
        );
      } else if (gameList[i].icon.endsWith(".bmp")) {
        imageOk = displayDrawBMP(path.c_str(), iconX, iconY);
      }
    }

    if (!imageOk) {
      displayDrawText(iconX, iconY + 35, "Image KO");
    }

    displayDrawText(cellX + 10, labelY, gameList[i].name.c_str());
  }

  char buf[64];
  snprintf(buf, sizeof(buf), "Touch: %d , %d   ", gTouchX, gTouchY);
  displayDrawText(10, 220, buf);
}

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
    } else {
      int hit = gameGridHitTest(x, y);
      if (hit >= 0 && hit < (int)gameList.size()) {
        if (hit != gameIndex) {
          gameIndex = hit;
          gNeedsRedraw = true;
        }
      }
    }
  }
  else if (wasTouching) {
    if (!inGameMenu) {
      if (gSelected == 0) {
        gameList = storageListGames();
        gameIndex = 0;
        inGameMenu = true;
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
    } else {
      int hit = gameGridHitTest(gTouchX, gTouchY);

      if (hit == -2) {
        inGameMenu = false;
        gSelected = -1;
        gNeedsRedraw = true;
      }
      else if (hit >= 0 && hit < (int)gameList.size()) {
        gameIndex = hit;

        displayClear();
        displayDrawText(10, 10, "Jeu selectionne :");
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

          Serial.print("[GAME] icon path = ");
          Serial.println(path);

          if (gameList[gameIndex].icon.endsWith(".raw")) {
            imageOk = displayDrawRAW(
              path.c_str(),
              180,
              20,
              gameList[gameIndex].iconW,
              gameList[gameIndex].iconH
            );
          } else if (gameList[gameIndex].icon.endsWith(".bmp")) {
            imageOk = displayDrawBMP(path.c_str(), 180, 20);
          }

          if (imageOk) {
            displayDrawText(10, 130, "Image OK");
          } else {
            displayDrawText(10, 130, "Image KO");
          }
        } else {
          displayDrawText(10, 130, "Pas d'icone ou taille absente");
        }

        if (gameList[gameIndex].title.length() > 0) {
          Serial.print("[GAME] title path = /games");
          Serial.print(gameList[gameIndex].folder);
          Serial.print("/");
          Serial.println(gameList[gameIndex].title);
        }

        delay(3000);
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
  } else {
    drawGameMenu();
  }
}