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

static const int MAIN_TITLE_Y      = 10;
static const int MAIN_MENU_Y0      = 50;
static const int MAIN_MENU_STEP    = 30;

static const int GAME_BACK_Y       = 10;
static const int GAME_TITLE_Y      = 40;
static const int GAME_LIST_Y0      = 80;
static const int GAME_LIST_STEP    = 28;

static int menuHitTest(int x, int y) {
  (void)x;

  if (y >= 45 && y <= 65)   return 0;
  if (y >= 75 && y <= 95)   return 1;
  if (y >= 105 && y <= 125) return 2;
  if (y >= 135 && y <= 155) return 3;

  return -1;
}

static int gameMenuHitTest(int x, int y) {
  (void)x;

  if (y >= 0 && y <= 30) {
    return -2;
  }

  for (int i = 0; i < (int)gameList.size(); i++) {
    int y0 = GAME_LIST_Y0 + i * GAME_LIST_STEP;
    int y1 = y0 + 22;
    if (y >= y0 && y <= y1) {
      return i;
    }
  }

  return -1;
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

  displayDrawText(10, GAME_BACK_Y, "< Retour");
  displayDrawText(10, GAME_TITLE_Y, "Liste des jeux :");

  if (gameList.empty()) {
    displayDrawText(10, GAME_LIST_Y0, "Aucun jeu trouve");
  } else {
    for (int i = 0; i < (int)gameList.size(); i++) {
      String line = (i == gameIndex ? "> " : "  ");
      line += gameList[i].name;
      displayDrawText(10, GAME_LIST_Y0 + i * GAME_LIST_STEP, line.c_str());
    }
  }

  char buf[64];
  snprintf(buf, sizeof(buf), "Touch: %d , %d   ", gTouchX, gTouchY);
  displayDrawText(10, 210, buf);
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
      int hit = gameMenuHitTest(x, y);
      if (hit >= 0 && hit < (int)gameList.size()) {
        if (hit != gameIndex) {
          gameIndex = hit;
          gNeedsRedraw = true;
        }
      } else if (hit == -2) {
        gNeedsRedraw = true;
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
      int hit = gameMenuHitTest(gTouchX, gTouchY);

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

        if (gameList[gameIndex].cover.length() > 0 &&
            gameList[gameIndex].coverW > 0 &&
            gameList[gameIndex].coverH > 0) {

          String path = "/games" + gameList[gameIndex].folder + "/" + gameList[gameIndex].cover;

          Serial.print("[GAME] image path = ");
          Serial.println(path);

          if (gameList[gameIndex].cover.endsWith(".raw")) {
            imageOk = displayDrawRAW(
              path.c_str(),
              150,
              20,
              gameList[gameIndex].coverW,
              gameList[gameIndex].coverH
            );
          } else if (gameList[gameIndex].cover.endsWith(".bmp")) {
            imageOk = displayDrawBMP(path.c_str(), 150, 20);
          }

          if (imageOk) {
            displayDrawText(10, 130, "Image OK");
          } else {
            displayDrawText(10, 130, "Image KO");
          }
        } else {
          displayDrawText(10, 130, "Pas de cover ou taille absente");
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