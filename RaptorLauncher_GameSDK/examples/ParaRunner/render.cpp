#include "render.h"
#include "game_state.h"
#include "player.h"
#include "sprites.h"

static AppState lastRenderedState = STATE_MENU;

static int prevPlayerX = PLAYER_X;
static int prevPlayerY = (int)PLAYER_STAND_Y;
static int prevPlayerW = 44;
static int prevPlayerH = 45;
static bool prevPlayerVisible = false;

static int prevObsX[MAX_OBSTACLES] = {0};
static int prevObsY[MAX_OBSTACLES] = {0};
static int prevObsW[MAX_OBSTACLES] = {0};
static int prevObsH[MAX_OBSTACLES] = {0};
static bool prevObsActive[MAX_OBSTACLES] = {false};
static uint32_t prevHudScore = 0xFFFFFFFFu;
static uint8_t prevHudLives = 255;

bool pointInRect(int px, int py, const ButtonRect& r) {
  return px >= r.x && px < (r.x + r.w) && py >= r.y && py < (r.y + r.h);
}

void drawSpriteKey(int x, int y, const SpriteFrame& s) {
  const uint16_t key = pgm_read_word(&s.pixels[s.w - 1]); // pixel haut-droite = transparence
  for (int yy = 0; yy < s.h; yy++) {
    for (int xx = 0; xx < s.w; xx++) {
      uint16_t c = pgm_read_word(&s.pixels[yy * s.w + xx]);
      if (c != key) {
        sdk.fillRect(x + xx, y + yy, 1, 1, c);
      }
    }
  }
}

void drawSpriteKeyFlipH(int x, int y, const SpriteFrame& s) {
  const uint16_t key = pgm_read_word(&s.pixels[s.w - 1]); // pixel haut-droite = transparence
  for (int yy = 0; yy < s.h; yy++) {
    for (int xx = 0; xx < s.w; xx++) {
      uint16_t c = pgm_read_word(&s.pixels[yy * s.w + xx]);
      if (c != key) {
        sdk.fillRect(x + (s.w - 1 - xx), y + yy, 1, 1, c);
      }
    }
  }
}

void drawFilledCircle(int cx, int cy, int r, uint16_t col) {
  for (int y = -r; y <= r; y++) {
    for (int x = -r; x <= r; x++) {
      if ((x * x + y * y) <= (r * r)) {
        sdk.fillRect(cx + x, cy + y, 1, 1, col);
      }
    }
  }
}

void drawButton(const ButtonRect& r, uint16_t fill, uint16_t border, const char* txt, uint16_t fg, bool selected) {
  uint16_t realBorder = selected ? COL_YELLOW : border;
  sdk.fillRect(r.x, r.y, r.w, r.h, fill);
  sdk.drawRect(r.x, r.y, r.w, r.h, realBorder);
  sdk.drawRect(r.x + 1, r.y + 1, r.w - 2, r.h - 2, realBorder);
  if (selected) {
    sdk.drawRect(r.x - 1, r.y - 1, r.w + 2, r.h + 2, COL_YELLOW);
  }
  sdk.drawSmallText(r.x + 10, r.y + (r.h / 2) - 4, txt, fg, fill);
}

static void drawBackgroundFull() {
  sdk.clear(COL_SKY_1);
  sdk.fillRect(0, 88, SCREEN_W, 50, COL_SKY_2);

  drawFilledCircle(265, 34, 16, COL_SUN);
  drawFilledCircle(265, 34, 10, COL_SUN_2);

  sdk.fillRect(0, GROUND_TOP_Y, SCREEN_W, 4, COL_GROUND_2);
  sdk.fillRect(0, GROUND_FILL_Y, SCREEN_W, SCREEN_H - GROUND_FILL_Y, COL_GROUND);
}

