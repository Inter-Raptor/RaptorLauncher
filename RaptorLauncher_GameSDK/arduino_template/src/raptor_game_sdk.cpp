#include "raptor_game_sdk.h"
#include <Preferences.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>
#if __has_include(<AudioFileSourceSD.h>) && __has_include(<AudioGeneratorWAV.h>) && __has_include(<AudioGeneratorMP3.h>) && __has_include(<AudioOutputI2SNoDAC.h>)
  #include <AudioFileSourceSD.h>
  #include <AudioGeneratorWAV.h>
  #include <AudioGeneratorMP3.h>
  #include <AudioOutputI2SNoDAC.h>
  #define SDK_HAS_ADV_AUDIO 1
#else
  #define SDK_HAS_ADV_AUDIO 0
#endif


class SDKDisplay : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 panel;
  lgfx::Bus_SPI bus;
  lgfx::Light_PWM light;
public:
  SDKDisplay() {
    auto busCfg = bus.config();
    busCfg.spi_host = VSPI_HOST;
    busCfg.spi_mode = 0;
    busCfg.freq_write = 40000000;
    busCfg.freq_read = 16000000;
    busCfg.pin_sclk = SDK_PIN_TFT_SCK;
    busCfg.pin_mosi = SDK_PIN_TFT_MOSI;
    busCfg.pin_miso = SDK_PIN_TFT_MISO;
    busCfg.pin_dc = SDK_PIN_TFT_DC;
    busCfg.dma_channel = 1;
    bus.config(busCfg);
    panel.setBus(&bus);

    auto panelCfg = panel.config();
    panelCfg.pin_cs = SDK_PIN_TFT_CS;
    panelCfg.pin_rst = SDK_PIN_TFT_RST;
    panelCfg.memory_width = 240;
    panelCfg.memory_height = 320;
    panelCfg.panel_width = 240;
    panelCfg.panel_height = 320;
    panelCfg.invert = true;
    panel.config(panelCfg);

    auto lightCfg = light.config();
    lightCfg.pin_bl = SDK_PIN_TFT_BL;
    lightCfg.freq = 12000;
    lightCfg.pwm_channel = 7;
    light.config(lightCfg);
    panel.setLight(&light);

    setPanel(&panel);
  }
};

static SDKDisplay gLcd;
static const char* PREF_NS = "raptor_boot";
static const char* PREF_LAUNCHER_LABEL = "launcher";

void RaptorGameSDK::begin() {
  Serial.begin(115200);
  delay(120);

  initDisplay();
  initInput();

  sdSPI.begin(SDK_PIN_SD_SCK, SDK_PIN_SD_MISO, SDK_PIN_SD_MOSI, SDK_PIN_SD_CS);
  sdReady = SD.begin(SDK_PIN_SD_CS, sdSPI);
  if (!sdReady) {
    Serial.println("[SDK] SD init KO");
  }
  loadTouchCalibrationFromSettings();

  ledcAttach(SDK_PIN_SPEAKER, 2000, 8);
  ledcWrite(SDK_PIN_SPEAKER, 0);

  ledcAttach(SDK_PIN_LED_R, 5000, 8);
  ledcAttach(SDK_PIN_LED_G, 5000, 8);
  ledcAttach(SDK_PIN_LED_B, 5000, 8);
  ledcWrite(SDK_PIN_LED_R, 255);
  ledcWrite(SDK_PIN_LED_G, 255);
  ledcWrite(SDK_PIN_LED_B, 255);

  analogReadResolution(12);

  // Important: des le debut du jeu, on arme le retour auto au launcher
  // en cas de reboot/reset inattendu.
  bool armed = armReturnToLauncherOnNextBoot();

  clear();
  drawCenteredText(10, "Raptor Game SDK ready");
  drawSmallText(8, 34, armed ? "Boot safety: launcher armed" : "Boot safety: launcher label missing", armed ? SDK_COLOR_OK : SDK_COLOR_WARN, SDK_COLOR_BG);
}

void RaptorGameSDK::initDisplay() {
  gLcd.init();
  gLcd.setRotation(3);
  gLcd.setBrightness(255);
}

