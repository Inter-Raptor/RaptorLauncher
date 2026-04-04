#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "settings_manager.h"

static const char* SETTINGS_PATH = "/settings.json";
static SystemSettings gSettings;

static int clampValue(int v, int minV, int maxV) {
  if (v < minV) return minV;
  if (v > maxV) return maxV;
  return v;
}

void settingsSetDefaults() {
  gSettings.volume = 80;
  gSettings.brightness = 100;
}

SystemSettings& settingsGet() {
  return gSettings;
}

bool settingsSave() {
  JsonDocument doc;
  doc["volume"] = gSettings.volume;
  doc["brightness"] = gSettings.brightness;

  if (SD.exists(SETTINGS_PATH)) {
    SD.remove(SETTINGS_PATH);
  }

  File f = SD.open(SETTINGS_PATH, FILE_WRITE);
  if (!f) {
    Serial.println("[SETTINGS] impossible d'ouvrir settings.json en ecriture");
    return false;
  }

  if (serializeJsonPretty(doc, f) == 0) {
    Serial.println("[SETTINGS] echec ecriture JSON");
    f.close();
    return false;
  }

  f.close();
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

  Serial.print("[SETTINGS] volume = ");
  Serial.println(gSettings.volume);
  Serial.print("[SETTINGS] brightness = ");
  Serial.println(gSettings.brightness);

  return true;
}