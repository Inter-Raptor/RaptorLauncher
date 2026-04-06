#include <Arduino.h>
#include <SD.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <Preferences.h>
#include "game_boot_manager.h"

static Preferences gPrefs;
static const char* PREF_NS = "raptor_boot";
static const char* PREF_LAUNCHER_LABEL = "launcher";
static const uint8_t ESP_IMAGE_MAGIC = 0xE9;
static const uint8_t ESP_IMAGE_CHECKSUM_INIT = 0xEF;

static uint32_t readU32LE(const uint8_t* p) {
  return ((uint32_t)p[0]) |
         ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

static bool inspectEspImageAtOffset(const String& path, uint32_t offset, size_t& imageSizeOut) {
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  if (!f.seek(offset)) {
    f.close();
    return false;
  }

  uint8_t hdr[24];
  if (f.read(hdr, sizeof(hdr)) != sizeof(hdr)) {
    f.close();
    return false;
  }
  if (hdr[0] != ESP_IMAGE_MAGIC) {
    f.close();
    return false;
  }

  uint8_t segmentCount = hdr[1];
  size_t pos = offset + sizeof(hdr);
  uint8_t checksum = ESP_IMAGE_CHECKSUM_INIT;
  uint8_t buf[256];

  for (uint8_t i = 0; i < segmentCount; ++i) {
    if (!f.seek(pos)) {
      f.close();
      return false;
    }
    uint8_t segHdr[8];
    if (f.read(segHdr, sizeof(segHdr)) != sizeof(segHdr)) {
      f.close();
      return false;
    }
    uint32_t segLen = readU32LE(segHdr + 4);
    pos += sizeof(segHdr);

    uint32_t remaining = segLen;
    while (remaining > 0) {
      size_t chunk = remaining > sizeof(buf) ? sizeof(buf) : remaining;
      size_t n = f.read(buf, chunk);
      if (n != chunk) {
        f.close();
        return false;
      }
      for (size_t k = 0; k < n; ++k) checksum ^= buf[k];
      remaining -= (uint32_t)n;
      pos += n;
    }
  }

  size_t checksumPos = (pos + 15) & ~((size_t)15);
  if (!f.seek(checksumPos)) {
    f.close();
    return false;
  }
  int expected = f.read();
  f.close();
  if (expected < 0) return false;
  if ((uint8_t)expected != checksum) return false;

  imageSizeOut = (checksumPos - offset) + 1; // inclus le byte checksum
  return true;
}

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

      size_t imageSize = 0;
      if (!inspectEspImageAtOffset(binPath, offset, imageSize)) {
        Serial.printf("[BOOT] image invalide a l'offset 0x%lX\n", (unsigned long)offset);
        f.close();
        continue;
      }
      if (!f.seek(offset)) {
        f.close();
        continue;
      }

    size_t expectedSize = imageSize;
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
