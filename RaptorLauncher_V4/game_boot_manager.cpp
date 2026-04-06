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

  File f = SD.open(binPath, FILE_READ);
  if (!f) {
    Serial.printf("[BOOT] impossible d'ouvrir %s\n", binPath.c_str());
    return false;
  }

  size_t fileSize = f.size();
  uint8_t magic = 0x00;
  if (fileSize == 0 || f.read(&magic, 1) != 1) {
    Serial.printf("[BOOT] binaire vide/invalide: %s\n", binPath.c_str());
    f.close();
    return false;
  }
  if (!f.seek(0)) {
    Serial.printf("[BOOT] impossible de relire le binaire: %s\n", binPath.c_str());
    f.close();
    return false;
  }
  Serial.printf("[BOOT] taille binaire: %u octets, magic=0x%02X\n", (unsigned)fileSize, (unsigned)magic);
  if (magic != 0xE9) {
    Serial.println("[BOOT] format invalide: utiliser un .ino.bin / game.bin (pas merged.bin/bootloader.bin/partitions.bin)");
    f.close();
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
    if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
      Serial.println("[BOOT] validation image KO: binaire corrompu ou mauvais type de .bin");
    }
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
