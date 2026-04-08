#include "game_state.h"

RaptorGameSDK sdk;

AppState appState = STATE_MENU;

float playerY = PLAYER_STAND_Y;
float playerVelY = 0.0f;
bool onGround = true;
bool ducking = false;
bool duckingPrev = false;

uint32_t score = 0;
uint32_t bestScore = 0;

float gameSpeed = 5.4f;
uint8_t lives = MAX_LIVES;
unsigned long invulnUntil = 0;

Obstacle obstacles[MAX_OBSTACLES];

unsigned long lastAnimMs = 0;
bool animToggle = false;

unsigned long lastSpawnMs = 0;
unsigned long nextSpawnDelayMs = 2000;
unsigned long lastScoreTickMs = 0;

bool jumpHeldPrev = false;

int menuSelection = MENU_PLAY;
int gameOverSelection = GAMEOVER_RETRY;

ButtonRect btnPlay   = { 70, 108, 180, 46 };
ButtonRect btnLaunch = { 70, 164, 180, 34 };
ButtonRect btnRetry  = { 58, 150, 92, 34 };
ButtonRect btnMenu   = { 170, 150, 92, 34 };