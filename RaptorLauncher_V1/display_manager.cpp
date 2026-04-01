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

bool displayDrawBMP(const char* path, int x, int y) {
  Serial.print("[IMG] ouverture: ");
  Serial.println(path);

  if (!SD.exists(path)) {
    Serial.println("[IMG] fichier introuvable");
    return false;
  }

  lcd.drawBmpFile(SD, path, x, y);

  Serial.println("[IMG] drawBmpFile OK");
  return true;
}