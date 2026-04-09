#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <SD.h>
#include "config.h"
#include "display_manager.h"

// ======================================================
// CONFIG ECRAN ESP32-2432S028
// ======================================================
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
      cfg.invert   = true;
      cfg.rgb_order = false;

      cfg.dlen_16bit = false;
      cfg.bus_shared = false;

      _panel_instance.config(cfg);
    }

    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = 27;
      cfg.invert = false;
      cfg.freq   = 12000;
      cfg.pwm_channel = 7;

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    setPanel(&_panel_instance);
  }
};

static LGFX_2432S028 lcd;

// ======================================================
// OUTILS BMP
// ======================================================
static uint16_t bmpColorTo565(uint8_t r, uint8_t g, uint8_t b) {
  return lcd.color565(r, g, b);
}

static bool readLE16(File& f, uint16_t& value) {
  uint8_t b0, b1;
  if (f.read(&b0, 1) != 1) return false;
  if (f.read(&b1, 1) != 1) return false;
  value = (uint16_t)b0 | ((uint16_t)b1 << 8);
  return true;
}

static bool readLE32(File& f, uint32_t& value) {
  uint8_t b0, b1, b2, b3;
  if (f.read(&b0, 1) != 1) return false;
  if (f.read(&b1, 1) != 1) return false;
  if (f.read(&b2, 1) != 1) return false;
  if (f.read(&b3, 1) != 1) return false;
  value = (uint32_t)b0 | ((uint32_t)b1 << 8) | ((uint32_t)b2 << 16) | ((uint32_t)b3 << 24);
  return true;
}

// ======================================================
// API PUBLIQUE
// ======================================================
void displayInit() {
  Serial.println("[DISPLAY] init debut");

  lcd.init();
  lcd.setRotation(3);
  lcd.setBrightness(255);
  lcd.fillScreen(COLOR_BG);

  lcd.setTextColor(COLOR_TEXT, COLOR_BG);
  lcd.setTextSize(2);
  lcd.setCursor(10, 10);
  lcd.print(APP_NAME);

  Serial.print("[DISPLAY] taille: ");
  Serial.print(lcd.width());
  Serial.print("x");
  Serial.println(lcd.height());

  Serial.println("[DISPLAY] init fin");
}

void displayClear() {
  lcd.fillScreen(COLOR_BG);
}

void displayFillScreen(uint16_t color) {
  lcd.fillScreen(color);
}

void displayDrawText(int x, int y, const char* text) {
  lcd.setTextColor(COLOR_TEXT, COLOR_BG);
  lcd.setTextSize(2);
  lcd.setCursor(x, y);
  lcd.print(text);
}

void displayDrawSmallText(int x, int y, const char* text) {
  lcd.setTextColor(COLOR_TEXT, COLOR_BG);
  lcd.setTextSize(1);
  lcd.setCursor(x, y);
  lcd.print(text);
}

void displayDrawSmallTextColor(int x, int y, const char* text, uint16_t fg, uint16_t bg) {
  lcd.setTextColor(fg, bg);
  lcd.setTextSize(1);
  lcd.setCursor(x, y);
  lcd.print(text);
}

void displayDrawCenteredText(int y, const char* text) {
  lcd.setTextSize(2);
  int w = lcd.textWidth(text);
  int x = (lcd.width() - w) / 2;
  if (x < 0) x = 0;
  displayDrawText(x, y, text);
}

void displayFillRect(int x, int y, int w, int h, uint16_t color) {
  lcd.fillRect(x, y, w, h, color);
}

void displayDrawRect(int x, int y, int w, int h, uint16_t color) {
  lcd.drawRect(x, y, w, h, color);
}

int displayWidth() {
  return lcd.width();
}

int displayHeight() {
  return lcd.height();
}

void displaySetBrightness(uint8_t value) {
  lcd.setBrightness(value);
}

