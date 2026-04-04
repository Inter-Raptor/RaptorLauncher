#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <XPT2046_Touchscreen.h>
#include <LovyanGFX.hpp>
#include <esp_ota_ops.h>
#include <esp_partition.h>

#include "dino1.h"
#include "dino2.h"
#include "dino3.h"
#include "dino4.h"
#include "dino5.h"
#include "dino6.h"
#include "pistolet.h"
#include "sang1.h"
#include "sang2.h"
#include "sang3.h"

class LGFX_2432S028 : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel_instance;
  lgfx::Bus_SPI _bus_instance;
  lgfx::Light_PWM _light_instance;

public:
  LGFX_2432S028(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = VSPI_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = 1;
      cfg.pin_sclk = 14;
      cfg.pin_mosi = 13;
      cfg.pin_miso = 12;
      cfg.pin_dc   = 2;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs = 15;
      cfg.pin_rst = -1;
      cfg.pin_busy = -1;
      cfg.memory_width = 240;
      cfg.memory_height = 320;
      cfg.panel_width = 240;
      cfg.panel_height = 320;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.readable = true;
      cfg.invert = true;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;
      _panel_instance.config(cfg);
    }

    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = 27;
      cfg.invert = false;
      cfg.freq = 12000;
      cfg.pwm_channel = 7;
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    setPanel(&_panel_instance);
  }
};

static LGFX_2432S028 lcd;
static XPT2046_Touchscreen touch(33, 36);
static SPIClass sdSPI(HSPI);

static const int SD_CS   = 5;
static const int SD_SCK  = 18;
static const int SD_MISO = 19;
static const int SD_MOSI = 23;

// Alignement avec RaptorLauncher (led_manager): LED RGB commune anode
static const int PIN_LED_R = 4;
static const int PIN_LED_G = 16;
static const int PIN_LED_B = 17;
static const bool RGB_COMMON_ANODE = true;

static const int PIN_AUDIO = 26;

static const char* SAVE_PATH = "/games/DinoBlaster/sauv.json";
static const char* WAV_PATH  = "/games/DinoBlaster/gun.wav";
static const char* SETTINGS_PATH = "/settings.json";

static const int SCREEN_W = 320;
static const int SCREEN_H = 240;
static const int HUD_H = 24;
static const int GROUND_H = 0;
static const int PLAY_Y = HUD_H + 2;

static const unsigned long BLOOD_MS = 300;
static const unsigned long MUZZLE_MS = 60;
static const unsigned long LED_FLASH_MS = 100;
static const unsigned long DINO_MOVE_MS = 120;
static const int DINO_SPEED_PX = 1;

static const int MAX_DINOS = 2;
static const unsigned long BONUS_MIN_INTERVAL_MS = 6000;
static const unsigned long BONUS_MAX_INTERVAL_MS = 10000;
static const unsigned long BONUS_MIN_LIFE_MS = 2000;
static const unsigned long BONUS_MAX_LIFE_MS = 3000;
static const int BONUS_TIME_SECONDS = 3;
static const int BONUS_SIZE = 26;

static const int NO_SPAWN_W = 120;
static const int NO_SPAWN_H = 120;
static const int NO_SPAWN_X = SCREEN_W - NO_SPAWN_W;
static const int NO_SPAWN_Y = SCREEN_H - NO_SPAWN_H;

int touch_x_min = 317;
int touch_x_max = 3870;
int touch_y_min = 258;
int touch_y_max = 3760;
int touch_offset_x = 0;
int touch_offset_y = -2;

enum Lang {
  LANG_EN = 0,
  LANG_FR = 1,
  LANG_DE = 2,
  LANG_IT = 3,
  LANG_ES = 4
};

int currentLanguage = LANG_FR;

struct TouchState {
  bool down = false;
  bool pressed = false;
  int x = 0;
  int y = 0;
} ts;

struct DinoSpriteInfo {
  const uint16_t* data;
  uint16_t w;
  uint16_t h;
  uint16_t key;
};

const DinoSpriteInfo DINO_SPRITES[] = {
  { dino1, dino1_W, dino1_H, dino1_KEY },
  { dino2, dino2_W, dino2_H, dino2_KEY },
  { dino3, dino3_W, dino3_H, dino3_KEY },
  { dino4, dino4_W, dino4_H, dino4_KEY },
  { dino5, dino5_W, dino5_H, dino5_KEY },
  { dino6, dino6_W, dino6_H, dino6_KEY }
};

