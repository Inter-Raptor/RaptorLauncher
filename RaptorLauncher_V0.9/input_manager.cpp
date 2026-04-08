#include <Arduino.h>
#include <string.h>
#include "input_manager.h"

static InputState gInput;

void inputInit() {
  memset(&gInput, 0, sizeof(gInput));
  Serial.println("[INPUT] init");
}

void inputUpdate() {
  // Plus tard: fusion tactile / I2C / BT
  for (int i = 0; i < BTN_COUNT; i++) {
    gInput.pressed[i] = false;
    gInput.released[i] = false;
  }
}

const InputState& inputGetState() {
  return gInput;
}