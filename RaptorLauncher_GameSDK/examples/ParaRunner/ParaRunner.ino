#include "../../arduino_template/src/raptor_game_sdk.h"

RaptorGameSDK sdk;

namespace {
constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 240;
constexpr int GROUND_Y = 200;
constexpr int GROUND_THICKNESS = 3;

constexpr int DINO_X = 52;
constexpr float GRAVITY = 0.95f;
constexpr float JUMP_VELOCITY = -15.5f;
constexpr float FAST_FALL = 1.45f;

constexpr int DINO_W = 22;
constexpr int DINO_H = 30;
constexpr int DINO_DUCK_H = 18;

constexpr int MAX_OBS = 4;
constexpr int CACTUS_W = 14;
constexpr int CACTUS_H = 28;
constexpr int PTERA_W = 22;
constexpr int PTERA_H = 14;

constexpr uint16_t COL_BG = SDK_COLOR_BG;
constexpr uint16_t COL_GROUND = SDK_COLOR_ACCENT;
constexpr uint16_t COL_DINO = 0xFFFF;
constexpr uint16_t COL_OBS = 0x07E0;
constexpr uint16_t COL_TEXT = SDK_COLOR_TEXT;

struct Obstacle {
  float x = SCREEN_W + 20;
  int y = GROUND_Y - CACTUS_H;
  int w = CACTUS_W;
  int h = CACTUS_H;
  bool flying = false;
  bool active = false;
};

float dinoY = (float)(GROUND_Y - DINO_H);
float dinoVY = 0.0f;
bool ducking = false;
bool alive = true;

Obstacle obstacles[MAX_OBS];

float worldSpeed = 6.4f;
unsigned long score = 0;
unsigned long lastTick = 0;
unsigned long nextSpawnAt = 0;

int prevDinoY = (int)dinoY;
int prevDinoH = DINO_H;

void clearRectSafe(int x, int y, int w, int h) {
  if (w <= 0 || h <= 0) return;
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
  if (w > 0 && h > 0) sdk.fillRect(x, y, w, h, COL_BG);
}

void drawGround() {
  sdk.fillRect(0, GROUND_Y, SCREEN_W, GROUND_THICKNESS, COL_GROUND);
}

void spawnObstacle() {
  for (auto &o : obstacles) {
    if (o.active) continue;
    o.active = true;
    o.x = SCREEN_W + (float)random(0, 40);

    const bool spawnPtera = random(0, 100) < 35;
    o.flying = spawnPtera;
    if (spawnPtera) {
      o.w = PTERA_W;
      o.h = PTERA_H;
      o.y = GROUND_Y - PTERA_H - random(28, 52);
    } else {
      o.w = CACTUS_W + random(0, 8);
      o.h = CACTUS_H + random(0, 6);
      o.y = GROUND_Y - o.h;
    }
    return;
  }
}

bool collide(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
  return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

void resetGame() {
  alive = true;
  ducking = false;
  dinoY = (float)(GROUND_Y - DINO_H);
  dinoVY = 0.0f;
  worldSpeed = 6.4f;
  score = 0;
  nextSpawnAt = millis() + 700;
  for (auto &o : obstacles) o.active = false;
  sdk.clear(COL_BG);
  drawGround();
  sdk.drawSmallText(8, 8, "ParaRunner - A saute | BAS se baisse", COL_TEXT, COL_BG);
}

void setup() {
  sdk.begin();
  randomSeed(micros());
  resetGame();
}

void loop() {
  sdk.updateInputs();

  if (!alive) {
    sdk.drawCenteredText(98, "PERDU", COL_TEXT, COL_BG);
    sdk.drawCenteredText(118, "START pour recommencer", COL_TEXT, COL_BG);
    if (sdk.isPressed(BTN_START)) resetGame();
    if (sdk.isPressed(BTN_SELECT)) sdk.requestReturnToLauncher();
    delay(16);
    return;
  }

  const unsigned long now = millis();
  if (lastTick == 0) lastTick = now;
  float dt = (float)(now - lastTick) / 16.0f;
  if (dt < 0.6f) dt = 0.6f;
  if (dt > 1.7f) dt = 1.7f;
  lastTick = now;

  if ((sdk.isPressed(BTN_A) || sdk.isPressed(BTN_UP)) && dinoY >= (GROUND_Y - DINO_H - 0.1f)) {
    dinoVY = JUMP_VELOCITY;
    sdk.playBeep(1800, 20);
  }

  ducking = sdk.isHeld(BTN_DOWN) && (dinoY >= (GROUND_Y - DINO_H - 0.1f));

  dinoVY += GRAVITY * dt * (ducking ? FAST_FALL : 1.0f);
  dinoY += dinoVY * dt;

  const float floorY = (float)(GROUND_Y - DINO_H);
  if (dinoY > floorY) {
    dinoY = floorY;
    dinoVY = 0.0f;
  }

  if (now >= nextSpawnAt) {
    spawnObstacle();
    nextSpawnAt = now + (unsigned long)random(600, 1200) - (unsigned long)(worldSpeed * 35.0f);
  }

  for (auto &o : obstacles) {
    if (!o.active) continue;
    clearRectSafe((int)o.x - 1, o.y - 1, o.w + 2, o.h + 2);
    o.x -= worldSpeed * dt;
    if (o.x + o.w < 0) {
      o.active = false;
      score += 10;
      continue;
    }
  }

  worldSpeed += 0.0012f * dt;
  if (worldSpeed > 12.0f) worldSpeed = 12.0f;

  const int dinoH = ducking ? DINO_DUCK_H : DINO_H;
  const int dinoTop = (int)dinoY + (DINO_H - dinoH);

  clearRectSafe(DINO_X - 2, prevDinoY - 2, DINO_W + 4, prevDinoH + 4);
  prevDinoY = dinoTop;
  prevDinoH = dinoH;

  for (const auto &o : obstacles) {
    if (!o.active) continue;
    sdk.fillRect((int)o.x, o.y, o.w, o.h, COL_OBS);

    if (collide(DINO_X + 2, dinoTop + 2, DINO_W - 4, dinoH - 2, (int)o.x + 1, o.y + 1, o.w - 2, o.h - 2)) {
      alive = false;
      sdk.playBeep(280, 140);
    }
  }

  sdk.fillRect(DINO_X, dinoTop, DINO_W, dinoH, COL_DINO);
  drawGround();

  char hud[48];
  snprintf(hud, sizeof(hud), "Score:%lu V:%.1f", score, worldSpeed);
  sdk.fillRect(220, 0, 100, 16, COL_BG);
  sdk.drawSmallText(222, 4, hud, COL_TEXT, COL_BG);

  if (sdk.isPressed(BTN_SELECT)) sdk.requestReturnToLauncher();
  delay(16);
}
