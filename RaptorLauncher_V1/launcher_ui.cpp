#include <Arduino.h>
#include <vector>
#include "launcher_ui.h"
#include "display_manager.h"
#include "touch_manager.h"
#include "audio_manager.h"
#include "storage_manager.h"

static bool gNeedsRedraw = true;
static int gTouchX = -1;
static int gTouchY = -1;
static int gSelected = -1;

static bool inGameMenu = false;
static std::vector<String> gameList;
static int gameIndex = 0;

// ---------- Menu principal ----------
static int menuHitTest(int x, int y) {
  (void)x;

  if (y >= 35 && y <= 60)   return 0; // Jeux
  if (y >= 65 && y <= 90)   return 1; // Parametres
  if (y >= 95 && y <= 120)  return 2; // Test tactile
  if (y >= 125 && y <= 150) return 3; // Test audio

  return -1;
}

// ---------- Menu jeux ----------
static int gameMenuHitTest(int x, int y) {
  (void)x;

  // Zone "retour" en haut
  if (y >= 0 && y <= 25) {
    return -2;
  }

  // Liste des jeux à partir de y=40, pas de 20 px
  for (int i = 0; i < (int)gameList.size(); i++) {
    int y0 = 40 + i * 20;
    int y1 = y0 + 18;
    if (y >= y0 && y <= y1) {
      return i;
    }
  }

  return -1;
}

static void drawMainMenu() {
  displayClear();

  displayDrawText(10, 10, "Raptor Launcher");
  displayDrawText(10, 40,  gSelected == 0 ? "> Jeux" : "  Jeux");
  displayDrawText(10, 70,  gSelected == 1 ? "> Parametres" : "  Parametres");
  displayDrawText(10, 100, gSelected == 2 ? "> Test tactile" : "  Test tactile");
  displayDrawText(10, 130, gSelected == 3 ? "> Test audio" : "  Test audio");

  char buf[64];
  snprintf(buf, sizeof(buf), "Touch: %d , %d   ", gTouchX, gTouchY);
  displayDrawText(10, 200, buf);
}

static void drawGameMenu() {
  displayClear();

  displayDrawText(10, 10, "< Retour");
  displayDrawText(10, 30, "Liste des jeux :");

  if (gameList.empty()) {
    displayDrawText(10, 60, "Aucun jeu trouve");
  } else {
    for (int i = 0; i < (int)gameList.size(); i++) {
      String line = (i == gameIndex ? "> " : "  ");
      line += gameList[i];
      displayDrawText(10, 40 + i * 20, line.c_str());
    }
  }

  char buf[64];
  snprintf(buf, sizeof(buf), "Touch: %d , %d   ", gTouchX, gTouchY);
  displayDrawText(10, 200, buf);
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
    // Relâchement = clic
    if (!inGameMenu) {
      Serial.print("[CLICK] selection = ");
      Serial.println(gSelected);

      if (gSelected == 0) {
        // Jeux
        gameList = storageListGames();
        gameIndex = 0;
        inGameMenu = true;
        gNeedsRedraw = true;
      }
      else if (gSelected == 1) {
        // Parametres
        displayClear();
        displayDrawText(10, 10, "Parametres");
        displayDrawText(10, 40, "Pas encore implemente");
        delay(800);
        gNeedsRedraw = true;
      }
      else if (gSelected == 2) {
        // Test tactile
        displayClear();
        displayDrawText(10, 10, "Test tactile");
        displayDrawText(10, 40, "Touchez l'ecran");
        delay(800);
        gNeedsRedraw = true;
      }
      else if (gSelected == 3) {
        // Test audio
        displayClear();
        displayDrawText(10, 10, "Test audio");
        displayDrawText(10, 40, "Message serie seulement");
        audioTestBeep();
        delay(800);
        gNeedsRedraw = true;
      }
    } else {
      int hit = gameMenuHitTest(gTouchX, gTouchY);

      if (hit == -2) {
        // Retour
        inGameMenu = false;
        gSelected = -1;
        gNeedsRedraw = true;
      }
      else if (hit >= 0 && hit < (int)gameList.size()) {
        gameIndex = hit;

        displayClear();
        displayDrawText(10, 10, "Jeu selectionne :");
        displayDrawText(10, 40, gameList[gameIndex].c_str());
        displayDrawText(10, 70, "Lancement plus tard");
        Serial.print("[GAME] selection = ");
        Serial.println(gameList[gameIndex]);
        delay(1000);
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