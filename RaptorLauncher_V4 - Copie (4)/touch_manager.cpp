#include "touch_manager.h"
#include "settings_manager.h"
#include <XPT2046_Touchscreen.h>

#define TOUCH_CS 33
#define TOUCH_IRQ 36

XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

int gTouchX = 0;
int gTouchY = 0;
int gTouchRawX = 0;
int gTouchRawY = 0;
bool gTouchPressed = false;

// calibration
int touch_x_min = 200;
int touch_x_max = 3800;
int touch_y_min = 200;
int touch_y_max = 3800;
int touch_offset_x = 0;
int touch_offset_y = 0;

void touchInit() {
  Serial.println("[TOUCH] init debut");
  touch.begin();

  // recharge les valeurs depuis settings.json
  touch_x_min = settingsGet().touch_x_min;
  touch_x_max = settingsGet().touch_x_max;
  touch_y_min = settingsGet().touch_y_min;
  touch_y_max = settingsGet().touch_y_max;
  touch_offset_x = settingsGet().touch_offset_x;
  touch_offset_y = settingsGet().touch_offset_y;

  Serial.println("[TOUCH] init fin");
}

void touchUpdate() {
  if (touch.touched()) {
    TS_Point p = touch.getPoint();
    gTouchRawX = p.x;
    gTouchRawY = p.y;

    gTouchX = map(p.x, touch_x_min, touch_x_max, 0, 319);
    gTouchY = map(p.y, touch_y_min, touch_y_max, 239, 0);
    gTouchX += touch_offset_x;
    gTouchY += touch_offset_y;

    if (gTouchX < 0) gTouchX = 0;
    if (gTouchX > 319) gTouchX = 319;
    if (gTouchY < 0) gTouchY = 0;
    if (gTouchY > 239) gTouchY = 239;

    gTouchPressed = true;
  } else {
    gTouchPressed = false;
  }
}