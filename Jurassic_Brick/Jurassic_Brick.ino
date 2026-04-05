#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <LovyanGFX.hpp>
#include <XPT2046_Touchscreen.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <math.h>
#include <string.h>

// =====================================================
// Assets
// =====================================================
#include "fond_jeux.h"
#include "fond_jeuxh.h"
#include "fond_jeuxd.h"

#include "platforme.h"
#include "bille.h"
#include "briquebase.h"
#include "briquedur.h"
#include "briquetaper1fois.h"
#include "briquebonus.h"
#include "coeur.h"

#include "brick_breaker_levels.h"
#include "brick_breaker_colors.h"

// Forward declaration for Arduino auto-generated prototypes.
enum SfxId : uint8_t;

// =====================================================
// Pins RaptorLauncher V4
// =====================================================
static constexpr int TFT_SCLK = 14;
static constexpr int TFT_MOSI = 13;
static constexpr int TFT_MISO = 12;
static constexpr int TFT_DC   = 2;
static constexpr int TFT_CS   = 15;
static constexpr int TFT_BL   = 27;

static constexpr int TOUCH_CS  = 33;
static constexpr int TOUCH_IRQ = 36;

static constexpr int SD_CS   = 5;
static constexpr int SD_SCK  = 18;
static constexpr int SD_MISO = 19;
static constexpr int SD_MOSI = 23;

static constexpr int LED_R = 4;
static constexpr int LED_G = 16;
static constexpr int LED_B = 17;

static constexpr int SPEAKER_PIN = 26;

// =====================================================
// Couleurs utiles
// =====================================================
static constexpr uint16_t C_BLACK = 0x0000;
static constexpr uint16_t C_WHITE = 0xFFFF;
static constexpr uint16_t C_RED   = 0xF800;
static constexpr uint16_t C_GREEN = 0x07E0;
static constexpr uint16_t C_BLUE  = 0x001F;

// =====================================================
// Dimensions / HUD
// =====================================================
static constexpr int SCREEN_W = 320;
static constexpr int SCREEN_H = 240;

static constexpr int HUD_LIVES_X1 = 2;
static constexpr int HUD_LIVES_Y1 = 2;
static constexpr int HUD_LIVES_X2 = 90;
static constexpr int HUD_LIVES_Y2 = 24;

static constexpr int HUD_SCORE_X1 = 200;
static constexpr int HUD_SCORE_Y1 = 2;
static constexpr int HUD_SCORE_X2 = 315;
static constexpr int HUD_SCORE_Y2 = 24;

static constexpr int HUD_H = 25;

static constexpr int GRID_X = 26;
static constexpr int GRID_Y = 35;

static constexpr int BRICK_W = 25;
static constexpr int BRICK_H = 10;
static constexpr int BRICK_GAP_X = 2;
static constexpr int BRICK_GAP_Y = 2;

static constexpr int PADDLE_Y = 224;

static constexpr int MAX_HEARTS = 4;
static constexpr int START_HEARTS = 3;

static constexpr uint32_t FRAME_MS = 16;      // ~60 FPS
static constexpr uint32_t LED_FLASH_MS = 35;
static constexpr char SAVE_FILE[] = "/sauv.json";

// =====================================================
// Langues
// =====================================================
enum GameLanguage : uint8_t {
  LANG_FR = 0,
  LANG_EN,
  LANG_DE,
  LANG_IT,
  LANG_ES
};

enum TextId : uint8_t {
  TXT_TITLE = 0,
  TXT_BOOT_LINE1,
  TXT_BOOT_LINE2,
  TXT_SCORE,
  TXT_GAME_OVER,
  TXT_NEW_SCORE,
  TXT_TOP5,
  TXT_ENTER_NAME,
  TXT_LEVEL,
  TXT_TOUCH_TO_CONTINUE,
  TXT_MAX
};

static const char* const TEXTS[TXT_MAX][5] PROGMEM = {
  { "JURASSIC BRICK", "JURASSIC BRICK", "JURASSIC BRICK", "JURASSIC BRICK", "JURASSIC BRICK" },
  { "TOUCHE ZONE SCORE", "TOUCH SCORE ZONE", "BERUHRE SCORE-FELD", "TOCCA ZONA SCORE", "TOCA ZONA SCORE" },
  { "POUR LE LAUNCHER", "FOR LAUNCHER", "FUR LAUNCHER", "PER IL LAUNCHER", "PARA EL LAUNCHER" },
  { "SCORE", "SCORE", "PUNKTE", "PUNTEGGIO", "PUNTOS" },
  { "PARTIE TERMINEE", "GAME OVER", "SPIEL VORBEI", "PARTITA FINITA", "FIN DE PARTIDA" },
  { "NOUVEAU SCORE", "NEW SCORE", "NEUER SCORE", "NUOVO PUNTEGGIO", "NUEVA PUNTUACION" },
  { "TOP 5", "TOP 5", "TOP 5", "TOP 5", "TOP 5" },
  { "ENTRER NOM", "ENTER NAME", "NAME EINGEBEN", "INSERISCI NOME", "INTRODUCIR NOMBRE" },
  { "NIVEAU", "LEVEL", "LEVEL", "LIVELLO", "NIVEL" },
  { "TOUCHE POUR JOUER", "TOUCH TO PLAY", "TIPPE ZUM SPIELEN", "TOCCA PER GIOCARE", "TOCA PARA JUGAR" }
};

