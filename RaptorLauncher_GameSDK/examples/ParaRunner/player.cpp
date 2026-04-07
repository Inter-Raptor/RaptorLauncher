#include "player.h"
#include "game_state.h"
#include "audio.h"
#include "ui.h"

static bool btnJumpHeld() {
  return sdk.isPressed(BTN_A) || sdk.isPressed(BTN_UP);
}

static bool btnDuckHeld() {
  return sdk.isPressed(BTN_DOWN);
}

void resetPlayer() {
  playerY = PLAYER_STAND_Y;
  playerVelY = 0.0f;
  onGround = true;
  ducking = false;
  duckingPrev = false;
  jumpHeldPrev = false;
  lives = MAX_LIVES;
  invulnUntil = 0;
}

void updateAnimation() {
  if (millis() - lastAnimMs >= 110) {
    animToggle = !animToggle;
    lastAnimMs = millis();
  }
}

const SpriteFrame& currentPlayerSprite() {
  if (!onGround) return animToggle ? SPR_JUMP_1 : SPR_JUMP_2;
  if (ducking)   return animToggle ? SPR_DUCK_1 : SPR_DUCK_2;
  return animToggle ? SPR_RUN_1 : SPR_RUN_2;
}

void updatePlayerInput() {
  bool touchHeld = sdk.isTouchHeld();
  int tx = sdk.touchX();
  int ty = sdk.touchY();

  bool touchJumpHeld = false;
  bool touchDuckHeld = false;

  if (touchHeld && ty >= 96) {
    if (tx < SCREEN_W / 2) touchJumpHeld = true;
    else                   touchDuckHeld = true;
  }

  bool jumpHeld = touchJumpHeld || btnJumpHeld();
  bool duckHeld = touchDuckHeld || btnDuckHeld();

  bool jumpPressedEdge = jumpHeld && !jumpHeldPrev;

  // saut uniquement sur nouvel appui
  if (jumpPressedEdge && onGround) {
    playerVelY = JUMP_VELOCITY;
    onGround = false;
    ducking = false;
    playJumpSound();
  }

  // accroupi tant qu'on maintient
  ducking = duckHeld && onGround;
  playerY = ducking ? PLAYER_DUCK_Y : PLAYER_STAND_Y;

  if (ducking && !duckingPrev) {
    playDuckSound();
  }

  duckingPrev = ducking;
  jumpHeldPrev = jumpHeld;
}

void updatePlayerPhysics() {
  if (!onGround) {
    playerVelY += GRAVITY;
    playerY += playerVelY;

    if (playerY >= PLAYER_STAND_Y) {
      playerY = PLAYER_STAND_Y;
      playerVelY = 0.0f;
      onGround = true;
    }
  }
}

void updateScoreAndSpeed() {
  if (millis() - lastScoreTickMs >= 140) {
    score += 1;
    lastScoreTickMs = millis();
  }

  gameSpeed = 5.2f + score * 0.025f;
  if (gameSpeed > 8.0f) gameSpeed = 8.0f;
}

void getPlayerHitbox(int& x, int& y, int& w, int& h) {
  if (ducking && onGround) {
    x = PLAYER_X + 9;
    y = (int)playerY + 10;
    w = 25;
    h = 14;
  } else {
    x = PLAYER_X + 10;
    y = (int)playerY + 5;
    w = 22;
    h = 31;
  }
}

void loseOneLife() {
  if (lives > 0) lives--;
  invulnUntil = millis() + HIT_INVULN_MS;

  if (lives == 0) {
    playCrashSound();
    finishGame();
  }
}