#include "../../arduino_template/src/raptor_game_sdk.h"

RaptorGameSDK sdk;

enum GameState { STATE_TITLE, STATE_PLAYING, STATE_GAME_OVER };
enum ObstacleKind { OBS_CACTUS, OBS_PTERA };

struct Rect {
  int x;
  int y;
  int w;
  int h;
};

struct Obstacle {
  bool active;
  ObstacleKind kind;
  float x;
  int y;
  int w;
  int h;
  Rect prevRect;
};

static constexpr uint16_t COLOR_SKY = 0xC69F;
static constexpr uint16_t COLOR_GROUND = 0xA4E0;
static constexpr uint16_t COLOR_GROUND_LINE = 0x4208;
static constexpr uint16_t COLOR_DINO = 0x0000;
static constexpr uint16_t COLOR_CACTUS = 0x03E0;
static constexpr uint16_t COLOR_PTERA = 0x001F;
static constexpr uint16_t COLOR_TEXT = 0x0000;
static constexpr uint16_t COLOR_HUD_BG = 0xC69F;

static const int SCREEN_W = 320;
static const int SCREEN_H = 240;
static const int HUD_H = 24;
static const int GROUND_Y = 190;
static const int GROUND_H = SCREEN_H - GROUND_Y;

static const int DINO_X = 52;
static const int DINO_RUN_W = 24;
static const int DINO_RUN_H = 34;
static const int DINO_DUCK_W = 30;
static const int DINO_DUCK_H = 20;

static const int MAX_OBS = 4;
Obstacle obstacles[MAX_OBS];

GameState gameState = STATE_TITLE;

float dinoY = (float)(GROUND_Y - DINO_RUN_H);
float dinoVy = 0.0f;
bool dinoGrounded = true;
bool dinoDucking = false;

Rect prevDinoRect = { DINO_X, GROUND_Y - DINO_RUN_H, DINO_RUN_W, DINO_RUN_H };

float worldSpeed = 210.0f;
float worldAccel = 6.0f;
const float jumpVelocity = -430.0f;
const float gravity = 1450.0f;
const float maxFallSpeed = 700.0f;

const uint32_t jumpBufferMs = 100;
const uint32_t coyoteMs = 80;
uint32_t lastJumpPressMs = 0;
uint32_t lastGroundedMs = 0;

uint32_t prevMs = 0;
uint32_t nextSpawnMs = 0;
uint32_t scoreMs = 0;
uint32_t bestScoreMs = 0;

void drawStaticBackground() {
  sdk.fillRect(0, 0, SCREEN_W, GROUND_Y, COLOR_SKY);
  sdk.fillRect(0, GROUND_Y, SCREEN_W, GROUND_H, COLOR_GROUND);
  sdk.fillRect(0, GROUND_Y - 2, SCREEN_W, 2, COLOR_GROUND_LINE);
  sdk.fillRect(0, 0, SCREEN_W, HUD_H, COLOR_HUD_BG);
}

void clearRectWithBackground(const Rect& r) {
  if (r.w <= 0 || r.h <= 0) return;

  int x = r.x;
  int y = r.y;
  int w = r.w;
  int h = r.h;

  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > SCREEN_W) w = SCREEN_W - x;
  if (y + h > SCREEN_H) h = SCREEN_H - y;
  if (w <= 0 || h <= 0) return;

  int skyH = 0;
  if (y < GROUND_Y) {
    int skyEnd = y + h;
    if (skyEnd > GROUND_Y) skyEnd = GROUND_Y;
    skyH = skyEnd - y;
  }
  if (skyH > 0) sdk.fillRect(x, y, w, skyH, COLOR_SKY);

  int groundStart = y;
  if (groundStart < GROUND_Y) groundStart = GROUND_Y;
  int groundH = (y + h) - groundStart;
  if (groundH > 0) sdk.fillRect(x, groundStart, w, groundH, COLOR_GROUND);

  if (y <= GROUND_Y - 2 && y + h >= GROUND_Y) {
    sdk.fillRect(x, GROUND_Y - 2, w, 2, COLOR_GROUND_LINE);
  }
}

Rect dinoRectNow() {
  Rect r;
  if (dinoDucking && dinoGrounded) {
    r.w = DINO_DUCK_W;
    r.h = DINO_DUCK_H;
    r.x = DINO_X;
    r.y = GROUND_Y - DINO_DUCK_H;
  } else {
    r.w = DINO_RUN_W;
    r.h = DINO_RUN_H;
    r.x = DINO_X;
    r.y = (int)dinoY;
  }
  return r;
}

