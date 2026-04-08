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

struct HitboxPreset {
  int x;
  int y;
  int w;
  int h;
};

static const HitboxPreset HB_RUN_1  = { 10,  6, 23, 30 };
static const HitboxPreset HB_RUN_2  = { 10,  6, 23, 30 };
static const HitboxPreset HB_JUMP_1 = { 10,  6, 23, 32 };
static const HitboxPreset HB_JUMP_2 = { 10,  7, 23, 33 };
static const HitboxPreset HB_DUCK_1 = { 10, 14, 20, 11 };
static const HitboxPreset HB_DUCK_2 = { 11, 14, 20, 11 };

static const HitboxPreset& currentHitboxPreset() {
  const SpriteFrame& s = currentPlayerSprite();
  if (&s == &SPR_RUN_1)  return HB_RUN_1;
  if (&s == &SPR_RUN_2)  return HB_RUN_2;
  if (&s == &SPR_JUMP_1) return HB_JUMP_1;
  if (&s == &SPR_JUMP_2) return HB_JUMP_2;
  if (&s == &SPR_DUCK_1) return HB_DUCK_1;
  return HB_DUCK_2;
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
  if (onGround) {
    playerY = ducking ? PLAYER_DUCK_Y : PLAYER_STAND_Y;
  }

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
  if (millis() - lastScoreTickMs >= 100) {
    score += 1;
    lastScoreTickMs = millis();
  }

  gameSpeed = 6.4f + score * 0.028f;
  if (gameSpeed > 10.5f) gameSpeed = 10.5f;
}

void getPlayerHitbox(int& x, int& y, int& w, int& h) {
  const HitboxPreset& hb = currentHitboxPreset();
  x = PLAYER_X + hb.x;
  y = (int)playerY + hb.y;
  w = hb.w;
  h = hb.h;
}

void loseOneLife() {
  if (lives > 0) lives--;
  invulnUntil = millis() + HIT_INVULN_MS;

  if (lives == 0) {
    playCrashSound();
    finishGame();
  }
}
