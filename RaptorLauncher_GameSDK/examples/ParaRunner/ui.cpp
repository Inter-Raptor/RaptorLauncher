#include "ui.h"
#include "game_state.h"
#include "player.h"
#include "obstacles.h"
#include "render.h"

static bool btnUpPressed()    { return sdk.isPressed(BTN_UP); }
static bool btnDownPressed()  { return sdk.isPressed(BTN_DOWN); }
static bool btnLeftPressed()  { return sdk.isPressed(BTN_LEFT); }
static bool btnRightPressed() { return sdk.isPressed(BTN_RIGHT); }
static bool btnAJustPressed() { return sdk.isPressed(BTN_A); }

// IMPORTANT : on désactive provisoirement le retour launcher par START
// tant qu'on n'a pas validé le mapping réel des boutons.
void handleGlobalInput() {
  // vide volontairement
}

void startGame() {
  appState = STATE_PLAYING;
  resetPlayer();
  resetObstacles();

  score = 0;
  gameSpeed = 5.4f;

  lastSpawnMs = millis();
  nextSpawnDelayMs = 2000;
  lastScoreTickMs = millis();
}

void finishGame() {
  appState = STATE_GAME_OVER;
  gameOverSelection = GAMEOVER_RETRY;
  if (score > bestScore) bestScore = score;
}

static void activateMenuSelection() {
  if (menuSelection == MENU_PLAY) {
    startGame();
  } else {
    // Désactivé provisoirement :
    // sdk.requestReturnToLauncher();
  }
}

void handleMenuInput() {
  if (btnUpPressed() || btnDownPressed()) {
    menuSelection = (menuSelection == MENU_PLAY) ? MENU_LAUNCHER : MENU_PLAY;
  }

  if (btnAJustPressed()) {
    activateMenuSelection();
    return;
  }

  if (sdk.isTouchPressed()) {
    int tx = sdk.touchX();
    int ty = sdk.touchY();

    if (pointInRect(tx, ty, btnPlay)) {
      menuSelection = MENU_PLAY;
      activateMenuSelection();
      return;
    }

    if (pointInRect(tx, ty, btnLaunch)) {
      menuSelection = MENU_LAUNCHER;
      activateMenuSelection();
      return;
    }
  }
}

static void activateGameOverSelection() {
  if (gameOverSelection == GAMEOVER_RETRY) {
    startGame();
  } else {
    appState = STATE_MENU;
  }
}

void handleGameOverInput() {
  if (btnLeftPressed() || btnRightPressed()) {
    gameOverSelection = (gameOverSelection == GAMEOVER_RETRY) ? GAMEOVER_MENU : GAMEOVER_RETRY;
  }

  if (btnAJustPressed()) {
    activateGameOverSelection();
    return;
  }

  if (sdk.isTouchPressed()) {
    int tx = sdk.touchX();
    int ty = sdk.touchY();

    if (pointInRect(tx, ty, btnRetry)) {
      gameOverSelection = GAMEOVER_RETRY;
      activateGameOverSelection();
      return;
    }

    if (pointInRect(tx, ty, btnMenu)) {
      gameOverSelection = GAMEOVER_MENU;
      activateGameOverSelection();
      return;
    }
  }
}