static inline const char* txt(TextId id, GameLanguage lang) {
  return (const char*)pgm_read_ptr(&TEXTS[id][lang]);
}

// =====================================================
// Sons placeholder
// =====================================================
enum SfxId : uint8_t {
  SFX_NONE = 0,
  SFX_PADDLE_HIT,
  SFX_WALL_HIT,
  SFX_BRICK_HIT,
  SFX_BRICK_BREAK,
  SFX_BONUS_HEART,
  SFX_LOSE_LIFE,
  SFX_GAME_OVER,
  SFX_LEVEL_CLEAR
};

void playSfx(SfxId id) {
  (void)id;
  // Plus tard : WAV mono
}

// =====================================================
// Réglages hérités du launcher
// =====================================================
struct GameSettings {
  GameLanguage language = LANG_FR;
  uint8_t screenBrightness = 255; // 0..255
  uint8_t ledBrightness = 255;    // 0..255
  uint8_t volume = 100;           // 0..100
  bool soundEnabled = true;
  bool ledEnabled = true;
};

GameSettings gSettings;

// =====================================================
// Affichage LovyanGFX
// =====================================================
class LauncherDisplay : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI _bus;
  lgfx::Light_PWM _light;

public:
  LauncherDisplay() {
    {
      auto cfg = _bus.config();
      cfg.spi_host   = VSPI_HOST;
      cfg.spi_mode   = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read  = 16000000;
      cfg.spi_3wire  = false;
      cfg.use_lock   = true;
      cfg.dma_channel = 1;
      cfg.pin_sclk   = TFT_SCLK;
      cfg.pin_mosi   = TFT_MOSI;
      cfg.pin_miso   = TFT_MISO;
      cfg.pin_dc     = TFT_DC;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }

    {
      auto cfg = _panel.config();
      cfg.pin_cs          = TFT_CS;
      cfg.pin_rst         = -1;
      cfg.pin_busy        = -1;
      cfg.memory_width    = 240;
      cfg.memory_height   = 320;
      cfg.panel_width     = 240;
      cfg.panel_height    = 320;
      cfg.offset_x        = 0;
      cfg.offset_y        = 0;
      cfg.offset_rotation = 0;
      cfg.readable        = true;
      cfg.invert          = true;
      cfg.rgb_order       = false;
      cfg.dlen_16bit      = false;
      cfg.bus_shared      = false;
      _panel.config(cfg);
    }

    {
      auto cfg = _light.config();
      cfg.pin_bl = TFT_BL;
      cfg.invert = false;
      cfg.freq = 12000;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }

    setPanel(&_panel);
  }
};

LauncherDisplay tft;
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);
SPIClass sdSPI(HSPI);
Preferences prefs;

// =====================================================
// Etats
// =====================================================
enum GameState : uint8_t {
  GS_BOOT = 0,
  GS_PLAY,
  GS_GAME_OVER,
  GS_ENTER_SCORE,
  GS_TOP5
};

struct Brick {
  uint8_t type;
  uint8_t hp;
  bool alive;
};

struct ScoreEntry {
  char name[4];
  uint32_t score;
};

Brick gBricks[BB_ROWS][BB_COLS];
ScoreEntry gScores[5] = {
  {"AAA", 0},
  {"BBB", 0},
  {"CCC", 0},
  {"DDD", 0},
  {"EEE", 0}
};

GameState gState = GS_BOOT;

uint8_t gLevel = 0;
int gHearts = START_HEARTS;
uint32_t gScore = 0;

float gBallX = 0.0f;
float gBallY = 0.0f;
float gBallVX = 1.6f;
float gBallVY = -1.6f;

int gPaddleX = 138;
int gOldPaddleX = 138;
int gOldBallX = 0;
int gOldBallY = 0;

bool gNeedFullRedraw = true;
bool gHudDirty = true;

char gNameEntry[4] = {'A','A','A','\0'};
uint8_t gNameIndex = 0;

uint32_t gLastFrameMs = 0;
uint32_t gLedFlashUntil = 0;
bool gLedFlashActive = false;

// =====================================================
// Tactile
// =====================================================
int touch_x_min = 200;
int touch_x_max = 3800;
int touch_y_min = 200;
int touch_y_max = 3800;

int gTouchX = 0;
int gTouchY = 0;
bool gTouchPressed = false;
bool gTouchJustPressed = false;

