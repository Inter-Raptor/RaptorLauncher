#include <Arduino.h>
#include "led_manager.h"

// ESP32-2432S032R: LED RGB discrete commune anode
// Schéma validé: R=IO4, G=IO16, B=IO17
static const int LED_PIN_R = 4;
static const int LED_PIN_G = 16;
static const int LED_PIN_B = 17;
static const int LED_PWM_FREQ = 5000;
static const int LED_PWM_RES = 8; // 0..255

static int gLedBrightness = 40; // 0..100
static uint8_t gBaseR = 255;
static uint8_t gBaseG = 255;
static uint8_t gBaseB = 255;

static uint8_t scaleToBrightness(uint8_t c) {
  return (uint8_t)((c * gLedBrightness) / 100);
}

// LED commune anode => actif à 0 (inversion PWM)
static uint8_t toPwmActiveLow(uint8_t c) {
  return 255 - c;
}

static void applyLed() {
  uint8_t r = scaleToBrightness(gBaseR);
  uint8_t g = scaleToBrightness(gBaseG);
  uint8_t b = scaleToBrightness(gBaseB);

  ledcWrite(LED_PIN_R, toPwmActiveLow(r));
  ledcWrite(LED_PIN_G, toPwmActiveLow(g));
  ledcWrite(LED_PIN_B, toPwmActiveLow(b));
}

void ledManagerInit() {
  ledcAttach(LED_PIN_R, LED_PWM_FREQ, LED_PWM_RES);
  ledcAttach(LED_PIN_G, LED_PWM_FREQ, LED_PWM_RES);
  ledcAttach(LED_PIN_B, LED_PWM_FREQ, LED_PWM_RES);
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
  ledcWrite(LED_PIN_R, 255);
  ledcWrite(LED_PIN_G, 255);
  ledcWrite(LED_PIN_B, 255);
}
