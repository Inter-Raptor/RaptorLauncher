#include <Arduino.h>
#include <SD.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <Preferences.h>
#include "game_boot_manager.h"

static Preferences gPrefs;
static const char* PREF_NS = "raptor_boot";
static const char* PREF_LAUNCHER_LABEL = "launcher";

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

  const int maxAttempts = 2;
  esp_err_t err = ESP_FAIL;
  for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
    File f = SD.open(binPath, FILE_READ);
    if (!f) {
      Serial.printf("[BOOT] impossible d'ouvrir %s\n", binPath.c_str());
      return false;
    }

    size_t expectedSize = f.size();
    size_t totalWritten = 0;
    Serial.printf("[BOOT] tentative %d/%d, taille=%u octets\n", attempt, maxAttempts, (unsigned)expectedSize);

    esp_ota_handle_t otaHandle = 0;
    err = esp_ota_begin(target, OTA_SIZE_UNKNOWN, &otaHandle);
    if (err != ESP_OK) {
      Serial.printf("[BOOT] esp_ota_begin KO: %d\n", (int)err);
      f.close();
      return false;
    }

    uint8_t buf[1024];
    while (totalWritten < expectedSize) {
      size_t toRead = expectedSize - totalWritten;
      if (toRead > sizeof(buf)) toRead = sizeof(buf);
      size_t n = f.read(buf, toRead);
      if (n == 0) {
        Serial.printf("[BOOT] lecture SD interrompue a %u/%u octets\n", (unsigned)totalWritten, (unsigned)expectedSize);
        esp_ota_abort(otaHandle);
        f.close();
        err = ESP_FAIL;
        break;
      }
      err = esp_ota_write(otaHandle, buf, n);
      if (err != ESP_OK) {
        Serial.printf("[BOOT] esp_ota_write KO: %d\n", (int)err);
        esp_ota_abort(otaHandle);
        f.close();
        break;
      }
      totalWritten += n;
    }
    f.close();

    if (err != ESP_OK) {
      continue;
    }

    if (totalWritten != expectedSize) {
      Serial.printf("[BOOT] taille ecrite invalide: %u/%u\n", (unsigned)totalWritten, (unsigned)expectedSize);
      esp_ota_abort(otaHandle);
      err = ESP_FAIL;
      continue;
    }

    err = esp_ota_end(otaHandle);
    if (err == ESP_OK) {
      break;
    }

    Serial.printf("[BOOT] esp_ota_end KO (tentative %d): %d\n", attempt, (int)err);
    delay(50);
  }

  if (err != ESP_OK) {
    Serial.printf("[BOOT] validation image KO apres %d tentative(s)\n", maxAttempts);
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
