#include "raptor_game_sdk.h"
#include <Preferences.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>

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
  SD.begin(SDK_PIN_SD_CS, sdSPI);

  ledcAttach(SDK_PIN_SPEAKER, 2000, 8);
  ledcWrite(SDK_PIN_SPEAKER, 0);

  pinMode(SDK_PIN_LED_R, OUTPUT);
  pinMode(SDK_PIN_LED_G, OUTPUT);
  pinMode(SDK_PIN_LED_B, OUTPUT);
  digitalWrite(SDK_PIN_LED_R, HIGH);
  digitalWrite(SDK_PIN_LED_G, HIGH);
  digitalWrite(SDK_PIN_LED_B, HIGH);

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
    int x = map(p.x, SDK_TOUCH_X_MIN, SDK_TOUCH_X_MAX, 0, SDK_SCREEN_WIDTH - 1);
    int y = map(p.y, SDK_TOUCH_Y_MIN, SDK_TOUCH_Y_MAX, SDK_SCREEN_HEIGHT - 1, 0);
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

bool RaptorGameSDK::saveJson(const JsonDocument& doc) {
  String path = saveJsonPath();
  SD.remove(path);
  File f = SD.open(path, FILE_WRITE);
  if (!f) return false;

  size_t n = serializeJsonPretty(doc, f);
  f.close();
  return n > 0;
}

bool RaptorGameSDK::loadJson(JsonDocument& doc) {
  String path = saveJsonPath();
  File f = SD.open(path, FILE_READ);
  if (!f) return false;

  DeserializationError err = deserializeJson(doc, f);
  f.close();
  return !err;
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
