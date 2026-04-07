#pragma once
#include <Arduino.h>

static const int SCREEN_W = 320;
static const int SCREEN_H = 240;

static const int GROUND_TOP_Y  = 184;
static const int GROUND_FILL_Y = 190;

static const int PLAYER_X = 42;
static const float PLAYER_STAND_Y = 134.0f;
static const float PLAYER_DUCK_Y  = 146.0f;

static const float GRAVITY = 0.88f;
static const float JUMP_VELOCITY = -12.7f;

static const int MAX_OBSTACLES = 2;

static const uint8_t MAX_LIVES = 3;
static const unsigned long HIT_INVULN_MS = 900;

// Couleurs RGB565
static const uint16_t COL_SKY_1     = 0x9DFF;
static const uint16_t COL_SKY_2     = 0xB75F;
static const uint16_t COL_SUN       = 0xFEA0;
static const uint16_t COL_SUN_2     = 0xFD20;
static const uint16_t COL_GROUND    = 0xC3A0;
static const uint16_t COL_GROUND_2  = 0xFC60;
static const uint16_t COL_PANEL     = 0x18E3;
static const uint16_t COL_WHITE     = 0xFFFF;
static const uint16_t COL_BLACK     = 0x0000;
static const uint16_t COL_RED       = 0xF800;
static const uint16_t COL_GREEN     = 0x07E0;
static const uint16_t COL_BLUE      = 0x001F;
static const uint16_t COL_YELLOW    = 0xFFE0;
static const uint16_t COL_PURPLE    = 0x881F;
static const uint16_t COL_ORANGE    = 0xFD20;
