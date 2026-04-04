#include "touch_manager.h"
#include "settings_manager.h"
#include <XPT2046_Touchscreen.h>

#define TOUCH_CS 33
#define TOUCH_IRQ 36

XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

int gTouchX = 0;
int gTouchY = 0;
bool gTouchPressed = false;

// calibration
int touch_x_min = 200;
int touch_x_max = 3800;
int touch_y_min = 200;
int touch_y_max = 3800;

void touchInit() {
  Serial.println("[TOUCH] init debut");
  touch.begin();
  Serial.println("[TOUCH] init fin");
}

void touchUpdate() {
  if (touch.touched()) {
    TS_Point p = touch.getPoint();

    // mapping
    gTouchX = map(p.x, touch_x_min, touch_x_max, 0, 320);
    gTouchY = map(p.y, touch_y_min, touch_y_max, 0, 240);

    gTouchPressed = true;

  } else {
    gTouchPressed = false;
  }
}