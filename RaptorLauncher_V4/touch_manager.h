#pragma once

extern int gTouchX;
extern int gTouchY;
extern int gTouchRawX;
extern int gTouchRawY;
extern bool gTouchPressed;

extern int touch_x_min;
extern int touch_x_max;
extern int touch_y_min;
extern int touch_y_max;
extern int touch_offset_x;
extern int touch_offset_y;

void touchInit();
void touchUpdate();
