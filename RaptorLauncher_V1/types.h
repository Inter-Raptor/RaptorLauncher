#pragma once
#include <Arduino.h>

enum GameButton {
  BTN_UP,
  BTN_DOWN,
  BTN_LEFT,
  BTN_RIGHT,
  BTN_A,
  BTN_B,
  BTN_X,
  BTN_Y,
  BTN_START,
  BTN_SELECT,
  BTN_COUNT
};

struct InputState {
  bool held[BTN_COUNT];
  bool pressed[BTN_COUNT];
  bool released[BTN_COUNT];
};

struct GameInfo {
  String folder;
  String name;
  String author;
  String description;
};