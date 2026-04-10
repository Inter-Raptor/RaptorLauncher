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
  int led_brightness;
  int language; // 0=EN,1=FR,2=ES,3=DE,4=IT
};

bool settingsInit();
SystemSettings& settingsGet();
bool settingsSave();
void settingsSetDefaults();