Rect dinoHitboxNow() {
  Rect body = dinoRectNow();
  Rect hit;
  if (dinoDucking && dinoGrounded) {
    hit = { body.x + 2, body.y + 5, body.w - 4, body.h - 5 };
  } else {
    hit = { body.x + 3, body.y + 5, body.w - 6, body.h - 7 };
  }
  return hit;
}

Rect obstacleHitbox(const Obstacle& o) {
  if (o.kind == OBS_CACTUS) {
    return { (int)o.x + 3, o.y + 2, o.w - 6, o.h - 3 };
  }
  return { (int)o.x + 4, o.y + 6, o.w - 8, o.h - 10 };
}

bool intersects(const Rect& a, const Rect& b) {
  return (a.x < b.x + b.w) && (a.x + a.w > b.x) && (a.y < b.y + b.h) && (a.y + a.h > b.y);
}

int randRange(int minV, int maxV) {
  return minV + (int)(esp_random() % (uint32_t)(maxV - minV + 1));
}

void scheduleNextSpawn(uint32_t nowMs) {
  int base = 640;
  int speedBonus = (int)((worldSpeed - 210.0f) * 0.9f);
  int minGap = 360;
  int maxGap = 950;
  int gap = base - speedBonus + randRange(-120, 180);
  if (gap < minGap) gap = minGap;
  if (gap > maxGap) gap = maxGap;
  nextSpawnMs = nowMs + (uint32_t)gap;
}

void spawnObstacle() {
  for (int i = 0; i < MAX_OBS; ++i) {
    if (obstacles[i].active) continue;

    Obstacle& o = obstacles[i];
    o.active = true;
    o.x = (float)SCREEN_W;

    int choose = randRange(0, 99);
    bool pteraAllowed = (scoreMs > 5000);
    if (pteraAllowed && choose > 62) {
      o.kind = OBS_PTERA;
      o.w = 28;
      o.h = 18;
      int lane = randRange(0, 2);
      if (lane == 0) o.y = GROUND_Y - 70;
      else if (lane == 1) o.y = GROUND_Y - 48;
      else o.y = GROUND_Y - 30;
    } else {
      o.kind = OBS_CACTUS;
      o.w = randRange(14, 20);
      o.h = randRange(26, 42);
      o.y = GROUND_Y - o.h;
    }

    o.prevRect = { (int)o.x, o.y, o.w, o.h };
    return;
  }
}

void resetRun() {
  dinoY = (float)(GROUND_Y - DINO_RUN_H);
  dinoVy = 0.0f;
  dinoGrounded = true;
  dinoDucking = false;

  worldSpeed = 210.0f;

  lastJumpPressMs = 0;
  lastGroundedMs = millis();
  prevMs = millis();
  scoreMs = 0;

  for (int i = 0; i < MAX_OBS; ++i) {
    obstacles[i].active = false;
    obstacles[i].prevRect = { 0, 0, 0, 0 };
  }

  prevDinoRect = { DINO_X, GROUND_Y - DINO_RUN_H, DINO_RUN_W, DINO_RUN_H };
  scheduleNextSpawn(millis());
}

void drawDino(const Rect& r) {
  sdk.fillRect(r.x, r.y, r.w, r.h, COLOR_DINO);
  if (!dinoDucking || !dinoGrounded) {
    sdk.fillRect(r.x + r.w - 4, r.y + 6, 3, 3, COLOR_SKY); // oeil
  }
}

void drawObstacle(const Obstacle& o) {
  uint16_t c = (o.kind == OBS_CACTUS) ? COLOR_CACTUS : COLOR_PTERA;
  sdk.fillRect((int)o.x, o.y, o.w, o.h, c);
}

void drawHud(bool gameOver) {
  char line[64];
  int score = (int)(scoreMs / 100);
  int best = (int)(bestScoreMs / 100);

  sdk.fillRect(0, 0, SCREEN_W, HUD_H, COLOR_HUD_BG);
  snprintf(line, sizeof(line), "SCORE %d   BEST %d", score, best);
  sdk.drawSmallText(6, 6, line, COLOR_TEXT, COLOR_HUD_BG);

  if (gameOver) {
    sdk.drawSmallText(180, 6, "GAME OVER", SDK_COLOR_ERROR, COLOR_HUD_BG);
  }
}

