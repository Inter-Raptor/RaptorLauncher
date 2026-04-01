#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <SD.h>
#include "config.h"
#include "display_manager.h"

// ====== Configuration écran pour ESP32-2432S028 ======
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

      cfg.pin_cs           = 15;
      cfg.pin_rst          = -1;
      cfg.pin_busy         = -1;

      cfg.memory_width     = 240;
      cfg.memory_height    = 320;
      cfg.panel_width      = 240;
      cfg.panel_height     = 320;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable         = true;
      cfg.invert           = false;
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = false;

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
  lcd.setCursor(10, 35);
  lcd.print("Ecran OK");

  Serial.println("[DISPLAY] init fin");
}

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

static uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read();
  ((uint8_t *)&result)[1] = f.read();
  return result;
}

static uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read();
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read();
  return result;
}

bool displayDrawBMP(const char* path, int x, int y) {
  Serial.print("[BMP] ouverture: ");
  Serial.println(path);

  File bmpFile = SD.open(path);
  if (!bmpFile) {
    Serial.println("[BMP] fichier introuvable");
    return false;
  }

  if (read16(bmpFile) != 0x4D42) {
    Serial.println("[BMP] signature invalide");
    bmpFile.close();
    return false;
  }

  (void)read32(bmpFile); // taille fichier
  (void)read32(bmpFile); // reserve
  uint32_t imageOffset = read32(bmpFile);

  uint32_t headerSize = read32(bmpFile);
  int32_t bmpWidth = (int32_t)read32(bmpFile);
  int32_t bmpHeight = (int32_t)read32(bmpFile);

  if (read16(bmpFile) != 1) {
    Serial.println("[BMP] planes invalide");
    bmpFile.close();
    return false;
  }

  uint16_t depth = read16(bmpFile);
  uint32_t compression = read32(bmpFile);

  if (headerSize != 40 || compression != 0 || (depth != 24 && depth != 16)) {
    Serial.println("[BMP] format non supporte");
    bmpFile.close();
    return false;
  }

  bool flip = true;
  if (bmpHeight < 0) {
    bmpHeight = -bmpHeight;
    flip = false;
  }

  uint32_t rowSize = ((bmpWidth * depth / 8) + 3) & ~3;

  if ((x >= lcd.width()) || (y >= lcd.height())) {
    bmpFile.close();
    return false;
  }

  int w = bmpWidth;
  int h = bmpHeight;

  if ((x + w - 1) >= lcd.width())  w = lcd.width()  - x;
  if ((y + h - 1) >= lcd.height()) h = lcd.height() - y;

  lcd.startWrite();

  for (int row = 0; row < h; row++) {
    uint32_t pos;
    if (flip) {
      pos = imageOffset + (bmpHeight - 1 - row) * rowSize;
    } else {
      pos = imageOffset + row * rowSize;
    }

    if (bmpFile.position() != pos) {
      bmpFile.seek(pos);
    }

    for (int col = 0; col < w; col++) {
      uint16_t color;

      if (depth == 24) {
        uint8_t b = bmpFile.read();
        uint8_t g = bmpFile.read();
        uint8_t r = bmpFile.read();
        color = lcd.color565(r, g, b);
      } else {
        color = read16(bmpFile);
      }

      lcd.drawPixel(x + col, y + row, color);
    }
  }

  lcd.endWrite();
  bmpFile.close();

  Serial.println("[BMP] affiche OK");
    Serial.print("[BMP] x=");
  Serial.print(x);
  Serial.print(" y=");
  Serial.println(y);
  return true;
}