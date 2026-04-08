#include <Arduino.h>
#include "raptor_game_sdk.h"

#include "config.h"
#include "game_state.h"
#include "audio.h"
#include "player.h"
#include "obstacles.h"
#include "render.h"
#include "ui.h"

void setup() {
  randomSeed((uint32_t)micros());
  sdk.begin();

  appState = STATE_MENU;
  bestScore = 0;
  resetPlayer();
  resetObstacles();
}

void loop() {
  sdk.updateInputs();
  updateAnimation();
  handleGlobalInput();

  switch (appState) {
    case STATE_MENU:
      handleMenuInput();
      drawMenuScreen();
      break;

    case STATE_PLAYING:
      updatePlayerInput();
      updatePlayerPhysics();
      updateObstacles();
      updateScoreAndSpeed();
      checkCollisions();
      drawGameScreen();
      break;

    case STATE_GAME_OVER:
      handleGameOverInput();
      drawGameOverScreen();
      break;
  }

  delay(28);
}