void RaptorGameSDK::initInput() {
  Wire.begin(SDK_PIN_I2C_SDA, SDK_PIN_I2C_SCL);
  mcpReady = mcp.begin_I2C(SDK_MCP23017_ADDR);

  if (mcpReady) {
    for (int pin = 0; pin < 16; ++pin) {
      mcp.pinMode(pin, INPUT_PULLUP);
    }
  }

  touch.begin();
}

void RaptorGameSDK::mapButtonsFromMcp(uint8_t gpioA, uint8_t gpioB) {
  bool now[BTN_COUNT] = {false};

  now[BTN_UP]     = !(gpioA & (1 << 0));
  now[BTN_DOWN]   = !(gpioA & (1 << 1));
  now[BTN_LEFT]   = !(gpioA & (1 << 2));
  now[BTN_RIGHT]  = !(gpioA & (1 << 3));
  now[BTN_A]      = !(gpioA & (1 << 4));
  now[BTN_B]      = !(gpioA & (1 << 5));
  now[BTN_X]      = !(gpioA & (1 << 6));
  now[BTN_Y]      = !(gpioA & (1 << 7));
  now[BTN_START]  = !(gpioB & (1 << 0));
  now[BTN_SELECT] = !(gpioB & (1 << 1));

  for (int i = 0; i < BTN_COUNT; i++) {
    bool prev = input.held[i];
    input.held[i] = now[i];
    input.pressed[i] = (!prev && now[i]);
    input.released[i] = (prev && !now[i]);
  }
}

