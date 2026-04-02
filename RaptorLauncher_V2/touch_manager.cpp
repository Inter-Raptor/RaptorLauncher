#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "touch_manager.h"

// Broches tactiles pour ESP32-2432S028
static const int TOUCH_CS   = 33;
static const int TOUCH_IRQ  = 36;
static const int TOUCH_CLK  = 25;
static const int TOUCH_MISO = 39;
static const int TOUCH_MOSI = 32;

SPIClass touchSPI(HSPI);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

// Valeurs de calibration de départ
static const int RAW_X_MIN = 200;
static const int RAW_X_MAX = 3800;
static const int RAW_Y_MIN = 200;
static const int RAW_Y_MAX = 3800;

// Taille écran paysage
static const int SCREEN_W = 320;
static const int SCREEN_H = 240;

static int mapAndClamp(int v, int inMin, int inMax, int outMin, int outMax) {
  long r = map(v, inMin, inMax, outMin, outMax);

  long minOut = outMin < outMax ? outMin : outMax;
  long maxOut = outMin > outMax ? outMin : outMax;

  if (r < minOut) r = minOut;
  if (r > maxOut) r = maxOut;

  return (int)r;
}

void touchInit() {
  Serial.println("[TOUCH] init debut");

  touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  ts.begin(touchSPI);
  ts.setRotation(1);

  Serial.println("[TOUCH] init fin");
}

bool touchPressed(int &x, int &y) {
  if (!ts.touched()) {
    return false;
  }

  TS_Point p = ts.getPoint();

  Serial.print("[TOUCH RAW] x=");
  Serial.print(p.x);
  Serial.print(" y=");
  Serial.println(p.y);

  // X inversé
  x = mapAndClamp(p.x, RAW_X_MIN, RAW_X_MAX, SCREEN_W - 1, 0);

  // Y inversé
  y = mapAndClamp(p.y, RAW_Y_MIN, RAW_Y_MAX, SCREEN_H - 1, 0);

  Serial.print("[TOUCH MAP] x=");
  Serial.print(x);
  Serial.print(" y=");
  Serial.println(y);

  return true;
}