// =====================================================
// Helpers couleur
// =====================================================
static inline uint8_t r5(uint16_t c) { return (c >> 11) & 0x1F; }
static inline uint8_t g6(uint16_t c) { return (c >> 5) & 0x3F; }
static inline uint8_t b5(uint16_t c) { return c & 0x1F; }

static inline uint16_t rgb565(uint8_t rr, uint8_t gg, uint8_t bb) {
  return ((uint16_t)rr << 11) | ((uint16_t)gg << 5) | bb;
}

uint16_t tint565(uint16_t src, uint16_t tint) {
  uint8_t sr = r5(src);
  uint8_t sg = g6(src);
  uint8_t sb = b5(src);

  // Luminance plus fidèle pour éviter les dérives de teinte
  // (ex: brun qui vire rose / jaune).
  uint8_t sr8 = (uint8_t)((sr * 255) / 31);
  uint8_t sg8 = (uint8_t)((sg * 255) / 63);
  uint8_t sb8 = (uint8_t)((sb * 255) / 31);

  uint8_t lum8 = (uint8_t)((30 * sr8 + 59 * sg8 + 11 * sb8) / 100);

  uint8_t tr = r5(tint);
  uint8_t tg = g6(tint);
  uint8_t tb = b5(tint);

  uint8_t rr = (uint8_t)((tr * lum8) / 255);
  uint8_t gg = (uint8_t)((tg * lum8) / 255);
  uint8_t bb = (uint8_t)((tb * lum8) / 255);

  return rgb565(rr, gg, bb);
}

// =====================================================
// LED ESP32 core 3.x
// =====================================================
uint8_t applyLedBrightness(uint8_t v) {
  return (uint8_t)((uint16_t)v * gSettings.ledBrightness / 255);
}

void ledOff() {
  ledcWrite(LED_R, 255);
  ledcWrite(LED_G, 255);
  ledcWrite(LED_B, 255);
}

void ledSetRgb8(uint8_t r, uint8_t g, uint8_t b) {
  r = applyLedBrightness(r);
  g = applyLedBrightness(g);
  b = applyLedBrightness(b);

  // anode commune
  ledcWrite(LED_R, 255 - r);
  ledcWrite(LED_G, 255 - g);
  ledcWrite(LED_B, 255 - b);
}

void ledFlash565(uint16_t c) {
  if (!gSettings.ledEnabled) return;

  uint8_t rr = (r5(c) * 255) / 31;
  uint8_t gg = (g6(c) * 255) / 63;
  uint8_t bb = (b5(c) * 255) / 31;

  ledSetRgb8(rr, gg, bb);
  gLedFlashActive = true;
  gLedFlashUntil = millis() + LED_FLASH_MS;
}

// =====================================================
// Retour launcher
// =====================================================
void requestLauncherOnNextBoot() {
  // Compat ancienne logique launcher (flag bool).
  prefs.begin("raptor", false);
  prefs.putBool("boot_launcher", true);
  prefs.end();

  // Compat RaptorLauncher V4 OTA: retourne explicitement vers la partition launcher.
  prefs.begin("raptor_boot", false);
  String launcherLabel = prefs.getString("launcher", "");
  prefs.end();

  if (launcherLabel.length() > 0) {
    const esp_partition_t* launcher = esp_partition_find_first(
      ESP_PARTITION_TYPE_APP,
      ESP_PARTITION_SUBTYPE_ANY,
      launcherLabel.c_str()
    );
    if (launcher) {
      esp_ota_set_boot_partition(launcher);
    }
  }
}

void leaveToLauncher() {
  requestLauncherOnNextBoot();
  delay(50);
  ESP.restart();
}

// =====================================================
// Réglages launcher
// =====================================================
GameLanguage parseLanguage(const char* s) {
  if (!s) return LANG_FR;
  if (!strcmp(s, "fr")) return LANG_FR;
  if (!strcmp(s, "en")) return LANG_EN;
  if (!strcmp(s, "de")) return LANG_DE;
  if (!strcmp(s, "it")) return LANG_IT;
  if (!strcmp(s, "es")) return LANG_ES;
  return LANG_FR;
}

void loadLauncherSettings() {
  gSettings.language = LANG_FR;
  gSettings.screenBrightness = 255;
  gSettings.ledBrightness = 255;
  gSettings.volume = 100;
  gSettings.soundEnabled = true;
  gSettings.ledEnabled = true;

  File f = SD.open("/settings.json", FILE_READ);
  if (!f) return;

  JsonDocument doc;
  if (deserializeJson(doc, f)) {
    f.close();
    return;
  }
  f.close();

  gSettings.language = parseLanguage(doc["language"] | "fr");
  gSettings.screenBrightness = doc["screen_brightness"] | 255;
  gSettings.ledBrightness    = doc["led_brightness"] | 255;
  gSettings.volume           = doc["volume"] | 100;
  gSettings.soundEnabled     = doc["sound_enabled"] | true;
  gSettings.ledEnabled       = doc["led_enabled"] | true;

  touch_x_min = doc["touch_x_min"] | 200;
  touch_x_max = doc["touch_x_max"] | 3800;
  touch_y_min = doc["touch_y_min"] | 200;
  touch_y_max = doc["touch_y_max"] | 3800;
}