static void redrawBackgroundRect(int x, int y, int w, int h) {
  if (w <= 0 || h <= 0) return;
  if (x >= SCREEN_W || y >= SCREEN_H) return;
  if (x + w <= 0 || y + h <= 0) return;

  int rx = x;
  int ry = y;
  int rw = w;
  int rh = h;

  if (rx < 0) { rw += rx; rx = 0; }
  if (ry < 0) { rh += ry; ry = 0; }
  if (rx + rw > SCREEN_W) rw = SCREEN_W - rx;
  if (ry + rh > SCREEN_H) rh = SCREEN_H - ry;
  if (rw <= 0 || rh <= 0) return;

  for (int yy = ry; yy < ry + rh; yy++) {
    uint16_t col = COL_SKY_1;
    if (yy >= 88 && yy < 138) col = COL_SKY_2;
    if (yy >= GROUND_TOP_Y && yy < GROUND_TOP_Y + 4) col = COL_GROUND_2;
    if (yy >= GROUND_FILL_Y) col = COL_GROUND;
    sdk.fillRect(rx, yy, rw, 1, col);
  }

  // si la zone recoupe le soleil, on le redessine
  if (!(rx > 281 || ry > 50 || rx + rw < 249 || ry + rh < 18)) {
    drawFilledCircle(265, 34, 16, COL_SUN);
    drawFilledCircle(265, 34, 10, COL_SUN_2);
  }
}

static void drawHearts() {
  int startX = SCREEN_W - 8 - (MAX_LIVES * 22);
  int y = 2;
  for (int i = 0; i < lives; i++) {
    drawSpriteKey(startX + i * 22, y, SPR_HEART);
  }
}

static void drawHud() {
  sdk.fillRect(0, 0, SCREEN_W, 24, COL_PANEL);

  char buf[32];
  snprintf(buf, sizeof(buf), "Score:%lu", (unsigned long)score);
  sdk.drawSmallText(8, 8, buf, COL_WHITE, COL_PANEL);

  snprintf(buf, sizeof(buf), "Best:%lu", (unsigned long)bestScore);
  sdk.drawSmallText(112, 8, buf, COL_YELLOW, COL_PANEL);

  drawHearts();
}

static void drawTouchZones() {
  sdk.fillRect(0, 204, 160, 36, COL_BLUE);
  sdk.fillRect(160, 204, 160, 36, COL_PURPLE);

  sdk.drawRect(0, 204, 160, 36, COL_WHITE);
  sdk.drawRect(160, 204, 160, 36, COL_WHITE);

  sdk.drawSmallText(52, 218, "SAUT", COL_WHITE, COL_BLUE);
  sdk.drawSmallText(196, 218, "BAISSE", COL_WHITE, COL_PURPLE);
}

static void drawPlayerCurrent() {
  const SpriteFrame& s = currentPlayerSprite();

  if (millis() < invulnUntil) {
    if (((millis() / 80) % 2) == 0) {
      prevPlayerVisible = false;
      prevPlayerW = s.w;
      prevPlayerH = s.h;
      prevPlayerX = PLAYER_X;
      prevPlayerY = (int)playerY;
      return;
    }
  }

  drawSpriteKeyFlipH(PLAYER_X, (int)playerY, s);

  prevPlayerVisible = true;
  prevPlayerW = s.w;
  prevPlayerH = s.h;
  prevPlayerX = PLAYER_X;
  prevPlayerY = (int)playerY;
}

static void erasePreviousMovingObjects() {
  if (prevPlayerVisible) {
    redrawBackgroundRect(prevPlayerX - 2, prevPlayerY - 2, prevPlayerW + 4, prevPlayerH + 4);
  }

  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (prevObsActive[i]) {
      redrawBackgroundRect(prevObsX[i] - 2, prevObsY[i] - 2, prevObsW[i] + 4, prevObsH[i] + 4);
    }
  }
}

