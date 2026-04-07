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

static uint16_t spriteTransparencyKeyFromTopRight(const SpriteFrame& s) {
  return pgm_read_word(&s.pixels[s.w - 1]);
}

static void getOpaqueBounds(const SpriteFrame& s, int& minX, int& minY, int& maxX, int& maxY) {
  const uint16_t key = spriteTransparencyKeyFromTopRight(s);
  minX = s.w;
  minY = s.h;
  maxX = -1;
  maxY = -1;

  for (int yy = 0; yy < s.h; yy++) {
    for (int xx = 0; xx < s.w; xx++) {
      uint16_t c = pgm_read_word(&s.pixels[yy * s.w + xx]);
      if (c == key) continue;
      if (xx < minX) minX = xx;
      if (yy < minY) minY = yy;
      if (xx > maxX) maxX = xx;
      if (yy > maxY) maxY = yy;
    }
  }

  if (maxX < minX || maxY < minY) {
    minX = 0;
    minY = 0;
    maxX = s.w - 1;
    maxY = s.h - 1;
  }
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
  const SpriteFrame& s = currentPlayerSprite();

  int minX, minY, maxX, maxY;
  getOpaqueBounds(s, minX, minY, maxX, maxY);

  const int trimX = ducking ? 2 : 3;
  const int trimTop = ducking ? 1 : 2;
  const int trimBottom = ducking ? 1 : 3;

  minX += trimX;
  maxX -= trimX;
  minY += trimTop;
  maxY -= trimBottom;

  if (maxX < minX) { minX = 0; maxX = s.w - 1; }
  if (maxY < minY) { minY = 0; maxY = s.h - 1; }

  x = PLAYER_X + minX;
  y = (int)playerY + minY;
  w = (maxX - minX + 1);
  h = (maxY - minY + 1);
}

void loseOneLife() {
  if (lives > 0) lives--;
  invulnUntil = millis() + HIT_INVULN_MS;

  if (lives == 0) {
    playCrashSound();
    finishGame();
  }
}