struct BloodSpriteInfo {
  const uint16_t* data;
  uint16_t w;
  uint16_t h;
  uint16_t key;
};

const BloodSpriteInfo BLOOD_SPRITES[] = {
  { sang1, sang1_W, sang1_H, sang1_KEY },
  { sang2, sang2_W, sang2_H, sang2_KEY },
  { sang3, sang3_W, sang3_H, sang3_KEY }
};

struct HighScore {
  char name[4];
  int score;
};

HighScore highs[5];

struct DinoTarget {
  int x = 0;
  int y = 0;
  int spriteIndex = 0;
  bool flipX = false;
  bool alive = false;
  unsigned long respawnAt = 0;
  int vx = -DINO_SPEED_PX;
};

DinoTarget dinos[MAX_DINOS];

struct BloodFX {
  bool active = false;
  int x = 0;
  int y = 0;
  int spriteIndex = 0;
  unsigned long until = 0;
} blood;

struct TimeBonus {
  bool active = false;
  int x = 0;
  int y = 0;
  unsigned long until = 0;
  unsigned long nextSpawnAt = 0;
} bonus;

enum GameState {
  GS_TITLE,
  GS_PLAY,
  GS_ENTER_NAME,
  GS_GAMEOVER
};

GameState state = GS_TITLE;

int score = 0;
int combo = 0;
int misses = 0;
int lives = 5;
int timeLeft = 45;
int bestIndexCandidate = -1;

char initials[4] = {'A','A','A','\0'};
int initialsIndex = 0;

bool sdOk = false;
bool wavOk = false;

unsigned long lastFrame = 0;
unsigned long lastSecond = 0;
unsigned long lastLog = 0;
unsigned long lastDinoMove = 0;
unsigned long ledFlashUntil = 0;
unsigned long muzzleUntil = 0;

bool needFullRedraw = true;
bool hudDirty = true;

bool shotVisualActive = false;

int randRange(int a, int b) {
  return a + (esp_random() % (b - a + 1));
}

bool rectsOverlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
  return !(ax + aw <= bx || bx + bw <= ax || ay + ah <= by || by + bh <= ay);
}

void ledOff() {
  if (RGB_COMMON_ANODE) {
    digitalWrite(PIN_LED_R, HIGH);
    digitalWrite(PIN_LED_G, HIGH);
    digitalWrite(PIN_LED_B, HIGH);
  } else {
    digitalWrite(PIN_LED_R, LOW);
    digitalWrite(PIN_LED_G, LOW);
    digitalWrite(PIN_LED_B, LOW);
  }
}

void ledRedOn() {
  if (RGB_COMMON_ANODE) {
    digitalWrite(PIN_LED_R, LOW);
    digitalWrite(PIN_LED_G, HIGH);
    digitalWrite(PIN_LED_B, HIGH);
  } else {
    digitalWrite(PIN_LED_R, HIGH);
    digitalWrite(PIN_LED_G, LOW);
    digitalWrite(PIN_LED_B, LOW);
  }
}

void ledFlashRed(unsigned long ms) {
  ledRedOn();
  ledFlashUntil = millis() + ms;
}

void ledUpdate() {
  if (ledFlashUntil && millis() > ledFlashUntil) {
    ledFlashUntil = 0;
    ledOff();
  }
}

void prepareReturnToLauncher() {
  const esp_partition_t* launcher = esp_partition_find_first(
    ESP_PARTITION_TYPE_APP,
    ESP_PARTITION_SUBTYPE_APP_OTA_0,
    nullptr
  );

  if (launcher) {
    esp_err_t err = esp_ota_set_boot_partition(launcher);
    Serial.printf("[BOOT] retour launcher programme sur app0, err=%d\n", err);
  } else {
    Serial.println("[BOOT] app0 introuvable");
  }
}

bool sdBegin() {
  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  sdOk = SD.begin(SD_CS, sdSPI);
  Serial.printf("[SD] %s\n", sdOk ? "OK" : "ECHEC");
  return sdOk;
}