// =====================================================
// Scores
// =====================================================
void loadScores() {
  File f = SD.open(SAVE_FILE, FILE_READ);
  if (!f) return;

  JsonDocument doc;
  if (deserializeJson(doc, f)) {
    f.close();
    return;
  }
  f.close();

  JsonArray arr = doc["highscores"].as<JsonArray>();
  int i = 0;
  for (JsonObject o : arr) {
    if (i >= 5) break;
    const char* name = o["name"] | "AAA";
    strncpy(gScores[i].name, name, 3);
    gScores[i].name[3] = '\0';
    gScores[i].score = o["score"] | 0;
    i++;
  }
}

void saveScores() {
  JsonDocument doc;
  JsonArray arr = doc["highscores"].to<JsonArray>();

  for (int i = 0; i < 5; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["name"] = gScores[i].name;
    o["score"] = gScores[i].score;
  }

  SD.remove(SAVE_FILE);
  File f = SD.open(SAVE_FILE, FILE_WRITE);
  if (!f) return;

  serializeJsonPretty(doc, f);
  f.close();
}

void insertScore(const char* name, uint32_t score) {
  int pos = -1;
  for (int i = 0; i < 5; i++) {
    if (score > gScores[i].score) {
      pos = i;
      break;
    }
  }
  if (pos < 0) pos = 5;
  if (pos >= 5) return;

  for (int i = 4; i > pos; i--) {
    gScores[i] = gScores[i - 1];
  }

  strncpy(gScores[pos].name, name, 3);
  gScores[pos].name[3] = '\0';
  gScores[pos].score = score;
  saveScores();
}

// =====================================================
// Tactile
// =====================================================
void updateTouch() {
  static bool lastPressed = false;

  if (touch.touched()) {
    TS_Point p = touch.getPoint();
    gTouchX = map(p.x, touch_x_min, touch_x_max, 0, 319);
    gTouchY = map(p.y, touch_y_min, touch_y_max, 239, 0);

    if (gTouchX < 0) gTouchX = 0;
    if (gTouchX > 319) gTouchX = 319;
    if (gTouchY < 0) gTouchY = 0;
    if (gTouchY > 239) gTouchY = 239;

    gTouchPressed = true;
  } else {
    gTouchPressed = false;
  }

  gTouchJustPressed = (gTouchPressed && !lastPressed);
  lastPressed = gTouchPressed;
}

// =====================================================
// Fond courant
// =====================================================
const uint16_t* currentBg() {
  uint8_t bg = bbGetLevelBg(gLevel);
  switch (bg) {
    case BG_FOND_JEUX_H: return fond_jeuxh;
    case BG_FOND_JEUX_D: return fond_jeuxd;
    case BG_FOND_JEUX:
    default: return fond_jeux;
  }
}

void drawFullBackground() {
  const uint16_t* bg = currentBg();
  static uint16_t line[SCREEN_W];

  for (int y = 0; y < SCREEN_H; y++) {
    for (int x = 0; x < SCREEN_W; x++) {
      line[x] = pgm_read_word(&bg[y * SCREEN_W + x]);
    }
    tft.pushImage(0, y, SCREEN_W, 1, line);
  }
}

void restoreBgRect(int x, int y, int w, int h) {
  if (w <= 0 || h <= 0) return;
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > SCREEN_W) w = SCREEN_W - x;
  if (y + h > SCREEN_H) h = SCREEN_H - y;
  if (w <= 0 || h <= 0) return;

  const uint16_t* bg = currentBg();
  static uint16_t line[SCREEN_W];

  for (int yy = 0; yy < h; yy++) {
    int sy = y + yy;
    for (int xx = 0; xx < w; xx++) {
      line[xx] = pgm_read_word(&bg[sy * SCREEN_W + (x + xx)]);
    }
    tft.pushImage(x, sy, w, 1, line);
  }
}

