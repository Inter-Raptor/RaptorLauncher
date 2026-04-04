#include <Arduino.h>
#include <Wire.h>
#include "mcp23017_manager.h"

static const uint8_t MCP_ADDR = 0x20;
static bool gPresent = false;

static bool writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MCP_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static bool readReg(uint8_t reg, uint8_t &value) {
  Wire.beginTransmission(MCP_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)MCP_ADDR, 1) != 1) return false;
  value = Wire.read();
  return true;
}

bool mcp23017Init() {
  Wire.begin(21, 22);
  uint8_t iodira = 0;
  gPresent = readReg(0x00, iodira);
  if (!gPresent) return false;

  // GPA/GPB en entree + pull-up interne
  if (!writeReg(0x00, 0xFF)) return false; // IODIRA
  if (!writeReg(0x01, 0xFF)) return false; // IODIRB
  if (!writeReg(0x0C, 0xFF)) return false; // GPPUA
  if (!writeReg(0x0D, 0xFF)) return false; // GPPUB
  gPresent = true;
  return true;
}

bool mcp23017IsPresent() {
  return gPresent;
}

void mcp23017Read(uint8_t &gpioA, uint8_t &gpioB) {
  gpioA = 0xFF;
  gpioB = 0xFF;
  if (!gPresent) return;
  if (!readReg(0x12, gpioA)) gPresent = false; // GPIOA
  if (!readReg(0x13, gpioB)) gPresent = false; // GPIOB
}
