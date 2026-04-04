#pragma once
#include <stdint.h>
#include <stdbool.h>

void displayInit();
void displayClear();
void displayFillScreen(uint16_t color);

void displayDrawText(int x, int y, const char* text);
void displayDrawSmallText(int x, int y, const char* text);
void displayDrawSmallTextColor(int x, int y, const char* text, uint16_t fg, uint16_t bg);
void displayDrawCenteredText(int y, const char* text);

void displayFillRect(int x, int y, int w, int h, uint16_t color);
void displayDrawRect(int x, int y, int w, int h, uint16_t color);

bool displayDrawRAW(const char* path, int x, int y, int width, int height);
bool displayDrawBMP(const char* path, int x, int y);
bool displayDrawBMPScaled(const char* path, int x, int y, int maxWidth, int maxHeight);

void displayColorBarsTest();

int displayWidth();
int displayHeight();
void displaySetBrightness(uint8_t value);