void displayColorBarsTest() {
  displayClear();

  int w = lcd.width() / 4;
  int h = lcd.height() / 2;

  lcd.fillRect(0 * w, 0, w, h, TFT_RED);
  lcd.fillRect(1 * w, 0, w, h, TFT_GREEN);
  lcd.fillRect(2 * w, 0, w, h, TFT_BLUE);
  lcd.fillRect(3 * w, 0, w, h, TFT_YELLOW);

  displayDrawText(10, h + 20,  "R = rouge");
  displayDrawText(10, h + 50,  "G = vert");
  displayDrawText(10, h + 80,  "B = bleu");
  displayDrawText(10, h + 110, "Y = jaune");
}

bool displayDrawRAW(const char* path, int x, int y, int width, int height) {
  Serial.print("[RAW] ouverture: ");
  Serial.println(path);

  if (width <= 0 || height <= 0) {
    Serial.println("[RAW] dimensions invalides");
    return false;
  }

  File rawFile = SD.open(path, FILE_READ);
  if (!rawFile) {
    Serial.println("[RAW] fichier introuvable");
    return false;
  }

  size_t expectedSize = (size_t)width * (size_t)height * 2;
  size_t fileSize = (size_t)rawFile.size();
  if (fileSize != expectedSize) {
    Serial.print("[RAW] taille fichier invalide. attendu=");
    Serial.print(expectedSize);
    Serial.print(" octets, recu=");
    Serial.println(fileSize);
    rawFile.close();
    return false;
  }

  if (x >= lcd.width() || y >= lcd.height()) {
    Serial.println("[RAW] image hors ecran");
    rawFile.close();
    return false;
  }

  int drawWidth = width;
  int drawHeight = height;

  if (x + drawWidth > lcd.width())   drawWidth = lcd.width() - x;
  if (y + drawHeight > lcd.height()) drawHeight = lcd.height() - y;

  if (drawWidth <= 0 || drawHeight <= 0) {
    Serial.println("[RAW] zone visible nulle");
    rawFile.close();
    return false;
  }

  uint16_t* lineBuffer = (uint16_t*)malloc(drawWidth * sizeof(uint16_t));
  if (!lineBuffer) {
    Serial.println("[RAW] allocation memoire echouee");
    rawFile.close();
    return false;
  }

  lcd.setSwapBytes(true);

  for (int row = 0; row < drawHeight; row++) {
    for (int col = 0; col < drawWidth; col++) {
      uint8_t hi, lo;
      if (rawFile.read(&hi, 1) != 1 || rawFile.read(&lo, 1) != 1) {
        Serial.println("[RAW] lecture pixel impossible");
        lcd.setSwapBytes(false);
        free(lineBuffer);
        rawFile.close();
        return false;
      }
      lineBuffer[col] = ((uint16_t)hi << 8) | (uint16_t)lo;
    }

    if (width > drawWidth) {
      rawFile.seek(rawFile.position() + (width - drawWidth) * 2);
    }

    lcd.pushImage(x, y + row, drawWidth, 1, lineBuffer);
  }

  lcd.setSwapBytes(false);

  free(lineBuffer);
  rawFile.close();

  Serial.println("[RAW] affiche OK");
  return true;
}

