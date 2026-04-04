#pragma once
#include <Arduino.h>

struct SystemSettings {
  int volume;       // 0 à 100
  int brightness;   // 0 à 100

  String wifi_ssid;
  String wifi_pass;

  int touch_x_min;
  int touch_x_max;
  int touch_y_min;
  int touch_y_max;
  int touch_offset_x;
  int touch_offset_y;
};

bool settingsInit();
SystemSettings& settingsGet();
bool settingsSave();
void settingsSetDefaults();
