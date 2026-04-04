#include <Arduino.h>
#include "led_manager.h"

// ESP32-2432S032R: LED RGB (WS2812) généralement sur GPIO 4
static const int LED_PIN = 4;

static int gLedBrightness = 40; // 0..100
static uint8_t gBaseR = 255;
static uint8_t gBaseG = 255;
static uint8_t gBaseB = 255;

static uint8_t scaleToBrightness(uint8_t c) {
  return (uint8_t)((c * gLedBrightness) / 100);
}

static void applyLed() {
  neopixelWrite(LED_PIN, scaleToBrightness(gBaseR), scaleToBrightness(gBaseG), scaleToBrightness(gBaseB));
}

void ledManagerInit() {
  pinMode(LED_PIN, OUTPUT);
  ledManagerOff();
}

void ledManagerSetBrightness(int percent) {
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  gLedBrightness = percent;
  applyLed();
}

void ledManagerSetColor(uint8_t r, uint8_t g, uint8_t b) {
  gBaseR = r;
  gBaseG = g;
  gBaseB = b;
  applyLed();
}

void ledManagerOff() {
  gBaseR = 0;
  gBaseG = 0;
  gBaseB = 0;
  neopixelWrite(LED_PIN, 0, 0, 0);
}
