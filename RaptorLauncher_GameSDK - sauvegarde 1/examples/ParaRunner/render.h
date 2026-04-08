#pragma once

#include <Arduino.h>
#include "config.h"
#include "game_state.h"
#include "sprites.h"

bool pointInRect(int px, int py, const ButtonRect& r);

void drawSpriteKey(int x, int y, const SpriteFrame& s);
void drawSpriteKeyFlipH(int x, int y, const SpriteFrame& s);
void drawFilledCircle(int cx, int cy, int r, uint16_t col);

void drawButton(const ButtonRect& r, uint16_t fill, uint16_t border, const char* txt, uint16_t fg = COL_WHITE, bool selected = false);

void drawMenuScreen();
void drawGameScreen();
void drawGameOverScreen();