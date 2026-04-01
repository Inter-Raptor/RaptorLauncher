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

  uint8_t fileHeader[14];
  if (bmpFile.read(fileHeader, 14) != 14) {
    Serial.println("[BMP] header erreur");
    bmpFile.close();
    return false;
  }

  if (fileHeader[0] != 'B' || fileHeader[1] != 'M') {
    Serial.println("[BMP] signature invalide");
    bmpFile.close();
    return false;
  }

  uint32_t dataOffset =
      fileHeader[10] |
      (fileHeader[11] << 8) |
      (fileHeader[12] << 16) |
      (fileHeader[13] << 24);

  uint8_t dibHeader[40];
  if (bmpFile.read(dibHeader, 40) != 40) {
    Serial.println("[BMP] DIB erreur");
    bmpFile.close();
    return false;
  }

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

  uint32_t compression =
      dibHeader[16] |
      (dibHeader[17] << 8) |
      (dibHeader[18] << 16) |
      (dibHeader[19] << 24);

  Serial.print("[BMP] w=");
  Serial.print(bmpWidth);
  Serial.print(" h=");
  Serial.print(bmpHeight);
  Serial.print(" depth=");
  Serial.print(depth);
  Serial.print(" comp=");
  Serial.println(compression);

  if (depth != 24) {
    Serial.println("[BMP] seulement 24 bits");
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

  uint32_t rowSize = (bmpWidth * 3 + 3) & ~3;

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

  uint8_t* rowBuffer = (uint8_t*)malloc(rowSize);
  uint16_t* lineBuffer = (uint16_t*)malloc(drawW * sizeof(uint16_t));

  if (!rowBuffer || !lineBuffer) {
    Serial.println("[BMP] malloc fail");
    if (rowBuffer) free(rowBuffer);
    if (lineBuffer) free(lineBuffer);
    bmpFile.close();
    return false;
  }

  for (int row = 0; row < drawH; row++) {
    uint32_t fileRow = flip ? (bmpHeight - 1 - row) : row;
    uint32_t pos = dataOffset + fileRow * rowSize;

    if (!bmpFile.seek(pos)) {
      Serial.println("[BMP] seek impossible");
      free(rowBuffer);
      free(lineBuffer);
      bmpFile.close();
      return false;
    }

    int readLen = bmpFile.read(rowBuffer, rowSize);
    if (readLen != (int)rowSize) {
      Serial.print("[BMP] lecture ligne incomplete: ");
      Serial.println(readLen);
      free(rowBuffer);
      free(lineBuffer);
      bmpFile.close();
      return false;
    }

    for (int col = 0; col < drawW; col++) {
      uint8_t b = rowBuffer[col * 3 + 0];
      uint8_t g = rowBuffer[col * 3 + 1];
      uint8_t r = rowBuffer[col * 3 + 2];
      lineBuffer[col] = lcd.color565(r, g, b);
    }

    lcd.pushImage(x, y + row, drawW, 1, lineBuffer);

    if ((row % 10) == 0) {
      Serial.print("[BMP] ligne ");
      Serial.println(row);
    }
  }

  free(rowBuffer);
  free(lineBuffer);
  bmpFile.close();

  Serial.println("[BMP] affiche OK");
  return true;
}