// =====================================================
// Sprites transparents
// Transparence = pixel en haut à droite
// =====================================================
void drawSpriteTransparentTinted(
  int x, int y,
  const uint16_t* data, int w, int h,
  uint16_t tintColor,
  bool useTint
) {
  if (x >= SCREEN_W || y >= SCREEN_H || x + w <= 0 || y + h <= 0) return;

  const uint16_t key = pgm_read_word(&data[w - 1]);

  static uint16_t rowBuffer[64];
  if (w > 64) return;

  for (int yy = 0; yy < h; yy++) {
    int runStart = -1;

    for (int xx = 0; xx <= w; xx++) {
      bool visible = false;
      uint16_t p = 0;

      if (xx < w) {
        p = pgm_read_word(&data[yy * w + xx]);
        visible = (p != key);
      }

      if (visible && runStart < 0) {
        runStart = xx;
      }

      if ((!visible || xx == w) && runStart >= 0) {
        int runEnd = xx - 1;
        int runW = runEnd - runStart + 1;

        for (int i = 0; i < runW; i++) {
          uint16_t src = pgm_read_word(&data[yy * w + runStart + i]);
          rowBuffer[i] = useTint ? tint565(src, tintColor) : src;
        }

        int drawX = x + runStart;
        int drawY = y + yy;
        if (drawX >= 0 && drawX + runW <= SCREEN_W && drawY >= 0 && drawY < SCREEN_H) {
          tft.pushImage(drawX, drawY, runW, 1, rowBuffer);
        }

        runStart = -1;
      }
    }
  }
}

void drawSpriteTransparentRaw(int x, int y, const uint16_t* data, int w, int h) {
  drawSpriteTransparentTinted(x, y, data, w, h, 0, false);
}

// =====================================================
// HUD
// =====================================================
void drawLivesHud() {
  restoreBgRect(HUD_LIVES_X1, HUD_LIVES_Y1,
                HUD_LIVES_X2 - HUD_LIVES_X1 + 1,
                HUD_LIVES_Y2 - HUD_LIVES_Y1 + 1);

  for (int i = 0; i < gHearts; i++) {
    drawSpriteTransparentRaw(2 + i * 22, 2, coeur, coeur_W, coeur_H);
  }
}

void drawScoreHud() {
  restoreBgRect(HUD_SCORE_X1, HUD_SCORE_Y1,
                HUD_SCORE_X2 - HUD_SCORE_X1 + 1,
                HUD_SCORE_Y2 - HUD_SCORE_Y1 + 1);

  // Texte HUD transparent (pas de fond noir) + score plus grand.
  tft.setTextSize(2);
  tft.setTextColor(C_WHITE);

  char scoreBuf[16];
  snprintf(scoreBuf, sizeof(scoreBuf), "%lu", (unsigned long)gScore);

  int textW = tft.textWidth(scoreBuf);
  int x = HUD_SCORE_X2 - textW;
  if (x < HUD_SCORE_X1) x = HUD_SCORE_X1;
  tft.setCursor(x, 5);
  tft.print(scoreBuf);
}

void drawHud() {
  drawLivesHud();
  drawScoreHud();
  gHudDirty = false;
}

// =====================================================
// Briques
// =====================================================
void getBrickRect(uint8_t row, uint8_t col, int& x, int& y, int& w, int& h) {
  x = GRID_X + col * (BRICK_W + BRICK_GAP_X);
  y = GRID_Y + row * (BRICK_H + BRICK_GAP_Y);
  w = BRICK_W;
  h = BRICK_H;
}

void drawBrick(uint8_t row, uint8_t col) {
  Brick& b = gBricks[row][col];
  int x, y, w, h;
  getBrickRect(row, col, x, y, w, h);

  if (!b.alive || b.type == BRICK_EMPTY) {
    restoreBgRect(x, y, w, h);
    return;
  }

  uint16_t tint = bbGetBrickColor(gLevel, row, col);

  if (b.type == BRICK_BASE) {
    drawSpriteTransparentTinted(x, y, briquebase, briquebase_W, briquebase_H, tint, true);
  } else if (b.type == BRICK_BONUS) {
    drawSpriteTransparentTinted(x, y, briquebonus, briquebonus_W, briquebonus_H, tint, true);
  } else if (b.type == BRICK_HARD) {
    if (b.hp >= 3) {
      drawSpriteTransparentTinted(x, y, briquedur, briquedur_W, briquedur_H, tint, true);
    } else if (b.hp == 2) {
      drawSpriteTransparentTinted(x, y, briquetaper1fois, briquetaper1fois_W, briquetaper1fois_H, tint, true);
    } else {
      drawSpriteTransparentTinted(x, y, briquebase, briquebase_W, briquebase_H, tint, true);
    }
  }
}

void drawAllBricks() {
  for (uint8_t r = 0; r < BB_ROWS; r++) {
    for (uint8_t c = 0; c < BB_COLS; c++) {
      drawBrick(r, c);
    }
  }
}

// =====================================================
// Paddle / balle
// =====================================================
void drawPaddle() {
  drawSpriteTransparentTinted(
    gPaddleX, PADDLE_Y,
    platforme, platforme_W, platforme_H,
    bbGetPaddleColor(gLevel), true
  );
}

void drawBall() {
  drawSpriteTransparentTinted(
    (int)gBallX, (int)gBallY,
    bille, bille_W, bille_H,
    bbGetBallColor(gLevel), true
  );
}

