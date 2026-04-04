#pragma once
#include <stdint.h>

bool mcp23017Init();
bool mcp23017IsPresent();
void mcp23017Read(uint8_t &gpioA, uint8_t &gpioB);
