#pragma once
#include <Arduino.h>

struct SystemSettings {
  int volume;       // 0 à 100
  int brightness;   // 0 à 100
};

bool settingsInit();
SystemSettings& settingsGet();
bool settingsSave();
void settingsSetDefaults();