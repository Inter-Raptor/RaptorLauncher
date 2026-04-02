#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <SD.h>
#include "config.h"
#include "display_manager.h"

// ====== CONFIG ECRAN ESP32-2432S028 ======
class LGFX_2432S028 : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9341 _panel_instance;
  lgfx::Bus_SPI _bus_instance;
  lgfx::Light_PWM _light_instance;

public:
  LGFX_2432S028(void) {
    {
      auto cfg = _bus_instance.config();

      cfg.spi_host = VSPI_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read  = 16000000;

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

      cfg.pin_cs   = 15;
      cfg.pin_rst  = -1;
      cfg.pin_busy = -1;

      cfg.memory_width  = 240;
      cfg.memory_height = 320;
      cfg.panel_width   = 240;
      cfg.panel_height  = 320;

      cfg.offset_x = 0;
      cfg.offset_y = 0;

      cfg.readable = true;
      cfg.invert   = false;

      // ⚠️ IMPORTANT POUR COULEURS
      cfg.rgb_order = false;

      cfg.dlen_16bit = false;
      cfg.bus_shared = false;

      _panel_instance.config(cfg);
    }

    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = 21;
      cfg.invert = false;
      cfg.freq   = 44100;
      cfg.pwm_channel = 7;

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    setPanel(&_panel_instance);
  }
};

static LGFX_2432S028 lcd;

// ================= INIT =================
void displayInit() {
  Serial.println("[DISPLAY] init debut");

  lcd.init();
  lcd.setRotation(1);
  lcd.setBrightness(255);
  lcd.fillScreen(TFT_BLACK);

  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextSize(2);
  lcd.setCursor(10, 10);
  lcd.print("Raptor Launcher");

  Serial.println("[DISPLAY] init fin");
}

// ================= BASIC =================
void displayClear() {
  lcd.fillScreen(TFT_BLACK);
}

void displayDrawText(int x, int y, const char* text) {
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextSize(2);
  lcd.setCursor(x, y);
  lcd.print(text);
}

void displayFillScreen(uint16_t color) {
  lcd.fillScreen(color);
}

// ================= TEST COULEUR =================
void displayColorBarsTest() {
  displayClear();

  lcd.fillRect(0,   0, 60, 120, TFT_RED);
  lcd.fillRect(60,  0, 60, 120, TFT_GREEN);
  lcd.fillRect(120, 0, 60, 120, TFT_BLUE);
  lcd.fillRect(180, 0, 60, 120, TFT_YELLOW);

  lcd.setCursor(10, 140);
  lcd.print("R");
  lcd.setCursor(70, 140);
  lcd.print("G");
  lcd.setCursor(130, 140);
  lcd.print("B");
  lcd.setCursor(190, 140);
  lcd.print("Y");
}

// ================= BMP =================
bool displayDrawBMP(const char* path, int x, int y) {
  Serial.print("[BMP] ouverture: ");
  Serial.println(path);

  File bmpFile = SD.open(path, FILE_READ);
  if (!bmpFile) {
    Serial.println("[BMP] fichier introuvable");
    return false;
  }

  uint8_t fileHeader[14];
  bmpFile.read(fileHeader, 14);

  uint32_t dataOffset =
      fileHeader[10] |
      (fileHeader[11] << 8) |
      (fileHeader[12] << 16) |
      (fileHeader[13] << 24);

  uint8_t dibHeader[40];
  bmpFile.read(dibHeader, 40);

  int32_t bmpWidth =
      dibHeader[4] |
      (dibHeader[5] << 8) |
      (dibHeader[6] << 16) |
      (dibHeader[7] << 24);

  int32_t bmpHeight =
      dibHeader[8] |
      (dibHeader[9] << 8) |
      (dibHeader[10] << 16) |
      (dibHeader[11] << 24);

  uint16_t depth =
      dibHeader[14] |
      (dibHeader[15] << 8);

  Serial.print("[BMP] w=");
  Serial.print(bmpWidth);
  Serial.print(" h=");
  Serial.print(bmpHeight);
  Serial.print(" depth=");
  Serial.println(depth);

  if (depth != 24) {
    Serial.println("[BMP] mauvais format");
    bmpFile.close();
    return false;
  }

  uint32_t rowSize = (bmpWidth * 3 + 3) & ~3;

  uint8_t* rowBuffer = (uint8_t*)malloc(rowSize);
  static uint16_t lineBuffer[240];

  bool flip = true;
  if (bmpHeight < 0) {
    bmpHeight = -bmpHeight;
    flip = false;
  }

  for (int row = 0; row < bmpHeight; row++) {
    uint32_t pos = dataOffset + (flip ? (bmpHeight - 1 - row) : row) * rowSize;
    bmpFile.seek(pos);

    bmpFile.read(rowBuffer, rowSize);

    for (int col = 0; col < bmpWidth; col++) {
      uint8_t b = rowBuffer[col * 3 + 0];
      uint8_t g = rowBuffer[col * 3 + 1];
      uint8_t r = rowBuffer[col * 3 + 2];

      // ⚠️ PAS D'INVERSION ICI
      lineBuffer[col] = lcd.color565(r, g, b);
    }

    lcd.pushImage(x, y + row, bmpWidth, 1, lineBuffer);
  }

  free(rowBuffer);
  bmpFile.close();

  Serial.println("[BMP] affiche OK");
  return true;
}