float speedMulFromHearts() {
  switch (gHearts) {
    case 4: return 1.2f;
    case 3: return 1.0f;
    case 2: return 0.8f;
    case 1: return 0.5f;
    default: return 1.0f;
  }
}

void resetBallAndPaddle() {
  gPaddleX = (SCREEN_W - platforme_W) / 2;
  gOldPaddleX = gPaddleX;

  gBallX = gPaddleX + (platforme_W / 2) - (bille_W / 2);
  gBallY = PADDLE_Y - bille_H - 2;
  gBallVX = 1.6f;
  gBallVY = -1.6f;

  gOldBallX = (int)gBallX;
  gOldBallY = (int)gBallY;
}

// =====================================================
// Niveau
// =====================================================
void loadLevel(uint8_t levelIndex) {
  gLevel = levelIndex;

  for (uint8_t r = 0; r < BB_ROWS; r++) {
    for (uint8_t c = 0; c < BB_COLS; c++) {
      uint8_t t = bbGetBrickType(levelIndex, r, c);
      gBricks[r][c].type = t;
      gBricks[r][c].alive = (t != BRICK_EMPTY);

      if (t == BRICK_HARD) gBricks[r][c].hp = 3;
      else if (t == BRICK_BASE) gBricks[r][c].hp = 1;
      else if (t == BRICK_BONUS) gBricks[r][c].hp = 1;
      else gBricks[r][c].hp = 0;
    }
  }

  resetBallAndPaddle();
  gNeedFullRedraw = true;
  gHudDirty = true;
}

bool allBricksDestroyed() {
  for (uint8_t r = 0; r < BB_ROWS; r++) {
    for (uint8_t c = 0; c < BB_COLS; c++) {
      if (gBricks[r][c].alive) return false;
    }
  }
  return true;
}

// =====================================================
// Collision
// =====================================================
bool rectHit(float x, float y, int w, int h, int rx, int ry, int rw, int rh) {
  return !(x + w <= rx || x >= rx + rw || y + h <= ry || y >= ry + rh);
}

void onBrickHit(uint8_t row, uint8_t col) {
  Brick& b = gBricks[row][col];
  if (!b.alive) return;

  uint16_t color = bbGetBrickColor(gLevel, row, col);

  if (b.type == BRICK_HARD) {
    if (b.hp > 0) b.hp--;
    if (b.hp == 0) {
      b.alive = false;
      gScore += 30;
      ledFlash565(color);
      playSfx(SFX_BRICK_BREAK);
    } else {
      gScore += 10;
      playSfx(SFX_BRICK_HIT);
    }
  } else if (b.type == BRICK_BONUS) {
    b.alive = false;
    if (gHearts < MAX_HEARTS) gHearts++;
    gScore += 50;
    gHudDirty = true;
    ledFlash565(color);
    playSfx(SFX_BONUS_HEART);
  } else if (b.type == BRICK_BASE) {
    b.alive = false;
    gScore += 20;
    ledFlash565(color);
    playSfx(SFX_BRICK_BREAK);
  }

  drawBrick(row, col);
  gHudDirty = true;
}

void handleBrickCollision() {
  for (uint8_t r = 0; r < BB_ROWS; r++) {
    for (uint8_t c = 0; c < BB_COLS; c++) {
      if (!gBricks[r][c].alive) continue;

      int bx, by, bw, bh;
      getBrickRect(r, c, bx, by, bw, bh);

      if (rectHit(gBallX, gBallY, bille_W, bille_H, bx, by, bw, bh)) {
        float ballCx = gBallX + bille_W * 0.5f;
        float ballCy = gBallY + bille_H * 0.5f;
        float brickCx = bx + bw * 0.5f;
        float brickCy = by + bh * 0.5f;

        float dx = ballCx - brickCx;
        float dy = ballCy - brickCy;

        if (fabsf(dx) > fabsf(dy)) gBallVX = -gBallVX;
        else gBallVY = -gBallVY;

        onBrickHit(r, c);
        return;
      }
    }
  }
}

// =====================================================
// Ecrans
// =====================================================
void drawBootScreen() {
  drawFullBackground();
  drawHud();
  drawAllBricks();
  drawPaddle();
  drawBall();

  tft.fillRect(36, 86, 248, 68, C_BLACK);
  tft.drawRect(36, 86, 248, 68, C_WHITE);
  tft.setTextColor(C_WHITE, C_BLACK);

  tft.setTextSize(2);
  tft.setCursor(62, 98);
  tft.print(txt(TXT_TITLE, gSettings.language));

  tft.setTextSize(1);
  tft.setCursor(64, 126);
  tft.print(txt(TXT_BOOT_LINE1, gSettings.language));
  tft.setCursor(90, 138);
  tft.print(txt(TXT_BOOT_LINE2, gSettings.language));
}