void RaptorGameSDK::loadTouchCalibrationFromSettings() {
  touchCalLoadedFromSettings = false;
  if (!sdReady) return;

  const char* settingsPath = "/settings.json";
  File f = SD.open(settingsPath, FILE_READ);
  if (!f) {
    Serial.println("[SDK] settings.json absent -> calibration tactile par defaut");
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();

  if (err) {
    Serial.println("[SDK] settings.json invalide -> calibration tactile par defaut");
    return;
  }

  touchCalXMin = doc["touch_x_min"] | SDK_TOUCH_X_MIN;
  touchCalXMax = doc["touch_x_max"] | SDK_TOUCH_X_MAX;
  touchCalYMin = doc["touch_y_min"] | SDK_TOUCH_Y_MIN;
  touchCalYMax = doc["touch_y_max"] | SDK_TOUCH_Y_MAX;
  touchOffsetX = doc["touch_offset_x"] | 0;
  touchOffsetY = doc["touch_offset_y"] | 0;

  if (touchCalXMin >= touchCalXMax) {
    touchCalXMin = SDK_TOUCH_X_MIN;
    touchCalXMax = SDK_TOUCH_X_MAX;
  }
  if (touchCalYMin >= touchCalYMax) {
    touchCalYMin = SDK_TOUCH_Y_MIN;
    touchCalYMax = SDK_TOUCH_Y_MAX;
  }

  touchCalLoadedFromSettings = true;
  Serial.printf("[SDK] touch calib x:%d..%d y:%d..%d off:%d,%d\n", touchCalXMin, touchCalXMax, touchCalYMin, touchCalYMax, touchOffsetX, touchOffsetY);
}

void RaptorGameSDK::updateInputs() {
  for (int i = 0; i < BTN_COUNT; i++) {
    input.pressed[i] = false;
    input.released[i] = false;
  }

  bool prevTouch = touchHeld;
  touchHeld = touch.touched();
  touchPressedEdge = (!prevTouch && touchHeld);
  touchReleasedEdge = (prevTouch && !touchHeld);

  if (touchHeld) {
    TS_Point p = touch.getPoint();
    int x = map(p.x, touchCalXMin, touchCalXMax, 0, SDK_SCREEN_WIDTH - 1);
    int y = map(p.y, touchCalYMin, touchCalYMax, SDK_SCREEN_HEIGHT - 1, 0);
    x += touchOffsetX;
    y += touchOffsetY;
    touchPx = constrain(x, 0, SDK_SCREEN_WIDTH - 1);
    touchPy = constrain(y, 0, SDK_SCREEN_HEIGHT - 1);
  }

  if (mcpReady) {
    uint16_t gpioAB = mcp.readGPIOAB();
    uint8_t gpioA = gpioAB & 0xFF;
    uint8_t gpioB = (gpioAB >> 8) & 0xFF;
    mapButtonsFromMcp(gpioA, gpioB);
  }
}

bool RaptorGameSDK::isHeld(GameButton b) const { return input.held[b]; }
bool RaptorGameSDK::isPressed(GameButton b) const { return input.pressed[b]; }
bool RaptorGameSDK::isReleased(GameButton b) const { return input.released[b]; }

bool RaptorGameSDK::isTouchHeld() const { return touchHeld; }
bool RaptorGameSDK::isTouching() const { return touchHeld; }
bool RaptorGameSDK::isTouchPressed() const { return touchPressedEdge; }
bool RaptorGameSDK::isTouchReleased() const { return touchReleasedEdge; }
int RaptorGameSDK::touchX() const { return touchPx; }
int RaptorGameSDK::touchY() const { return touchPy; }

int RaptorGameSDK::width() const { return gLcd.width(); }
int RaptorGameSDK::height() const { return gLcd.height(); }

void RaptorGameSDK::clear(uint16_t color) { gLcd.fillScreen(color); }
void RaptorGameSDK::drawRect(int x, int y, int w, int h, uint16_t color) { gLcd.drawRect(x, y, w, h, color); }
void RaptorGameSDK::fillRect(int x, int y, int w, int h, uint16_t color) { gLcd.fillRect(x, y, w, h, color); }

void RaptorGameSDK::drawSmallText(int x, int y, const char* text, uint16_t fg, uint16_t bg) {
  gLcd.setTextSize(1);
  gLcd.setTextColor(fg, bg);
  gLcd.setCursor(x, y);
  gLcd.print(text);
}

void RaptorGameSDK::drawCenteredText(int y, const char* text, uint16_t fg, uint16_t bg) {
  gLcd.setTextSize(2);
  gLcd.setTextColor(fg, bg);
  int x = (gLcd.width() - gLcd.textWidth(text)) / 2;
  if (x < 0) x = 0;
  gLcd.setCursor(x, y);
  gLcd.print(text);
}

bool RaptorGameSDK::drawRaw565(const String& path, int x, int y, int width, int height) {
  if (!sdReady) return false;
  if (width <= 0 || height <= 0) return false;

  File f = SD.open(path, FILE_READ);
  if (!f) return false;

  uint16_t* line = (uint16_t*)malloc((size_t)width * sizeof(uint16_t));
  if (!line) {
    f.close();
    return false;
  }

  gLcd.setSwapBytes(true);
  for (int row = 0; row < height; ++row) {
    size_t need = (size_t)width * 2;
    if (f.read((uint8_t*)line, need) != (int)need) {
      gLcd.setSwapBytes(false);
      free(line);
      f.close();
      return false;
    }
    gLcd.pushImage(x, y + row, width, 1, line);
  }
  gLcd.setSwapBytes(false);

  free(line);
  f.close();
  return true;
}

bool RaptorGameSDK::drawBmp(const String& path, int x, int y) {
  if (!sdReady) return false;
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  f.close();
  return gLcd.drawBmpFile(SD, path.c_str(), x, y);
}

bool RaptorGameSDK::drawJpg(const String& path, int x, int y) {
  if (!sdReady) return false;
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  f.close();
  return gLcd.drawJpgFile(SD, path.c_str(), x, y);
}

bool RaptorGameSDK::drawPng(const String& path, int x, int y) {
  if (!sdReady) return false;
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  f.close();
  return gLcd.drawPngFile(SD, path.c_str(), x, y);
}

bool RaptorGameSDK::playWav(const String& path) {
  #if SDK_HAS_ADV_AUDIO
    if (!sdReady) return false;
    auto* file = new AudioFileSourceSD(path.c_str());
    auto* out = new AudioOutputI2SNoDAC();
    auto* wav = new AudioGeneratorWAV();
    if (!wav->begin(file, out)) {
      delete wav; delete out; delete file;
      return false;
    }
    while (wav->isRunning()) wav->loop();
    wav->stop();
    delete wav; delete out; delete file;
    return true;
  #else
    (void)path;
    return false;
  #endif
}

bool RaptorGameSDK::playMp3(const String& path) {
  #if SDK_HAS_ADV_AUDIO
    if (!sdReady) return false;
    auto* file = new AudioFileSourceSD(path.c_str());
    auto* out = new AudioOutputI2SNoDAC();
    auto* mp3 = new AudioGeneratorMP3();
    if (!mp3->begin(file, out)) {
      delete mp3; delete out; delete file;
      return false;
    }
    while (mp3->isRunning()) mp3->loop();
    mp3->stop();
    delete mp3; delete out; delete file;
    return true;
  #else
    (void)path;
    return false;
  #endif
}

void RaptorGameSDK::playBeep(int freq, int durationMs) {
  ledcWriteTone(SDK_PIN_SPEAKER, freq);
  ledcWrite(SDK_PIN_SPEAKER, 120);
  delay(durationMs);
  ledcWrite(SDK_PIN_SPEAKER, 0);
  ledcWriteTone(SDK_PIN_SPEAKER, 0);
}

String RaptorGameSDK::gameRootPath() const {
  return String("/games/") + String(SDK_GAME_FOLDER_NAME);
}

String RaptorGameSDK::saveJsonPath() const {
  return gameRootPath() + "/" + String(SDK_META_SAVE_FILENAME);
}

String RaptorGameSDK::assetPath(const String& filename) const {
  String clean = filename;
  while (clean.startsWith("/")) clean.remove(0, 1);
  return gameRootPath() + "/assets/" + clean;
}

bool RaptorGameSDK::saveJson(const JsonDocument& doc) {
  if (!sdReady) return false;
  String path = saveJsonPath();
  SD.remove(path);
  File f = SD.open(path, FILE_WRITE);
  if (!f) return false;

  size_t n = serializeJsonPretty(doc, f);
  f.close();
  return n > 0;
}

bool RaptorGameSDK::loadJson(JsonDocument& doc) {
  if (!sdReady) return false;
  String path = saveJsonPath();
  File f = SD.open(path, FILE_READ);
  if (!f) return false;

  DeserializationError err = deserializeJson(doc, f);
  f.close();
  return !err;
}

bool RaptorGameSDK::isSdReady() const {
  return sdReady;
}

bool RaptorGameSDK::loadLauncherSettings(JsonDocument& doc) const {
  if (!sdReady) return false;
  File f = SD.open("/settings.json", FILE_READ);
  if (!f) return false;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  return !err;
}

bool RaptorGameSDK::validateGameMeta(const String& metaPath, String& errorOut) const {
  if (!sdReady) {
    errorOut = "SD non initialisee";
    return false;
  }

  File f = SD.open(metaPath, FILE_READ);
  if (!f) {
    errorOut = "meta.json introuvable";
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();

  if (err) {
    errorOut = String("meta.json invalide: ") + err.c_str();
    return false;
  }

  if (!doc["name"].is<const char*>()) { errorOut = "champ name manquant"; return false; }
  if (!doc["bin"].is<const char*>()) { errorOut = "champ bin manquant"; return false; }
  if (!doc["save"].is<const char*>()) { errorOut = "champ save manquant"; return false; }

  int iw = doc["icon_w"] | -1;
  int ih = doc["icon_h"] | -1;
  int tw = doc["title_w"] | -1;
  int th = doc["title_h"] | -1;

  if (iw != SDK_META_ICON_W || ih != SDK_META_ICON_H) {
    errorOut = "icon_w/icon_h non conformes";
    return false;
  }
  if (tw != SDK_META_TITLE_W || th != SDK_META_TITLE_H) {
    errorOut = "title_w/title_h non conformes";
    return false;
  }

  if (String((const char*)doc["save"]) != String(SDK_META_SAVE_FILENAME)) {
    errorOut = "save doit etre sauv.json";
    return false;
  }

  errorOut = "OK";
  return true;
}

String RaptorGameSDK::sdkHealthReport() const {
  String out;
  out += "SD=" + String(sdReady ? "OK" : "KO");
  out += " | MCP=" + String(mcpReady ? "OK" : "KO");
  out += " | TouchCal=" + String(touchCalLoadedFromSettings ? "settings.json" : "defaults");
  out += " | WiFi=" + String(wifiIsConnected() ? "ON" : "OFF");
  out += " | LDR=" + String(readLightPercent()) + "%";
  int batt = batteryPercent();
  if (batt >= 0) out += " | BAT=" + String(batt) + "%";
  return out;
}


void RaptorGameSDK::setLedRgb(uint8_t r, uint8_t g, uint8_t b) {
  // LED commune anode (actif low) sur CYD: 0 = allume, 255 = eteint
  uint8_t rr = 255 - r;
  uint8_t gg = 255 - g;
  uint8_t bb = 255 - b;
  ledcWrite(SDK_PIN_LED_R, rr);
  ledcWrite(SDK_PIN_LED_G, gg);
  ledcWrite(SDK_PIN_LED_B, bb);
}

void RaptorGameSDK::ledOff() {
  ledcWrite(SDK_PIN_LED_R, 255);
  ledcWrite(SDK_PIN_LED_G, 255);
  ledcWrite(SDK_PIN_LED_B, 255);
}

int RaptorGameSDK::readLightRaw() const {
  return analogRead(SDK_PIN_LIGHT_SENSOR);
}

int RaptorGameSDK::readLightPercent() const {
  int v = readLightRaw();
  int pct = map(v, 0, 4095, 0, 100);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

bool RaptorGameSDK::hasBatterySense() const {
  return SDK_PIN_BATTERY_ADC >= 0;
}

int RaptorGameSDK::batteryMilliVolts() const {
  if (!hasBatterySense()) return -1;
  int raw = analogRead(SDK_PIN_BATTERY_ADC);
  float mvPin = (raw / 4095.0f) * 3300.0f;
  float mvBat = mvPin * SDK_BATTERY_ADC_DIVIDER;
  return (int)mvBat;
}

int RaptorGameSDK::batteryPercent() const {
  int mv = batteryMilliVolts();
  if (mv < 0) return -1;
  int pct = map(mv, SDK_BATTERY_MV_MIN, SDK_BATTERY_MV_MAX, 0, 100);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

bool RaptorGameSDK::hasBmpSupport() const {
  return true;
}

bool RaptorGameSDK::hasJpgSupport() const {
  return true;
}

bool RaptorGameSDK::hasPngSupport() const {
  return true;
}

bool RaptorGameSDK::hasWavSupport() const {
  return hasAdvancedAudio();
}

bool RaptorGameSDK::hasMp3Support() const {
  return hasAdvancedAudio();
}

bool RaptorGameSDK::hasPngDecoder() const {
  return hasPngSupport();
}

bool RaptorGameSDK::hasAdvancedAudio() const {
  return SDK_HAS_ADV_AUDIO == 1;
}

bool RaptorGameSDK::wifiConnectFromSettings() {
  JsonDocument doc;
  if (!loadLauncherSettings(doc)) return false;

  bool enabled = doc["wifi_enabled"] | false;
  String ssid = doc["wifi_ssid"] | "";
  String pass = doc["wifi_pass"] | "";

  if (!enabled || ssid.length() == 0) return false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < SDK_WIFI_CONNECT_TIMEOUT_MS) {
    delay(150);
  }

  return WiFi.status() == WL_CONNECTED;
}

void RaptorGameSDK::wifiDisconnect() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

bool RaptorGameSDK::wifiIsConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

bool RaptorGameSDK::armReturnToLauncherOnNextBoot() {
  Preferences pref;
  if (!pref.begin(PREF_NS, false)) return false;

  String label = pref.getString(PREF_LAUNCHER_LABEL, "");
  pref.end();

  if (!label.length()) return false;

  const esp_partition_t* launcher = esp_partition_find_first(
    ESP_PARTITION_TYPE_APP,
    ESP_PARTITION_SUBTYPE_ANY,
    label.c_str()
  );

  if (!launcher) return false;
  return esp_ota_set_boot_partition(launcher) == ESP_OK;
}

void RaptorGameSDK::requestReturnToLauncher() {
  armReturnToLauncherOnNextBoot();
  ESP.restart();
}