bool loadSettings() {
  if (!sdOk || !SD.exists(SETTINGS_PATH)) return false;

  File f = SD.open(SETTINGS_PATH, FILE_READ);
  if (!f) return false;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  touch_x_min = doc["touch_x_min"] | touch_x_min;
  touch_x_max = doc["touch_x_max"] | touch_x_max;
  touch_y_min = doc["touch_y_min"] | touch_y_min;
  touch_y_max = doc["touch_y_max"] | touch_y_max;
  touch_offset_x = doc["touch_offset_x"] | touch_offset_x;
  touch_offset_y = doc["touch_offset_y"] | touch_offset_y;
  currentLanguage = doc["language"] | currentLanguage;

  Serial.println("[SETTINGS] calibration tactile chargee");
  return true;
}

void initHighs() {
  const char* defaults[5] = {"AAA", "BBB", "CCC", "DDD", "EEE"};
  for (int i = 0; i < 5; i++) {
    strncpy(highs[i].name, defaults[i], 4);
    highs[i].name[3] = '\0';
    highs[i].score = (5 - i) * 10;
  }
}

void sortHighs() {
  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 5; j++) {
      if (highs[j].score > highs[i].score) {
        HighScore t = highs[i];
        highs[i] = highs[j];
        highs[j] = t;
      }
    }
  }
}

void loadHighs() {
  initHighs();
  if (!sdOk || !SD.exists(SAVE_PATH)) return;

  File f = SD.open(SAVE_PATH, FILE_READ);
  if (!f) return;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return;

  JsonArray arr = doc["scores"].as<JsonArray>();
  if (arr.isNull()) return;

  int i = 0;
  for (JsonObject obj : arr) {
    if (i >= 5) break;
    const char* n = obj["name"] | "AAA";
    strncpy(highs[i].name, n, 4);
    highs[i].name[3] = '\0';
    highs[i].score = obj["score"] | 0;
    i++;
  }

  sortHighs();
  Serial.println("[SAVE] highs charges");
}

void saveHighs() {
  if (!sdOk) return;

  JsonDocument doc;
  JsonArray arr = doc["scores"].to<JsonArray>();

  for (int i = 0; i < 5; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["name"] = highs[i].name;
    o["score"] = highs[i].score;
  }

  SD.remove(SAVE_PATH);

  File f = SD.open(SAVE_PATH, FILE_WRITE);
  if (!f) {
    Serial.println("[SAVE] ouverture echec");
    return;
  }

  serializeJsonPretty(doc, f);
  f.close();
  Serial.println("[SAVE] highs sauves");
}

int findHighScorePosition(int newScore) {
  for (int i = 0; i < 5; i++) {
    if (newScore > highs[i].score) return i;
  }
  return -1;
}

void insertHighScore(int pos, const char* name, int newScore) {
  for (int i = 4; i > pos; i--) highs[i] = highs[i - 1];
  strncpy(highs[pos].name, name, 4);
  highs[pos].name[3] = '\0';
  highs[pos].score = newScore;
  saveHighs();
}

void playGunWavShort() {
  if (!sdOk) return;

  File f = SD.open(WAV_PATH, FILE_READ);
  if (!f) return;

  uint8_t header[44];
  if (f.read(header, 44) != 44) {
    f.close();
    return;
  }

  const int samplesToPlay = 4000;
  for (int i = 0; i < samplesToPlay && f.available(); i++) {
    uint8_t s = f.read();
    dacWrite(PIN_AUDIO, s);
    delayMicroseconds(23);
  }

  dacWrite(PIN_AUDIO, 128);
  f.close();
}

void beepFallback(uint16_t freq, uint16_t ms) {
  bool ok = ledcAttach(PIN_AUDIO, freq, 8);
  if (!ok) return;
  ledcWriteTone(PIN_AUDIO, freq);
  delay(ms);
  ledcWriteTone(PIN_AUDIO, 0);
}

void fireSound() {
  if (wavOk) playGunWavShort();
  else beepFallback(1800, 25);
}

void updateTouch() {
  ts.pressed = false;

  if (touch.touched()) {
    TS_Point p = touch.getPoint();

    int x = map(p.x, touch_x_min, touch_x_max, 0, SCREEN_W - 1);
    int y = map(p.y, touch_y_min, touch_y_max, SCREEN_H - 1, 0);
    x += touch_offset_x;
    y += touch_offset_y;

    if (x < 0) x = 0;
    if (x >= SCREEN_W) x = SCREEN_W - 1;
    if (y < 0) y = 0;
    if (y >= SCREEN_H) y = SCREEN_H - 1;

    ts.x = x;
    ts.y = y;

    if (!ts.down) ts.pressed = true;
    ts.down = true;
  } else {
    ts.down = false;
  }
}

