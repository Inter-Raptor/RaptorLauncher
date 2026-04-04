#pragma once

extern int gTouchX;
extern int gTouchY;
extern bool gTouchPressed;

extern int touch_x_min;
extern int touch_x_max;
extern int touch_y_min;
extern int touch_y_max;

void touchInit();
void touchUpdate();