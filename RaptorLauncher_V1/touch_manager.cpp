#include <Arduino.h>
#include "touch_manager.h"

void touchInit() {
  Serial.println("[TOUCH] init");
}

bool touchPressed(int &x, int &y) {
  x = 0;
  y = 0;
  return false;
}