void drawGameOverScreen() {
  drawFullBackground();
  drawHud();

  tft.fillRect(56, 76, 208, 88, C_BLACK);
  tft.drawRect(56, 76, 208, 88, C_WHITE);

  tft.setTextColor(C_RED, C_BLACK);
  tft.setTextSize(2);
  tft.setCursor(82, 94);
  tft.print(txt(TXT_GAME_OVER, gSettings.language));

  tft.setTextSize(1);
  tft.setTextColor(C_WHITE, C_BLACK);
  tft.setCursor(106, 126);
  tft.printf("%s %lu", txt(TXT_SCORE, gSettings.language), (unsigned long)gScore);

  tft.setCursor(84, 144);
  tft.print(txt(TXT_TOUCH_TO_CONTINUE, gSettings.language));
}

void drawEnterScoreScreen() {
  drawFullBackground();

  tft.fillRect(50, 54, 220, 122, C_BLACK);
  tft.drawRect(50, 54, 220, 122, C_WHITE);

  tft.setTextColor(C_WHITE, C_BLACK);
  tft.setTextSize(2);
  tft.setCursor(74, 70);
  tft.print(txt(TXT_NEW_SCORE, gSettings.language));

  tft.setTextSize(1);
  tft.setCursor(92, 100);
  tft.printf("%s %lu", txt(TXT_SCORE, gSettings.language), (unsigned long)gScore);

  tft.setCursor(100, 114);
  tft.print(txt(TXT_ENTER_NAME, gSettings.language));

  tft.setTextSize(3);
  tft.setCursor(110, 136);
  tft.print(gNameEntry);
}

void drawTop5Screen() {
  drawFullBackground();

  tft.fillRect(56, 26, 208, 188, C_BLACK);
  tft.drawRect(56, 26, 208, 188, C_WHITE);

  tft.setTextColor(C_WHITE, C_BLACK);
  tft.setTextSize(2);
  tft.setCursor(122, 38);
  tft.print(txt(TXT_TOP5, gSettings.language));

  tft.setTextSize(1);
  for (int i = 0; i < 5; i++) {
    tft.setCursor(82, 72 + i * 24);
    tft.printf("%d. %s   %lu", i + 1, gScores[i].name, (unsigned long)gScores[i].score);
  }

  tft.setCursor(80, 196);
  tft.print(txt(TXT_BOOT_LINE1, gSettings.language));
}

// =====================================================
// Rendu plein jeu
// =====================================================
void redrawPlayfield() {
  drawFullBackground();
  drawAllBricks();
  drawHud();
  drawPaddle();
  drawBall();
  gNeedFullRedraw = false;
}

// =====================================================
// Jeu
// =====================================================
void loseLife() {
  gHearts--;
  gHudDirty = true;
  playSfx(SFX_LOSE_LIFE);

  if (gHearts <= 0) {
    gHearts = 0;
    gState = GS_GAME_OVER;
    gNeedFullRedraw = true;
    playSfx(SFX_GAME_OVER);
    return;
  }

  resetBallAndPaddle();
  gNeedFullRedraw = true;
}

void updatePaddle() {
  if (!gTouchPressed) return;

  int targetX = gTouchX - (platforme_W / 2);
  if (targetX < 25) targetX = 25;
  if (targetX > SCREEN_W - 25 - platforme_W) targetX = SCREEN_W - 25 - platforme_W;

  if (targetX != gPaddleX) {
    restoreBgRect(gOldPaddleX, PADDLE_Y, platforme_W, platforme_H);
    gPaddleX = targetX;
    drawPaddle();
    gOldPaddleX = gPaddleX;
  }
}

void updateBall() {
  restoreBgRect(gOldBallX, gOldBallY, bille_W, bille_H);

  float mul = speedMulFromHearts();
  gBallX += gBallVX * mul;
  gBallY += gBallVY * mul;

  // murs
  if (gBallX <= 25) {
    gBallX = 25;
    gBallVX = fabsf(gBallVX);
    playSfx(SFX_WALL_HIT);
  }

  if (gBallX + bille_W >= SCREEN_W - 25) {
    gBallX = (SCREEN_W - 25) - bille_W;
    gBallVX = -fabsf(gBallVX);
    playSfx(SFX_WALL_HIT);
  }

  if (gBallY <= 26) {
    gBallY = 26;
    gBallVY = fabsf(gBallVY);
    playSfx(SFX_WALL_HIT);
  }

  // paddle
  if (rectHit(gBallX, gBallY, bille_W, bille_H, gPaddleX, PADDLE_Y, platforme_W, platforme_H) && gBallVY > 0) {
    gBallY = PADDLE_Y - bille_H;
    gBallVY = -fabsf(gBallVY);

    float impact = ((gBallX + bille_W * 0.5f) - gPaddleX) / (float)platforme_W;
    float centered = (impact - 0.5f) * 2.0f;
    gBallVX = centered * 2.2f;

    if (gBallVX > -0.3f && gBallVX < 0.3f) {
      gBallVX = (gBallVX < 0 ? -0.5f : 0.5f);
    }

    playSfx(SFX_PADDLE_HIT);
  }

  handleBrickCollision();

  if (gBallY > SCREEN_H) {
    loseLife();
    return;
  }

  drawBall();
  gOldBallX = (int)gBallX;
  gOldBallY = (int)gBallY;

  if (allBricksDestroyed()) {
    playSfx(SFX_LEVEL_CLEAR);
    if (gLevel < BB_LEVEL_COUNT - 1) {
      gScore += 100;
      loadLevel(gLevel + 1);
    } else {
      gState = GS_ENTER_SCORE;
      gNeedFullRedraw = true;
    }
  }
}

