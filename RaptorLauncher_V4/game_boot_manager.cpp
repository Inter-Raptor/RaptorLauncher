#include <Arduino.h>
#include <SD.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <Preferences.h>
#include "game_boot_manager.h"

static Preferences gPrefs;
static const char* PREF_NS = "raptor_boot";
static const char* PREF_LAUNCHER_LABEL = "launcher";
static const char* PREF_PENDING_BIN = "pending_bin";
static const char* PREF_FAIL_COUNT = "fail_count";
static const char* PREF_BLOCKED_BIN = "blocked_bin";

static bool selectTargetOta(const esp_partition_t* running, const esp_partition_t*& target) {
  if (running->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_0) {
    target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, nullptr);
  } else {
    target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, nullptr);
  }
  return target != nullptr;
}

bool gameBootInit() {
  return gPrefs.begin(PREF_NS, false);
}

bool gameBootLaunchFromPath(const String& binPath) {
  const esp_partition_t* running = esp_ota_get_running_partition();
  if (!running) return false;

  // mémorise la partition launcher courante pour retour futur
  gPrefs.putString(PREF_LAUNCHER_LABEL, String(running->label));

  const esp_partition_t* target = nullptr;
  if (!selectTargetOta(running, target)) return false;

  gPrefs.putString(PREF_PENDING_BIN, binPath);

  File f = SD.open(binPath, FILE_READ);
  if (!f) {
    Serial.printf("[BOOT] impossible d'ouvrir %s\n", binPath.c_str());
    return false;
  }

  esp_ota_handle_t otaHandle = 0;
  esp_err_t err = esp_ota_begin(target, OTA_SIZE_UNKNOWN, &otaHandle);
  if (err != ESP_OK) {
    Serial.printf("[BOOT] esp_ota_begin KO: %d\n", (int)err);
    f.close();
    return false;
  }

  uint8_t buf[4096];
  while (f.available()) {
    size_t n = f.read(buf, sizeof(buf));
    if (n == 0) break;
    err = esp_ota_write(otaHandle, buf, n);
    if (err != ESP_OK) {
      Serial.printf("[BOOT] esp_ota_write KO: %d\n", (int)err);
      esp_ota_end(otaHandle);
      f.close();
      return false;
    }
  }
  f.close();

  err = esp_ota_end(otaHandle);
  if (err != ESP_OK) {
    Serial.printf("[BOOT] esp_ota_end KO: %d\n", (int)err);
    return false;
  }

  err = esp_ota_set_boot_partition(target);
  if (err != ESP_OK) {
    Serial.printf("[BOOT] set_boot_partition KO: %d\n", (int)err);
    return false;
  }

  Serial.printf("[BOOT] jeu programme dans %s, reboot...\n", target->label);
  delay(150);
  ESP.restart();
  return true;
}

bool gameBootMarkReturnToLauncher() {
  if (!gPrefs.begin(PREF_NS, false)) return false;
  String label = gPrefs.getString(PREF_LAUNCHER_LABEL, "");
  if (label.length() == 0) return false;

  const esp_partition_t* launcher = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, label.c_str());
  if (!launcher) return false;
  return esp_ota_set_boot_partition(launcher) == ESP_OK;
}

void gameBootHandleLauncherBoot() {
  String pending = gPrefs.getString(PREF_PENDING_BIN, "");
  if (pending.length() == 0) {
    return;
  }

  uint8_t failCount = (uint8_t)gPrefs.getUChar(PREF_FAIL_COUNT, 0);
  failCount++;
  gPrefs.putUChar(PREF_FAIL_COUNT, failCount);
  Serial.printf("[SAFE] retour launcher apres tentative: %s (echecs=%u)\n", pending.c_str(), (unsigned)failCount);

  if (failCount >= 2) {
    gPrefs.putString(PREF_BLOCKED_BIN, pending);
    Serial.printf("[SAFE] jeu bloque temporairement: %s\n", pending.c_str());
  }
}

void gameBootMarkLauncherStable() {
  if (gPrefs.getString(PREF_PENDING_BIN, "").length() == 0) return;
  gPrefs.putString(PREF_PENDING_BIN, "");
  gPrefs.putUChar(PREF_FAIL_COUNT, 0);
}

bool gameBootIsPathBlocked(const String& binPath, String& reason) {
  String blocked = gPrefs.getString(PREF_BLOCKED_BIN, "");
  if (blocked.length() == 0) return false;
  if (blocked != binPath) return false;
  reason = "SAFE_MODE_BLOCKED";
  return true;
}
