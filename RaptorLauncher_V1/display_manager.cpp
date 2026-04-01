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
  Serial.print("[BMP] ouverture: ");
  Serial.println(path);

  File bmpFile = SD.open(path, FILE_READ);
  if (!bmpFile) {
    Serial.println("[BMP] fichier introuvable");
    return false;
  }

  // --- Header BMP 14 octets ---
  uint8_t fileHeader[14];
  if (bmpFile.read(fileHeader, 14) != 14) {
    Serial.println("[BMP] lecture fileHeader impossible");
    bmpFile.close();
    return false;
  }

  if (fileHeader[0] != 'B' || fileHeader[1] != 'M') {
    Serial.println("[BMP] signature invalide");
    bmpFile.close();
    return false;
  }

  uint32_t dataOffset =
      (uint32_t)fileHeader[10] |
      ((uint32_t)fileHeader[11] << 8) |
      ((uint32_t)fileHeader[12] << 16) |
      ((uint32_t)fileHeader[13] << 24);

  // --- DIB header minimal 40 octets ---
  uint8_t dibHeader[40];
  if (bmpFile.read(dibHeader, 40) != 40) {
    Serial.println("[BMP] lecture DIB impossible");
    bmpFile.close();
    return false;
  }

  uint32_t dibSize =
      (uint32_t)dibHeader[0] |
      ((uint32_t)dibHeader[1] << 8) |
      ((uint32_t)dibHeader[2] << 16) |
      ((uint32_t)dibHeader[3] << 24);

  int32_t bmpWidth =
      (int32_t)(
        (uint32_t)dibHeader[4] |
        ((uint32_t)dibHeader[5] << 8) |
        ((uint32_t)dibHeader[6] << 16) |
        ((uint32_t)dibHeader[7] << 24)
      );

  int32_t bmpHeight =
      (int32_t)(
        (uint32_t)dibHeader[8] |
        ((uint32_t)dibHeader[9] << 8) |
        ((uint32_t)dibHeader[10] << 16) |
        ((uint32_t)dibHeader[11] << 24)
      );

  uint16_t planes =
      (uint16_t)dibHeader[12] |
      ((uint16_t)dibHeader[13] << 8);

  uint16_t depth =
      (uint16_t)dibHeader[14] |
      ((uint16_t)dibHeader[15] << 8);

  uint32_t compression =
      (uint32_t)dibHeader[16] |
      ((uint32_t)dibHeader[17] << 8) |
      ((uint32_t)dibHeader[18] << 16) |
      ((uint32_t)dibHeader[19] << 24);

  Serial.print("[BMP] dib=");
  Serial.print(dibSize);
  Serial.print(" w=");
  Serial.print(bmpWidth);
  Serial.print(" h=");
  Serial.print(bmpHeight);
  Serial.print(" planes=");
  Serial.print(planes);
  Serial.print(" depth=");
  Serial.print(depth);
  Serial.print(" comp=");
  Serial.println(compression);

  if (dibSize < 40) {
    Serial.println("[BMP] DIB non supporte");
    bmpFile.close();
    return false;
  }

  if (planes != 1) {
    Serial.println("[BMP] planes invalide");
    bmpFile.close();
    return false;
  }

  if (depth != 24) {
    Serial.println("[BMP] seulement 24 bits supporte");
    bmpFile.close();
    return false;
  }

  if (compression != 0) {
    Serial.println("[BMP] compression non supportee");
    bmpFile.close();
    return false;
  }

  if (bmpWidth <= 0 || bmpHeight == 0) {
    Serial.println("[BMP] dimensions invalides");
    bmpFile.close();
    return false;
  }

  bool flip = true;
  if (bmpHeight < 0) {
    bmpHeight = -bmpHeight;
    flip = false;
  }

  // Taille d'une ligne BMP alignée sur 4 octets
  uint32_t rowSize = (bmpWidth * 3 + 3) & ~3;

  // Vérif écran
  if (x >= lcd.width() || y >= lcd.height()) {
    Serial.println("[BMP] position hors ecran");
    bmpFile.close();
    return false;
  }

  int drawW = bmpWidth;
  int drawH = bmpHeight;

  if (x + drawW > lcd.width())  drawW = lcd.width() - x;
  if (y + drawH > lcd.height()) drawH = lcd.height() - y;

  if (drawW <= 0 || drawH <= 0) {
    Serial.println("[BMP] rien a afficher");
    bmpFile.close();
    return false;
  }

  // Buffer d'une ligne max 240 px -> 240 * 2 = 480 octets
  static uint16_t lineBuffer[240];

  lcd.startWrite();

  for (int row = 0; row < drawH; row++) {
    uint32_t fileRow = flip ? (bmpHeight - 1 - row) : row;
    uint32_t pos = dataOffset + fileRow * rowSize;

    if (!bmpFile.seek(pos)) {
      Serial.println("[BMP] seek impossible");
      lcd.endWrite();
      bmpFile.close();
      return false;
    }

    for (int col = 0; col < drawW; col++) {
      int b = bmpFile.read();
      int g = bmpFile.read();
      int r = bmpFile.read();

      if (b < 0 || g < 0 || r < 0) {
        Serial.println("[BMP] lecture pixel impossible");
        lcd.endWrite();
        bmpFile.close();
        return false;
      }

      lineBuffer[col] = lcd.color565((uint8_t)r, (uint8_t)g, (uint8_t)b);
    }

    lcd.pushImage(x, y + row, drawW, 1, lineBuffer);
  }

  lcd.endWrite();
  bmpFile.close();

  Serial.println("[BMP] affiche OK");
  return true;
}