// =====================================================
// Entry score tactile
// Haut moitié : gauche=-1, droite=+1
// Bas moitié : valider lettre
// =====================================================
void updateScoreEntry() {
  if (!gTouchJustPressed) return;

  if (gTouchY < 150) {
    if (gTouchX < 160) {
      if (gNameEntry[gNameIndex] <= 'A') gNameEntry[gNameIndex] = 'Z';
      else gNameEntry[gNameIndex]--;
    } else {
      if (gNameEntry[gNameIndex] >= 'Z') gNameEntry[gNameIndex] = 'A';
      else gNameEntry[gNameIndex]++;
    }
    drawEnterScoreScreen();
    delay(120);
    return;
  }

  gNameIndex++;
  if (gNameIndex >= 3) {
    insertScore(gNameEntry, gScore);
    gState = GS_TOP5;
    gNeedFullRedraw = true;
  } else {
    drawEnterScoreScreen();
  }
  delay(120);
}

// =====================================================
// Setup
// =====================================================
void setup() {
  Serial.begin(115200);

  // Si le jeu démarre directement après un reboot "boot game",
  // on force le prochain boot sur le launcher.
  requestLauncherOnNextBoot();

  tft.init();
  tft.setRotation(3);
  tft.setSwapBytes(false);
  tft.fillScreen(C_BLACK);

  touch.begin();

  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  SD.begin(SD_CS, sdSPI);

  loadLauncherSettings();
  tft.setBrightness(gSettings.screenBrightness);

  ledcAttach(LED_R, 5000, 8);
  ledcAttach(LED_G, 5000, 8);
  ledcAttach(LED_B, 5000, 8);
  ledOff();

  ledcAttach(SPEAKER_PIN, 2000, 8);
  ledcWrite(SPEAKER_PIN, 0);

  loadScores();
  loadLevel(0);

  gState = GS_BOOT;
  drawBootScreen();
}

// =====================================================
// Loop
// =====================================================
void loop() {
  updateTouch();

  if (gLedFlashActive && millis() >= gLedFlashUntil) {
    gLedFlashActive = false;
    ledOff();
  }

  switch (gState) {
    case GS_BOOT:
      if (gTouchJustPressed) {
        // Si on touche la zone score au boot => launcher
        if (gTouchX >= HUD_SCORE_X1 && gTouchX <= HUD_SCORE_X2 &&
            gTouchY >= HUD_SCORE_Y1 && gTouchY <= HUD_SCORE_Y2) {
          leaveToLauncher();
        } else {
          gState = GS_PLAY;
          gNeedFullRedraw = true;
        }
      }
      break;

    case GS_PLAY:
      if (millis() - gLastFrameMs >= FRAME_MS) {
        gLastFrameMs = millis();

        if (gNeedFullRedraw) {
          redrawPlayfield();
        } else {
          updatePaddle();
          updateBall();
          if (gHudDirty) drawHud();
        }
      }
      break;

    case GS_GAME_OVER:
      if (gNeedFullRedraw) {
        drawGameOverScreen();
        gNeedFullRedraw = false;
      }
      if (gTouchJustPressed) {
        gState = GS_ENTER_SCORE;
        gNameEntry[0] = 'A';
        gNameEntry[1] = 'A';
        gNameEntry[2] = 'A';
        gNameEntry[3] = '\0';
        gNameIndex = 0;
        gNeedFullRedraw = true;
      }
      break;

    case GS_ENTER_SCORE:
      if (gNeedFullRedraw) {
        drawEnterScoreScreen();
        gNeedFullRedraw = false;
      }
      updateScoreEntry();
      break;

    case GS_TOP5:
      if (gNeedFullRedraw) {
        drawTop5Screen();
        gNeedFullRedraw = false;
      }
      if (gTouchJustPressed) {
        if (gTouchX >= HUD_SCORE_X1 && gTouchX <= HUD_SCORE_X2 &&
            gTouchY >= HUD_SCORE_Y1 && gTouchY <= HUD_SCORE_Y2) {
          leaveToLauncher();
        } else {
          leaveToLauncher();
        }
      }
      break;
  }
}