void clearRectSafe(int x, int y, int w, int h) {
  if (w <= 0 || h <= 0) return;
  if (x >= SCREEN_W || y >= SCREEN_H) return;
  if (x + w < 0 || y + h < 0) return;

  int rx = x;
  int ry = y;
  int rw = w;
  int rh = h;

  if (rx < 0) { rw += rx; rx = 0; }
  if (ry < 0) { rh += ry; ry = 0; }
  if (rx + rw > SCREEN_W) rw = SCREEN_W - rx;
  if (ry + rh > SCREEN_H) rh = SCREEN_H - ry;
  if (rw <= 0 || rh <= 0) return;

  lcd.fillRect(rx, ry, rw, rh, TFT_BLACK);

  if (ry + rh >= SCREEN_H - GROUND_H) {
    int gy = max(ry, SCREEN_H - GROUND_H);
    int gh = min(ry + rh, SCREEN_H) - gy;
    if (gh > 0) lcd.fillRect(rx, gy, rw, gh, TFT_DARKGREEN);
  }
}

void drawRGB565Transparent(int x, int y, const uint16_t* data, int w, int h, uint16_t key, bool flipX = false) {
  (void)key;
  // Convention sprites: le pixel (0,0) definit toujours la couleur transparente.
  const uint16_t transparentKey = pgm_read_word(&data[0]);
  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      int sx = flipX ? (w - 1 - xx) : xx;
      uint16_t c = pgm_read_word(&data[yy * w + sx]);
      if (c == transparentKey) continue;
      lcd.drawPixel(x + xx, y + yy, c);
    }
  }
}

void drawHUD() {
  lcd.fillRect(0, 0, SCREEN_W, HUD_H, TFT_DARKGREY);
  lcd.setTextColor(TFT_WHITE, TFT_DARKGREY);
  lcd.setTextSize(1);
  lcd.setCursor(4, 8);   lcd.printf("Score:%d", score);
  lcd.setCursor(92, 8);  lcd.printf("Combo:%d", combo);
  lcd.setCursor(180, 8); lcd.printf("Vies:%d", lives);
  lcd.setCursor(252, 8); lcd.printf("T:%d", timeLeft);
}

void drawGround() {
  if (GROUND_H <= 0) return;
  lcd.fillRect(0, SCREEN_H - GROUND_H, SCREEN_W, GROUND_H, TFT_DARKGREEN);
}

void getIntroText(const char*& l1, const char*& l2, const char*& l3, const char*& l4, const char*& startText) {
  switch (currentLanguage) {
    case LANG_FR:
      l1 = "Une faille temporelle s'est ouverte.";
      l2 = "Des dinos surgissent dans notre epoque.";
      l3 = "Elimine-les avant la fin du chrono.";
      l4 = "Chaque tir rate te rapproche du desastre.";
      startText = "Touchez l'ecran pour commencer";
      break;

    case LANG_EN:
      l1 = "A time rift has opened.";
      l2 = "Dinosaurs are breaking into our era.";
      l3 = "Take them down before time runs out.";
      l4 = "Every missed shot brings disaster closer.";
      startText = "Touch the screen to start";
      break;

    case LANG_DE:
      l1 = "Ein Zeitriss hat sich geoeffnet.";
      l2 = "Dinosaurier dringen in unsere Zeit ein.";
      l3 = "Schalte sie aus, bevor die Zeit ablaeuft.";
      l4 = "Jeder Fehlschuss bringt das Unheil naeher.";
      startText = "Bildschirm beruehren zum Start";
      break;

    case LANG_IT:
      l1 = "Si e aperta una frattura nel tempo.";
      l2 = "I dinosauri stanno invadendo la nostra epoca.";
      l3 = "Eliminali prima che il tempo finisca.";
      l4 = "Ogni colpo mancato avvicina il disastro.";
      startText = "Tocca lo schermo per iniziare";
      break;

    case LANG_ES:
      l1 = "Se ha abierto una grieta temporal.";
      l2 = "Los dinosaurios invaden nuestra epoca.";
      l3 = "Eliminalos antes de que se acabe el tiempo.";
      l4 = "Cada disparo fallado acerca el desastre.";
      startText = "Toca la pantalla para empezar";
      break;

    default:
      l1 = "A time rift has opened.";
      l2 = "Dinosaurs are breaking into our era.";
      l3 = "Take them down before time runs out.";
      l4 = "Every missed shot brings disaster closer.";
      startText = "Touch the screen to start";
      break;
  }
}

