#pragma once
#include <stdint.h>

void ledManagerInit();
void ledManagerSetBrightness(int percent);
void ledManagerSetColor(uint8_t r, uint8_t g, uint8_t b);
void ledManagerOff();
