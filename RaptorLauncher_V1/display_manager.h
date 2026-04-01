#pragma once
#include <stdint.h>

void displayInit();
void displayClear();
void displayDrawText(int x, int y, const char* text);
void displayFillScreen(uint16_t color);