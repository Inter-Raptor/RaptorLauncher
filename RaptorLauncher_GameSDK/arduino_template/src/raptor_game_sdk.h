#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <XPT2046_Touchscreen.h>
#include <Adafruit_MCP23X17.h>
#include <LovyanGFX.hpp>
#include <ArduinoJson.h>
#include "raptor_game_config.h"

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

class RaptorGameSDK {
public:
  void begin();
  void updateInputs();

  bool isHeld(GameButton b) const;
  bool isPressed(GameButton b) const;
  bool isReleased(GameButton b) const;

  // Tactile
  bool isTouchHeld() const;
  bool isTouching() const; // alias clair
  bool isTouchPressed() const;
  bool isTouchReleased() const;
  int touchX() const;
  int touchY() const;

  int width() const;
  int height() const;

  void clear(uint16_t color = SDK_COLOR_BG);
  void drawRect(int x, int y, int w, int h, uint16_t color);
  void fillRect(int x, int y, int w, int h, uint16_t color);
  void drawSmallText(int x, int y, const char* text, uint16_t fg = SDK_COLOR_TEXT, uint16_t bg = SDK_COLOR_BG);
  void drawCenteredText(int y, const char* text, uint16_t fg = SDK_COLOR_TEXT, uint16_t bg = SDK_COLOR_BG);

  void playBeep(int freq = 1200, int durationMs = 80);

  // Armement securite: au boot du jeu, le prochain redemarrage revient sur launcher.
  bool armReturnToLauncherOnNextBoot();
  // Retour immediat volontaire vers launcher.
  void requestReturnToLauncher();

  // Helpers de chemin lies au dossier de jeu.
  String gameRootPath() const;
  String saveJsonPath() const;

  // Persistance JSON standard dans /games/<jeu>/sauv.json
  bool saveJson(const JsonDocument& doc);
  bool loadJson(JsonDocument& doc);

private:
  void initDisplay();
  void initInput();
  void mapButtonsFromMcp(uint8_t gpioA, uint8_t gpioB);

  InputState input{};
  bool mcpReady = false;

  bool touchHeld = false;
  bool touchPressedEdge = false;
  bool touchReleasedEdge = false;
  int touchPx = 0;
  int touchPy = 0;

  SPIClass sdSPI = SPIClass(HSPI);
  XPT2046_Touchscreen touch = XPT2046_Touchscreen(SDK_PIN_TOUCH_CS, SDK_PIN_TOUCH_IRQ);
  Adafruit_MCP23X17 mcp;
};