void updateGameplay(float dt, uint32_t nowMs) {
  dinoDucking = (sdk.isHeld(BTN_DOWN) || (sdk.isTouchHeld() && sdk.touchY() > 150)) && dinoGrounded;

  bool jumpPressed = sdk.isPressed(BTN_A) || sdk.isPressed(BTN_UP) || sdk.isTouchPressed();
  bool jumpReleased = sdk.isReleased(BTN_A) || sdk.isReleased(BTN_UP) || sdk.isTouchReleased();

  if (jumpPressed) {
    lastJumpPressMs = nowMs;
  }

  if (dinoGrounded) {
    lastGroundedMs = nowMs;
  }

  bool canUseBufferedJump = (nowMs - lastJumpPressMs <= jumpBufferMs);
  bool canUseCoyote = (nowMs - lastGroundedMs <= coyoteMs);
  if (canUseBufferedJump && canUseCoyote) {
    dinoVy = jumpVelocity;
    dinoGrounded = false;
    lastJumpPressMs = 0;
    sdk.playBeep(1700, 10);
  }

  if (jumpReleased && dinoVy < -120.0f) {
    dinoVy = -120.0f;
  }

  dinoVy += gravity * dt;
  if (dinoVy > maxFallSpeed) dinoVy = maxFallSpeed;

  dinoY += dinoVy * dt;
  float groundDinoY = (float)(GROUND_Y - DINO_RUN_H);
  if (dinoY >= groundDinoY) {
    dinoY = groundDinoY;
    dinoVy = 0.0f;
    dinoGrounded = true;
  }

  worldSpeed += worldAccel * dt;

  if (nowMs >= nextSpawnMs) {
    spawnObstacle();
    scheduleNextSpawn(nowMs);
  }

  Rect dinoHit = dinoHitboxNow();

  for (int i = 0; i < MAX_OBS; ++i) {
    if (!obstacles[i].active) continue;
    Obstacle& o = obstacles[i];

    o.x -= worldSpeed * dt;

    if (o.x + o.w < 0) {
      o.active = false;
      continue;
    }

    if (intersects(dinoHit, obstacleHitbox(o))) {
      gameState = STATE_GAME_OVER;
      if (scoreMs > bestScoreMs) bestScoreMs = scoreMs;
      sdk.playBeep(220, 120);
      return;
    }
  }

  scoreMs += (uint32_t)(dt * 1000.0f);
}

void renderFrame(bool fullRedraw) {
  if (fullRedraw) {
    drawStaticBackground();
  } else {
    clearRectWithBackground(prevDinoRect);
    for (int i = 0; i < MAX_OBS; ++i) {
      if (obstacles[i].prevRect.w > 0) clearRectWithBackground(obstacles[i].prevRect);
    }
  }

  Rect dinoRect = dinoRectNow();
  drawDino(dinoRect);
  prevDinoRect = dinoRect;

  for (int i = 0; i < MAX_OBS; ++i) {
    Obstacle& o = obstacles[i];
    if (!o.active) {
      o.prevRect = { 0, 0, 0, 0 };
      continue;
    }
    drawObstacle(o);
    o.prevRect = { (int)o.x, o.y, o.w, o.h };
  }

  drawHud(gameState == STATE_GAME_OVER);
}

void drawTitleScreen() {
  drawStaticBackground();
  sdk.drawCenteredText(52, "PARA RUNNER", COLOR_TEXT, COLOR_SKY);
  sdk.drawSmallText(22, 100, "A/UP/TOUCH: SAUT", COLOR_TEXT, COLOR_SKY);
  sdk.drawSmallText(22, 116, "DOWN: SE BAISSER", COLOR_TEXT, COLOR_SKY);
  sdk.drawSmallText(22, 132, "START: MENU", COLOR_TEXT, COLOR_SKY);
  sdk.drawSmallText(22, 156, "Appuie sur A pour jouer", COLOR_TEXT, COLOR_SKY);
}

void setup() {
  sdk.begin();
  randomSeed((uint32_t)esp_random());
  drawTitleScreen();
}

void loop() {
  sdk.updateInputs();

  if (sdk.isPressed(BTN_START)) {
    sdk.requestReturnToLauncher();
  }

  uint32_t nowMs = millis();

  if (gameState == STATE_TITLE) {
    if (sdk.isPressed(BTN_A) || sdk.isPressed(BTN_UP) || sdk.isTouchPressed()) {
      resetRun();
      gameState = STATE_PLAYING;
      renderFrame(true);
    }
    delay(8);
    return;
  }

  float dt = (nowMs - prevMs) * 0.001f;
  prevMs = nowMs;
  if (dt > 0.033f) dt = 0.033f;

  if (gameState == STATE_PLAYING) {
    updateGameplay(dt, nowMs);
    renderFrame(false);
  } else {
    renderFrame(false);
    sdk.drawCenteredText(108, "PERDU", SDK_COLOR_ERROR, COLOR_SKY);
    sdk.drawSmallText(70, 132, "A/TOUCH: REJOUER", COLOR_TEXT, COLOR_SKY);
    if (sdk.isPressed(BTN_A) || sdk.isPressed(BTN_UP) || sdk.isTouchPressed()) {
      resetRun();
      gameState = STATE_PLAYING;
      renderFrame(true);
    }
  }

  delay(16);
}
