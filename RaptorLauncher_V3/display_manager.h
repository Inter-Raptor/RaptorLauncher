#pragma once
#include <stdint.h>
#include <stdbool.h>

void displayInit();
void displayClear();
void displayFillScreen(uint16_t color);
void displayDrawText(int x, int y, const char* text);
void displayDrawCenteredText(int y, const char* text);

bool displayDrawBMP(const char* path, int x, int y);
bool displayDrawBMPScaled(const char* path, int x, int y, int maxWidth, int maxHeight);
bool displayDrawRAW(const char* path, int x, int y, int width, int height);

void displayColorBarsTest();

int displayWidth();
int displayHeight();
void displaySetBrightness(uint8_t value);