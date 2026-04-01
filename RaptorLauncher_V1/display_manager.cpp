#include <Arduino.h>
#include "display_manager.h"

void displayInit() {
  Serial.println("[DISPLAY] init debut");
  Serial.println("[DISPLAY] init fin");
}

void displayClear() {
}

void displayDrawText(int x, int y, const char* text) {
  Serial.print("[DISPLAY TEXT] ");
  Serial.println(text);
}

void displayFillScreen(uint16_t color) {
}