void eraseGunZone() {
  int gunX = SCREEN_W - pistolet_W - 6;
  int gunY = SCREEN_H - pistolet_H + 6;
  clearRectSafe(gunX - 28, gunY - 10, pistolet_W + 44, pistolet_H + 32);
}

void drawGun(bool shooting = false) {
  int baseX = SCREEN_W - pistolet_W - 6;
  int baseY = SCREEN_H - pistolet_H + 6;
  int kick = shooting ? 6 : 0;

  drawRGB565Transparent(baseX + kick, baseY + kick, pistolet, pistolet_W, pistolet_H, pistolet_KEY, false);

  if (shooting) {
    int mx = baseX + 8;
    int my = baseY + 15;
    lcd.fillCircle(mx, my, 8, TFT_YELLOW);
    lcd.fillCircle(mx - 10, my, 4, TFT_ORANGE);
  }
}

void clearGunShotEffect() {
  eraseGunZone();
  drawGun(false);
}

void spawnDino(int idx) {
  DinoTarget& dino = dinos[idx];
  dino.spriteIndex = randRange(0, 5);
  dino.flipX = false; // Tous les sprites regardent a gauche
  dino.vx = -DINO_SPEED_PX;

  const DinoSpriteInfo& sp = DINO_SPRITES[dino.spriteIndex];

  for (int tries = 0; tries < 100; tries++) {
    int x = randRange(0, SCREEN_W - sp.w - 1);
    int y = randRange(PLAY_Y, SCREEN_H - GROUND_H - sp.h - 1);

    if (!rectsOverlap(x, y, sp.w, sp.h, NO_SPAWN_X, NO_SPAWN_Y, NO_SPAWN_W, NO_SPAWN_H)) {
      dino.x = x;
      dino.y = y;
      dino.alive = true;
      dino.respawnAt = 0;
      Serial.printf("[DINO] spawn sprite=%d flip=%d x=%d y=%d\n", dino.spriteIndex, dino.flipX ? 1 : 0, dino.x, dino.y);
      return;
    }
  }

  dino.x = 130;
  dino.y = PLAY_Y + 20;
  dino.alive = true;
  dino.respawnAt = 0;
  Serial.println("[DINO] spawn fallback");
}

void eraseDinoAt(int x, int y, int spriteIndex) {
  const DinoSpriteInfo& sp = DINO_SPRITES[spriteIndex];
  clearRectSafe(x, y, sp.w, sp.h);
}

void eraseDino(int idx) {
  DinoTarget& dino = dinos[idx];
  if (!dino.alive) return;
  eraseDinoAt(dino.x, dino.y, dino.spriteIndex);
}

void drawDino(int idx) {
  DinoTarget& dino = dinos[idx];
  if (!dino.alive) return;
  const DinoSpriteInfo& sp = DINO_SPRITES[dino.spriteIndex];
  drawRGB565Transparent(dino.x, dino.y, sp.data, sp.w, sp.h, sp.key, dino.flipX);
}

int hitDinoIndexAt(int x, int y) {
  for (int i = 0; i < MAX_DINOS; i++) {
    DinoTarget& dino = dinos[i];
    if (!dino.alive) continue;
    const DinoSpriteInfo& sp = DINO_SPRITES[dino.spriteIndex];
    if (x >= dino.x && x < dino.x + sp.w && y >= dino.y && y < dino.y + sp.h) return i;
  }
  return -1;
}

void startBlood(int x, int y) {
  blood.active = true;
  blood.spriteIndex = randRange(0, 2);
  blood.x = x;
  blood.y = y;
  blood.until = millis() + BLOOD_MS;
}

void eraseBlood() {
  if (!blood.active) return;
  const BloodSpriteInfo& b = BLOOD_SPRITES[blood.spriteIndex];
  clearRectSafe(blood.x, blood.y, b.w, b.h);
}

