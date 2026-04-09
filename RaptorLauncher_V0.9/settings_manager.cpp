#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "settings_manager.h"

static const char* SETTINGS_PATH = "/settings.json";
static const char* SETTINGS_TMP_PATH = "/settings.tmp";
static SystemSettings gSettings;

static int clampValue(int v, int minV, int maxV) {
  if (v < minV) return minV;
  if (v > maxV) return maxV;
  return v;
}

void settingsSetDefaults() {
  gSettings.volume = 80;
  gSettings.brightness = 100;
  gSettings.boot_splash_ms = 1500;
  gSettings.boot_splash_w = 320;
  gSettings.boot_splash_h = 240;

  gSettings.wifi_ssid = "";
  gSettings.wifi_pass = "";
  gSettings.boot_splash_raw = "";
  gSettings.home_bg_raw = "";
  gSettings.home_bg_w = 320;
  gSettings.home_bg_h = 240;

  gSettings.touch_x_min = 200;
  gSettings.touch_x_max = 3800;
  gSettings.touch_y_min = 200;
  gSettings.touch_y_max = 3800;
  gSettings.touch_offset_x = 0;
  gSettings.touch_offset_y = 0;
  gSettings.led_brightness = 40;
  gSettings.language = 0;
}

SystemSettings& settingsGet() {
  return gSettings;
}

bool settingsSave() {
  JsonDocument doc;
  doc["volume"] = gSettings.volume;
  doc["brightness"] = gSettings.brightness;
  doc["boot_splash_ms"] = gSettings.boot_splash_ms;
  doc["boot_splash_w"] = gSettings.boot_splash_w;
  doc["boot_splash_h"] = gSettings.boot_splash_h;
  doc["wifi_ssid"] = gSettings.wifi_ssid;
  doc["wifi_pass"] = gSettings.wifi_pass;
  doc["boot_splash_raw"] = gSettings.boot_splash_raw;
  doc["home_bg_raw"] = gSettings.home_bg_raw;
  doc["home_bg_w"] = gSettings.home_bg_w;
  doc["home_bg_h"] = gSettings.home_bg_h;
  doc["touch_x_min"] = gSettings.touch_x_min;
  doc["touch_x_max"] = gSettings.touch_x_max;
  doc["touch_y_min"] = gSettings.touch_y_min;
  doc["touch_y_max"] = gSettings.touch_y_max;
  doc["touch_offset_x"] = gSettings.touch_offset_x;
  doc["touch_offset_y"] = gSettings.touch_offset_y;
  doc["led_brightness"] = gSettings.led_brightness;
  doc["language"] = gSettings.language;

  if (SD.exists(SETTINGS_TMP_PATH)) {
    SD.remove(SETTINGS_TMP_PATH);
  }

  File f = SD.open(SETTINGS_TMP_PATH, FILE_WRITE);
  if (!f) {
    Serial.println("[SETTINGS] impossible d'ouvrir settings.tmp en ecriture");
    return false;
  }

  if (serializeJsonPretty(doc, f) == 0) {
    Serial.println("[SETTINGS] echec ecriture JSON");
    f.close();
    return false;
  }

  f.close();

  if (SD.exists(SETTINGS_PATH) && !SD.remove(SETTINGS_PATH)) {
    Serial.println("[SETTINGS] impossible de supprimer ancien settings.json");
    return false;
  }

  if (!SD.rename(SETTINGS_TMP_PATH, SETTINGS_PATH)) {
    Serial.println("[SETTINGS] impossible de renommer settings.tmp -> settings.json");
    return false;
  }

  Serial.println("[SETTINGS] sauvegarde OK");
  return true;
}

