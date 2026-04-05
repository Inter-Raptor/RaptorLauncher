#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "gb_launch_config.h"

static const char* GB_PENDING_PATH = "/.gb_launch.json";
static const char* GB_PENDING_TMP_PATH = "/.gb_launch.tmp";

bool gbLaunchSavePendingRom(const String& romPath, const String& gameName) {
  JsonDocument doc;
  doc["rom"] = romPath;
  doc["game"] = gameName;
  doc["updated_at_ms"] = millis();

  if (SD.exists(GB_PENDING_TMP_PATH)) {
    SD.remove(GB_PENDING_TMP_PATH);
  }

  File f = SD.open(GB_PENDING_TMP_PATH, FILE_WRITE);
  if (!f) {
    Serial.println("[GB] impossible d'ouvrir config temporaire");
    return false;
  }

  if (serializeJson(doc, f) == 0) {
    Serial.println("[GB] echec ecriture config");
    f.close();
    return false;
  }
  f.close();

  if (SD.exists(GB_PENDING_PATH) && !SD.remove(GB_PENDING_PATH)) {
    Serial.println("[GB] impossible de supprimer ancienne config");
    return false;
  }

  if (!SD.rename(GB_PENDING_TMP_PATH, GB_PENDING_PATH)) {
    Serial.println("[GB] impossible de renommer config temp");
    return false;
  }

  return true;
}

bool gbLaunchLoadPendingRom(String& romPath, String& gameName) {
  if (!SD.exists(GB_PENDING_PATH)) return false;

  File f = SD.open(GB_PENDING_PATH, FILE_READ);
  if (!f) return false;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  romPath = doc["rom"] | "";
  gameName = doc["game"] | "";
  return romPath.length() > 0;
}