void drawBlood() {
  if (!blood.active) return;
  const BloodSpriteInfo& b = BLOOD_SPRITES[blood.spriteIndex];
  drawRGB565Transparent(blood.x, blood.y, b.data, b.w, b.h, b.key, false);
}

void scheduleBonusSpawn() {
  bonus.nextSpawnAt = millis() + randRange(BONUS_MIN_INTERVAL_MS, BONUS_MAX_INTERVAL_MS);
}

void spawnBonus() {
  bonus.active = true;
  bonus.x = randRange(6, SCREEN_W - BONUS_SIZE - 6);
  bonus.y = randRange(PLAY_Y + 4, SCREEN_H - BONUS_SIZE - 6);
  bonus.until = millis() + randRange(BONUS_MIN_LIFE_MS, BONUS_MAX_LIFE_MS);
}

void eraseBonus() {
  if (!bonus.active) return;
  clearRectSafe(bonus.x - 2, bonus.y - 2, BONUS_SIZE + 4, BONUS_SIZE + 4);
}

void drawBonus() {
  if (!bonus.active) return;
  lcd.fillCircle(bonus.x + BONUS_SIZE / 2, bonus.y + BONUS_SIZE / 2, BONUS_SIZE / 2, TFT_ORANGE);
  lcd.drawCircle(bonus.x + BONUS_SIZE / 2, bonus.y + BONUS_SIZE / 2, BONUS_SIZE / 2, TFT_WHITE);
  lcd.setTextColor(TFT_WHITE, TFT_ORANGE);
  lcd.setTextSize(1);
  lcd.setCursor(bonus.x + 6, bonus.y + 9);
  lcd.print("+3s");
}

bool pointInBonus(int x, int y) {
  if (!bonus.active) return false;
  return x >= bonus.x && x < bonus.x + BONUS_SIZE && y >= bonus.y && y < bonus.y + BONUS_SIZE;
}

void gunShot() {
  muzzleUntil = millis() + MUZZLE_MS;
  shotVisualActive = true;
  fireSound();
  ledFlashRed(LED_FLASH_MS);
  eraseGunZone();
  drawGun(true);
}

void missShot() {
  misses++;
  combo = 0;
  lives--;
  hudDirty = true;
  Serial.printf("[GAME] rate misses=%d vies=%d\n", misses, lives);
}

void hitDino(int idx) {
  DinoTarget& dino = dinos[idx];
  score += 10 + combo * 2;
  combo++;
  hudDirty = true;
  startBlood(dino.x + 6, dino.y + 6);
  dino.alive = false;
  dino.respawnAt = millis() + 450;
  Serial.printf("[GAME] hit score=%d combo=%d\n", score, combo);
}

void startGame() {
  score = 0;
  combo = 0;
  misses = 0;
  lives = 5;
  timeLeft = 45;
  lastSecond = millis();
  blood.active = false;
  bonus.active = false;
  scheduleBonusSpawn();
  muzzleUntil = 0;
  shotVisualActive = false;
  state = GS_PLAY;
  hudDirty = true;
  needFullRedraw = true;
  lastDinoMove = millis();
  ledOff();
  for (int i = 0; i < MAX_DINOS; i++) {
    dinos[i].alive = false;
    dinos[i].respawnAt = 0;
    spawnDino(i);
  }
}

void endGame() {
  bestIndexCandidate = findHighScorePosition(score);
  if (bestIndexCandidate >= 0) {
    initials[0] = 'A';
    initials[1] = 'A';
    initials[2] = 'A';
    initials[3] = '\0';
    initialsIndex = 0;
    state = GS_ENTER_NAME;
  } else {
    state = GS_GAMEOVER;
  }
  needFullRedraw = true;
  ledOff();
  Serial.printf("[GAME] fin score=%d pos=%d\n", score, bestIndexCandidate);
}