static void drawObstaclesCurrent() {
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    prevObsActive[i] = false;

    if (!obstacles[i].active) continue;

    if (obstacles[i].type == OBS_CACTUS) {
      if (obstacles[i].variant == 0)      drawSpriteKey((int)obstacles[i].x, (int)obstacles[i].y, SPR_CACTUS_1);
      else if (obstacles[i].variant == 1) drawSpriteKey((int)obstacles[i].x, (int)obstacles[i].y, SPR_CACTUS_2);
      else                                drawSpriteKey((int)obstacles[i].x, (int)obstacles[i].y, SPR_CACTUS_3);
    } else {
      if (animToggle) drawSpriteKey((int)obstacles[i].x, (int)obstacles[i].y, SPR_PTERA_1);
      else            drawSpriteKey((int)obstacles[i].x, (int)obstacles[i].y, SPR_PTERA_2);
    }

    prevObsX[i] = (int)obstacles[i].x;
    prevObsY[i] = (int)obstacles[i].y;
    prevObsW[i] = obstacles[i].w;
    prevObsH[i] = obstacles[i].h;
    prevObsActive[i] = true;
  }
}

static void drawGameFullOnce() {
  drawBackgroundFull();
  drawHud();
  prevHudScore = score;
  prevHudLives = lives;
  drawTouchZones();
  drawObstaclesCurrent();
  drawPlayerCurrent();
}

void drawMenuScreen() {
  lastRenderedState = STATE_MENU;
  sdk.beginBatch();

  drawBackgroundFull();

  sdk.fillRect(0, 0, SCREEN_W, 28, COL_PANEL);
  sdk.drawCenteredText(8, "PARARUNNER", COL_YELLOW, COL_PANEL);

  drawSpriteKeyFlipH(28, 138, SPR_RUN_1);
  drawSpriteKey(250, 136, SPR_CACTUS_2);

  sdk.drawCenteredText(54, "Parasaurolophus Runner", COL_BLACK, COL_SKY_1);

  drawButton(btnPlay,   COL_GREEN, COL_WHITE, "PLAY", COL_BLACK, menuSelection == MENU_PLAY);
  drawButton(btnLaunch, COL_RED,   COL_WHITE, "Launcher", COL_WHITE, menuSelection == MENU_LAUNCHER);

  sdk.drawCenteredText(210, "Tactile ou boutons", COL_BLACK, COL_SKY_1);
  sdk.drawCenteredText(224, "A/Haut=saut  Bas=baisse", COL_BLACK, COL_SKY_1);
  sdk.endBatch();
}

void drawGameScreen() {
  sdk.beginBatch();
  if (lastRenderedState != STATE_PLAYING) {
    drawGameFullOnce();
    lastRenderedState = STATE_PLAYING;
    sdk.endBatch();
    return;
  }

  erasePreviousMovingObjects();
  drawObstaclesCurrent();
  drawPlayerCurrent();
  if (score != prevHudScore || lives != prevHudLives) {
    drawHud();
    prevHudScore = score;
    prevHudLives = lives;
  }
  sdk.endBatch();
}

void drawGameOverScreen() {
  lastRenderedState = STATE_GAME_OVER;
  sdk.beginBatch();

  drawBackgroundFull();

  // on redessine une vraie frame jeu figée derrière
  drawObstaclesCurrent();
  drawPlayerCurrent();
  drawHud();
  drawTouchZones();

  sdk.fillRect(34, 48, 252, 118, COL_PANEL);
  sdk.drawRect(34, 48, 252, 118, COL_WHITE);
  sdk.drawRect(35, 49, 250, 116, COL_WHITE);

  sdk.drawCenteredText(62, "GAME OVER", COL_RED, COL_PANEL);

  char a[32];
  char b[32];
  snprintf(a, sizeof(a), "Score : %lu", (unsigned long)score);
  snprintf(b, sizeof(b), "Best  : %lu", (unsigned long)bestScore);

  sdk.drawCenteredText(90, a, COL_WHITE, COL_PANEL);
  sdk.drawCenteredText(108, b, COL_YELLOW, COL_PANEL);

  drawButton(btnRetry, COL_GREEN,  COL_WHITE, "REJOUER", COL_BLACK, gameOverSelection == GAMEOVER_RETRY);
  drawButton(btnMenu,  COL_ORANGE, COL_WHITE, "MENU",    COL_BLACK, gameOverSelection == GAMEOVER_MENU);
  sdk.endBatch();
}
