#include <Arduino.h>
#include <XPT2046_Touchscreen.h>
#include "touch_manager.h"

// Broches tactiles pour ESP32-2432S028
// D'après ta base JurassicLifeFR
static const int TOUCH_CS  = 33;
static const int TOUCH_IRQ = 36;
static const int TOUCH_CLK = 25;
static const int TOUCH_MISO = 39;
static const int TOUCH_MOSI = 32;

SPIClass touchSPI(HSPI);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

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

  x = p.x;
  y = p.y;

  return true;
}