bool displayDrawBMP(const char* path, int x, int y) {
  Serial.print("[BMP] ouverture: ");
  Serial.println(path);

  File bmpFile = SD.open(path, FILE_READ);
  if (!bmpFile) {
    Serial.println("[BMP] fichier introuvable");
    return false;
  }

  uint8_t sig[2];
  if (bmpFile.read(sig, 2) != 2) {
    Serial.println("[BMP] lecture signature impossible");
    bmpFile.close();
    return false;
  }

  if (sig[0] != 'B' || sig[1] != 'M') {
    Serial.println("[BMP] signature invalide");
    bmpFile.close();
    return false;
  }

  uint32_t fileSize;
  uint16_t reserved1, reserved2;
  uint32_t dataOffset;

  if (!readLE32(bmpFile, fileSize) ||
      !readLE16(bmpFile, reserved1) ||
      !readLE16(bmpFile, reserved2) ||
      !readLE32(bmpFile, dataOffset)) {
    Serial.println("[BMP] entete fichier invalide");
    bmpFile.close();
    return false;
  }

  uint32_t dibSize;
  if (!readLE32(bmpFile, dibSize)) {
    Serial.println("[BMP] DIB header invalide");
    bmpFile.close();
    return false;
  }

  if (dibSize < 40) {
    Serial.println("[BMP] DIB header non supporte");
    bmpFile.close();
    return false;
  }

  uint32_t bmpWidthU, bmpHeightU;
  uint16_t planes, depth;
  uint32_t compression;

  if (!readLE32(bmpFile, bmpWidthU) ||
      !readLE32(bmpFile, bmpHeightU) ||
      !readLE16(bmpFile, planes) ||
      !readLE16(bmpFile, depth) ||
      !readLE32(bmpFile, compression)) {
    Serial.println("[BMP] lecture entete image invalide");
    bmpFile.close();
    return false;
  }

  int32_t bmpWidth = (int32_t)bmpWidthU;
  int32_t bmpHeight = (int32_t)bmpHeightU;

  bmpFile.seek(14 + dibSize);

  if (planes != 1 || depth != 24 || compression != 0) {
    Serial.println("[BMP] format BMP non supporte");
    bmpFile.close();
    return false;
  }

  bool flip = true;
  if (bmpHeight < 0) {
    bmpHeight = -bmpHeight;
    flip = false;
  }

  const uint32_t rowSize = (bmpWidth * 3 + 3) & ~3;

  uint8_t* rowBuffer = (uint8_t*)malloc(rowSize);
  uint16_t* lineBuffer = (uint16_t*)malloc(bmpWidth * sizeof(uint16_t));

  if (!rowBuffer || !lineBuffer) {
    Serial.println("[BMP] allocation memoire echouee");
    if (rowBuffer) free(rowBuffer);
    if (lineBuffer) free(lineBuffer);
    bmpFile.close();
    return false;
  }

  for (int row = 0; row < bmpHeight; row++) {
    uint32_t fileRow = flip ? (bmpHeight - 1 - row) : row;
    uint32_t pos = dataOffset + fileRow * rowSize;

    if (!bmpFile.seek(pos)) {
      free(rowBuffer);
      free(lineBuffer);
      bmpFile.close();
      return false;
    }

    if (bmpFile.read(rowBuffer, rowSize) != (int)rowSize) {
      free(rowBuffer);
      free(lineBuffer);
      bmpFile.close();
      return false;
    }

    for (int col = 0; col < bmpWidth; col++) {
      uint8_t b = rowBuffer[col * 3 + 0];
      uint8_t g = rowBuffer[col * 3 + 1];
      uint8_t r = rowBuffer[col * 3 + 2];

      lineBuffer[col] = bmpColorTo565(b, r, g);
    }

    lcd.pushImage(x, y + row, bmpWidth, 1, lineBuffer);
  }

  free(rowBuffer);
  free(lineBuffer);
  bmpFile.close();

  return true;
}

bool displayDrawBMPScaled(const char* path, int x, int y, int maxWidth, int maxHeight) {
  File bmpFile = SD.open(path, FILE_READ);
  if (!bmpFile) {
    return false;
  }

  uint8_t sig[2];
  if (bmpFile.read(sig, 2) != 2 || sig[0] != 'B' || sig[1] != 'M') {
    bmpFile.close();
    return false;
  }

  bmpFile.seek(18);

  uint32_t wU, hU;
  if (!readLE32(bmpFile, wU) || !readLE32(bmpFile, hU)) {
    bmpFile.close();
    return false;
  }

  int32_t w = (int32_t)wU;
  int32_t h = (int32_t)hU;
  if (h < 0) h = -h;

  bmpFile.close();

  if (w <= maxWidth && h <= maxHeight) {
    return displayDrawBMP(path, x, y);
  }

  return false;
}
