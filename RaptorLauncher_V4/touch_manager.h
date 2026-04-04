#pragma once

void touchInit();
bool touchPressed(int &x, int &y);

// brut
bool touchReadRaw(int &x, int &y, int &z);

// calibration
void touchSetCalibration(int xmin, int xmax, int ymin, int ymax);
void touchGetCalibration(int &xmin, int &xmax, int &ymin, int &ymax);
void touchResetCalibration();