#pragma once
#include <Arduino.h>

struct SystemSettings {
  int volume;       // 0 à 100
  int brightness;   // 0 à 100
  int boot_splash_ms; // 0 à 10000

  String wifi_ssid;
  String wifi_pass;
  String boot_splash_raw; // image RAW plein ecran au demarrage
  String home_bg_raw;     // image RAW fond ecran accueil

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