void fullRedrawTitle() {
  lcd.fillScreen(TFT_BLACK);
  drawGround();

  const char* l1;
  const char* l2;
  const char* l3;
  const char* l4;
  const char* startText;
  getIntroText(l1, l2, l3, l4, startText);

  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextSize(2);
  lcd.setCursor(74, 28);
  lcd.print("DINO BLASTER");

  lcd.setTextSize(1);
  lcd.setCursor(34, 74);  lcd.print(l1);
  lcd.setCursor(34, 90);  lcd.print(l2);
  lcd.setCursor(34, 106); lcd.print(l3);
  lcd.setCursor(34, 122); lcd.print(l4);

  drawRGB565Transparent(188, 148, dino3, dino3_W, dino3_H, dino3_KEY, true);
  drawRGB565Transparent(246, 154, dino5, dino5_W, dino5_H, dino5_KEY, false);
  drawGun(false);

  lcd.setCursor(84, 210);
  lcd.print(startText);
}

void fullRedrawGame() {
  lcd.fillScreen(TFT_BLACK);
  drawGround();
  drawHUD();
  drawGun(false);
  for (int i = 0; i < MAX_DINOS; i++) drawDino(i);
  drawBonus();
}

void fullRedrawGameOver() {
  lcd.fillScreen(TFT_BLACK);
  drawGround();
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextSize(2);
  lcd.setCursor(86, 35);
  lcd.print("PARTIE TERMINEE");

  lcd.setTextSize(1);
  lcd.setCursor(118, 82); lcd.printf("Score: %d", score);
  lcd.setCursor(118, 98); lcd.printf("Rates: %d", misses);
  lcd.setCursor(108, 124); lcd.print("Top 5 local :");

  for (int i = 0; i < 5; i++) {
    lcd.setCursor(92, 146 + i * 14);
    lcd.printf("%d. %s  %d", i + 1, highs[i].name, highs[i].score);
  }

  lcd.setCursor(62, 220);
  lcd.print("Touchez l'ecran pour revenir au titre");
}

void fullRedrawEnterName() {
  lcd.fillScreen(TFT_BLACK);
  drawGround();
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextSize(2);
  lcd.setCursor(82, 30);
  lcd.print("NOUVEAU SCORE !");

  lcd.setTextSize(1);
  lcd.setCursor(110, 66);
  lcd.printf("Score: %d", score);
  lcd.setCursor(70, 84);
  lcd.print("Choisis tes 3 lettres");

  for (int i = 0; i < 3; i++) {
    int bx = 70 + i * 60;
    uint16_t bg = (i == initialsIndex) ? TFT_DARKCYAN : TFT_DARKGREY;
    lcd.fillRoundRect(bx, 110, 44, 36, 6, bg);
    lcd.setTextColor(TFT_WHITE, bg);
    lcd.setTextSize(2);
    lcd.setCursor(bx + 14, 120);
    lcd.printf("%c", initials[i]);

    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setTextSize(1);
    lcd.drawCentreString("+", bx + 22, 96, 2);
    lcd.drawCentreString("-", bx + 22, 150, 2);
  }

  lcd.fillRoundRect(236, 112, 60, 40, 8, TFT_DARKGREEN);
  lcd.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  lcd.setTextSize(2);
  lcd.setCursor(247, 122);
  lcd.print("OK");

  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextSize(1);
  lcd.setCursor(40, 202);
  lcd.print("Haut/Bas d'une lettre pour changer, OK pour valider");
}

void renderPlayPartial() {
  if (hudDirty) {
    drawHUD();
    hudDirty = false;
  }

  if (blood.active) drawBlood();
  for (int i = 0; i < MAX_DINOS; i++) {
    if (dinos[i].alive) drawDino(i);
  }
  drawBonus();
}

void render() {
  if (needFullRedraw) {
    needFullRedraw = false;
    switch (state) {
      case GS_TITLE: fullRedrawTitle(); break;
      case GS_PLAY: fullRedrawGame(); break;
      case GS_ENTER_NAME: fullRedrawEnterName(); break;
      case GS_GAMEOVER: fullRedrawGameOver(); break;
    }
    return;
  }

  if (state == GS_PLAY) {
    renderPlayPartial();
  }
}

