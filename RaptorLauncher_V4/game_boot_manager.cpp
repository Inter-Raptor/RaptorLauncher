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

  const uint32_t candidateOffsets[] = {0, 0x10000, 0x1000};
  const int maxAttemptsPerOffset = 2;
  esp_err_t err = ESP_FAIL;

  for (size_t oi = 0; oi < (sizeof(candidateOffsets) / sizeof(candidateOffsets[0])); ++oi) {
    uint32_t offset = candidateOffsets[oi];
    for (int attempt = 1; attempt <= maxAttemptsPerOffset; ++attempt) {
    File f = SD.open(binPath, FILE_READ);
    if (!f) {
      Serial.printf("[BOOT] impossible d'ouvrir %s\n", binPath.c_str());
      return false;
    }

      size_t fileSize = f.size();
      if (offset >= fileSize) {
        f.close();
        continue;
      }

      if (!f.seek(offset)) {
        Serial.printf("[BOOT] seek KO sur offset 0x%lX\n", (unsigned long)offset);
        f.close();
        continue;
      }

      int magic = f.read();
      if (magic != 0xE9) {
        f.close();
        continue;
      }

      if (!f.seek(offset)) {
        f.close();
        continue;
      }

    size_t expectedSize = fileSize - offset;
    size_t totalWritten = 0;
      if (expectedSize > (size_t)target->size) {
        Serial.printf("[BOOT] image trop grande pour %s: %u > %u (offset 0x%lX)\n",
                      target->label,
                      (unsigned)expectedSize,
                      (unsigned)target->size,
                      (unsigned long)offset);
        f.close();
        continue;
      }
      Serial.printf("[BOOT] offset=0x%lX tentative %d/%d, taille=%u octets\n",
                    (unsigned long)offset, attempt, maxAttemptsPerOffset, (unsigned)expectedSize);

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
    if (err == ESP_OK) break;
  }

  if (err != ESP_OK) {
    Serial.printf("[BOOT] validation image KO (offsets testes: 0x0,0x10000,0x1000)\n");
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
