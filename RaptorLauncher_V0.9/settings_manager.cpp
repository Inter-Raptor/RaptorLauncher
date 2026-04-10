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

  gSettings.wifi_ssid = "";
  gSettings.wifi_pass = "";

  gSettings.touch_x_min = 200;
  gSettings.touch_x_max = 3800;
  gSettings.touch_y_min = 200;
  gSettings.touch_y_max = 3800;
  gSettings.touch_offset_x = 0;
  gSettings.touch_offset_y = 0;
  gSettings.led_brightness = 40;
  gSettings.language = 0;
  gSettings.home_text_color_mode = 0;
  gSettings.home_text_color_hex = "#FFFFFF";
  gSettings.home_show_info_badge = true;
  gSettings.home_boot_image_ms = 3000;
}

SystemSettings& settingsGet() {
  return gSettings;
}

bool settingsSave() {
  JsonDocument doc;
  doc["volume"] = gSettings.volume;
  doc["brightness"] = gSettings.brightness;
  doc["wifi_ssid"] = gSettings.wifi_ssid;
  doc["wifi_pass"] = gSettings.wifi_pass;
  doc["touch_x_min"] = gSettings.touch_x_min;
  doc["touch_x_max"] = gSettings.touch_x_max;
  doc["touch_y_min"] = gSettings.touch_y_min;
  doc["touch_y_max"] = gSettings.touch_y_max;
  doc["touch_offset_x"] = gSettings.touch_offset_x;
  doc["touch_offset_y"] = gSettings.touch_offset_y;
  doc["led_brightness"] = gSettings.led_brightness;
  doc["language"] = gSettings.language;
  doc["home_text_color_mode"] = gSettings.home_text_color_mode;
  doc["home_text_color_hex"] = gSettings.home_text_color_hex;
  doc["home_show_info_badge"] = gSettings.home_show_info_badge;
  doc["home_boot_image_ms"] = gSettings.home_boot_image_ms;

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

  gSettings.wifi_ssid = doc["wifi_ssid"] | "";
  gSettings.wifi_pass = doc["wifi_pass"] | "";

  gSettings.touch_x_min = doc["touch_x_min"] | 200;
  gSettings.touch_x_max = doc["touch_x_max"] | 3800;
  gSettings.touch_y_min = doc["touch_y_min"] | 200;
  gSettings.touch_y_max = doc["touch_y_max"] | 3800;
  gSettings.touch_offset_x = clampValue(doc["touch_offset_x"] | 0, -80, 80);
  gSettings.touch_offset_y = clampValue(doc["touch_offset_y"] | 0, -80, 80);
  gSettings.led_brightness = clampValue(doc["led_brightness"] | 40, 0, 100);
  gSettings.language = clampValue(doc["language"] | 0, 0, 4);
  gSettings.home_text_color_mode = clampValue(doc["home_text_color_mode"] | 0, 0, 3);
  gSettings.home_text_color_hex = doc["home_text_color_hex"] | "#FFFFFF";
  gSettings.home_show_info_badge = doc["home_show_info_badge"] | true;
  gSettings.home_boot_image_ms = clampValue(doc["home_boot_image_ms"] | 3000, 500, 10000);

  Serial.print("[SETTINGS] volume = ");
  Serial.println(gSettings.volume);

  Serial.print("[SETTINGS] brightness = ");
  Serial.println(gSettings.brightness);

  Serial.print("[SETTINGS] wifi_ssid = ");
  Serial.println(gSettings.wifi_ssid);

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
  Serial.print("[SETTINGS] home_text_color_mode = ");
  Serial.println(gSettings.home_text_color_mode);
  Serial.print("[SETTINGS] home_text_color_hex = ");
  Serial.println(gSettings.home_text_color_hex);
  Serial.print("[SETTINGS] home_show_info_badge = ");
  Serial.println(gSettings.home_show_info_badge ? "true" : "false");
  Serial.print("[SETTINGS] home_boot_image_ms = ");
  Serial.println(gSettings.home_boot_image_ms);

  return true;
}