void updatePlay() {
  if (millis() - lastSecond >= 1000) {
    lastSecond += 1000;
    timeLeft--;
    hudDirty = true;
    if (timeLeft <= 0) {
      endGame();
      return;
    }
  }

  if (blood.active && millis() > blood.until) {
    eraseBlood();
    blood.active = false;
  }

  if (bonus.active && millis() > bonus.until) {
    eraseBonus();
    bonus.active = false;
    scheduleBonusSpawn();
  }

  if (!bonus.active && bonus.nextSpawnAt && millis() >= bonus.nextSpawnAt) {
    spawnBonus();
  }

  for (int i = 0; i < MAX_DINOS; i++) {
    DinoTarget& dino = dinos[i];
    if (!dino.alive && dino.respawnAt && millis() >= dino.respawnAt) {
      spawnDino(i);
    }
  }

  if (millis() - lastDinoMove >= DINO_MOVE_MS) {
    lastDinoMove = millis();
    for (int i = 0; i < MAX_DINOS; i++) {
      DinoTarget& dino = dinos[i];
      if (!dino.alive) continue;
      eraseDino(i);
      dino.x += dino.vx;
      const DinoSpriteInfo& sp = DINO_SPRITES[dino.spriteIndex];
      if (dino.x + sp.w < 0) {
        dino.alive = false;
        dino.respawnAt = millis() + randRange(500, 1200);
      }
    }
  }

  bool muzzleOn = millis() < muzzleUntil;
  if (!muzzleOn && shotVisualActive) {
    clearGunShotEffect();
    shotVisualActive = false;
  }

  if (ts.pressed) {
    gunShot();

    if (pointInBonus(ts.x, ts.y)) {
      eraseBonus();
      bonus.active = false;
      scheduleBonusSpawn();
      timeLeft += BONUS_TIME_SECONDS;
      hudDirty = true;
    } else {
      int idx = hitDinoIndexAt(ts.x, ts.y);
      if (idx >= 0) {
        eraseDino(idx);
        hitDino(idx);
      } else {
        missShot();
        if (lives <= 0) {
          endGame();
          return;
        }
      }
    }
  }
}

void handleTitleTouch() {
  if (ts.pressed) startGame();
}

void handleGameOverTouch() {
  if (ts.pressed) {
    state = GS_TITLE;
    needFullRedraw = true;
  }
}

void handleEnterNameTouch() {
  if (!ts.pressed) return;

  for (int i = 0; i < 3; i++) {
    int bx = 70 + i * 60;
    if (ts.x >= bx && ts.x < bx + 44) {
      initialsIndex = i;

      if (ts.y >= 88 && ts.y < 110) {
        initials[i]++;
        if (initials[i] > 'Z') initials[i] = 'A';
        needFullRedraw = true;
        return;
      }

      if (ts.y >= 146 && ts.y < 170) {
        initials[i]--;
        if (initials[i] < 'A') initials[i] = 'Z';
        needFullRedraw = true;
        return;
      }

      if (ts.y >= 110 && ts.y < 146) {
        needFullRedraw = true;
        return;
      }
    }
  }

  if (ts.x >= 236 && ts.x < 296 && ts.y >= 112 && ts.y < 152) {
    insertHighScore(bestIndexCandidate, initials, score);
    state = GS_GAMEOVER;
    needFullRedraw = true;
  }
}

void handleUI() {
  switch (state) {
    case GS_TITLE: handleTitleTouch(); break;
    case GS_PLAY: break;
    case GS_ENTER_NAME: handleEnterNameTouch(); break;
    case GS_GAMEOVER: handleGameOverTouch(); break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("===========================");
  Serial.println("=== DINO BLASTER START ===");
  Serial.println("===========================");

  prepareReturnToLauncher();

  lcd.init();
  lcd.setRotation(3);
  lcd.setBrightness(255);
  Serial.println("[DISPLAY] OK");

  touch.begin();
  Serial.println("[TOUCH] OK");

  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  ledOff();

  sdBegin();
  loadSettings();
  loadHighs();

  wavOk = sdOk && SD.exists(WAV_PATH);
  Serial.printf("[WAV] %s\n", wavOk ? "gun.wav trouve" : "gun.wav absent -> beep fallback");

  needFullRedraw = true;
  hudDirty = true;
}

void loop() {
  if (millis() - lastFrame < 33) return;
  lastFrame = millis();

  updateTouch();
  ledUpdate();
  handleUI();

  if (state == GS_PLAY) {
    updatePlay();
  }

  if (millis() - lastLog > 1000) {
    lastLog = millis();
    Serial.printf("[LOOP] state=%d score=%d vies=%d temps=%d combo=%d\n",
                  state, score, lives, timeLeft, combo);
  }

  render();
}
