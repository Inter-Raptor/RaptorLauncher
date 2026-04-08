#pragma once
#include <Arduino.h>
#include "raptor_game_sdk.h"
#include "config.h"

enum AppState {
  STATE_MENU,
  STATE_PLAYING,
  STATE_GAME_OVER
};

enum ObstacleType {
  OBS_CACTUS,
  OBS_PTERA
};

enum MenuSelection {
  MENU_PLAY = 0,
  MENU_LAUNCHER = 1
};

enum GameOverSelection {
  GAMEOVER_RETRY = 0,
  GAMEOVER_MENU = 1
};

struct ButtonRect {
  int x;
  int y;
  int w;
  int h;
};

struct Obstacle {
  bool active = false;
  ObstacleType type = OBS_CACTUS;
  int variant = 0;
  float x = 0;
  float y = 0;
  int w = 0;
  int h = 0;
  bool counted = false;
};

extern RaptorGameSDK sdk;

extern AppState appState;

extern float playerY;
extern float playerVelY;
extern bool onGround;
extern bool ducking;
extern bool duckingPrev;

extern uint32_t score;
extern uint32_t bestScore;

extern float gameSpeed;
extern uint8_t lives;
extern unsigned long invulnUntil;

extern Obstacle obstacles[MAX_OBSTACLES];

extern unsigned long lastAnimMs;
extern bool animToggle;

extern unsigned long lastSpawnMs;
extern unsigned long nextSpawnDelayMs;
extern unsigned long lastScoreTickMs;

extern bool jumpHeldPrev;

extern int menuSelection;
extern int gameOverSelection;

extern ButtonRect btnPlay;
extern ButtonRect btnLaunch;
extern ButtonRect btnRetry;
extern ButtonRect btnMenu;