bool settingsInit() {
  Serial.println("[SETTINGS] init");

  settingsSetDefaults();

  if (!SD.exists(SETTINGS_PATH)) {
    Serial.println("[SETTINGS] fichier absent, creation par defaut");
    return settingsSave();
  }

  File f = SD.open(SETTINGS_PATH, FILE_READ);
  if (!f) {
    Serial.println("[SETTINGS] impossible d'ouvrir settings.json");
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();

  if (err) {
    Serial.print("[SETTINGS] JSON invalide: ");
    Serial.println(err.c_str());
    settingsSetDefaults();
    return settingsSave();
  }

  gSettings.volume = clampValue(doc["volume"] | 80, 0, 100);
  gSettings.brightness = clampValue(doc["brightness"] | 100, 0, 100);
  gSettings.boot_splash_ms = clampValue(doc["boot_splash_ms"] | 1500, 0, 10000);
  gSettings.boot_splash_w = clampValue(doc["boot_splash_w"] | 320, 1, 1024);
  gSettings.boot_splash_h = clampValue(doc["boot_splash_h"] | 240, 1, 1024);

  gSettings.wifi_ssid = doc["wifi_ssid"] | "";
  gSettings.wifi_pass = doc["wifi_pass"] | "";
  gSettings.boot_splash_raw = doc["boot_splash_raw"] | "";
  gSettings.home_bg_raw = doc["home_bg_raw"] | "";
  gSettings.home_bg_w = clampValue(doc["home_bg_w"] | 320, 1, 1024);
  gSettings.home_bg_h = clampValue(doc["home_bg_h"] | 240, 1, 1024);

  gSettings.touch_x_min = doc["touch_x_min"] | 200;
  gSettings.touch_x_max = doc["touch_x_max"] | 3800;
  gSettings.touch_y_min = doc["touch_y_min"] | 200;
  gSettings.touch_y_max = doc["touch_y_max"] | 3800;
  gSettings.touch_offset_x = clampValue(doc["touch_offset_x"] | 0, -80, 80);
  gSettings.touch_offset_y = clampValue(doc["touch_offset_y"] | 0, -80, 80);
  gSettings.led_brightness = clampValue(doc["led_brightness"] | 40, 0, 100);
  gSettings.language = clampValue(doc["language"] | 0, 0, 4);

  Serial.print("[SETTINGS] volume = ");
  Serial.println(gSettings.volume);

  Serial.print("[SETTINGS] brightness = ");
  Serial.println(gSettings.brightness);
  Serial.print("[SETTINGS] boot_splash_ms = ");
  Serial.println(gSettings.boot_splash_ms);
  Serial.print("[SETTINGS] boot_splash_w = ");
  Serial.println(gSettings.boot_splash_w);
  Serial.print("[SETTINGS] boot_splash_h = ");
  Serial.println(gSettings.boot_splash_h);

  Serial.print("[SETTINGS] wifi_ssid = ");
  Serial.println(gSettings.wifi_ssid);
  Serial.print("[SETTINGS] boot_splash_raw = ");
  Serial.println(gSettings.boot_splash_raw);
  Serial.print("[SETTINGS] home_bg_raw = ");
  Serial.println(gSettings.home_bg_raw);
  Serial.print("[SETTINGS] home_bg_w = ");
  Serial.println(gSettings.home_bg_w);
  Serial.print("[SETTINGS] home_bg_h = ");
  Serial.println(gSettings.home_bg_h);

  Serial.print("[SETTINGS] touch_x_min = ");
  Serial.println(gSettings.touch_x_min);
  Serial.print("[SETTINGS] touch_x_max = ");
  Serial.println(gSettings.touch_x_max);
  Serial.print("[SETTINGS] touch_y_min = ");
  Serial.println(gSettings.touch_y_min);
  Serial.print("[SETTINGS] touch_y_max = ");
  Serial.println(gSettings.touch_y_max);
  Serial.print("[SETTINGS] touch_offset_x = ");
  Serial.println(gSettings.touch_offset_x);
  Serial.print("[SETTINGS] touch_offset_y = ");
  Serial.println(gSettings.touch_offset_y);
  Serial.print("[SETTINGS] led_brightness = ");
  Serial.println(gSettings.led_brightness);
  Serial.print("[SETTINGS] language = ");
  Serial.println(gSettings.language);

  return true;
}
