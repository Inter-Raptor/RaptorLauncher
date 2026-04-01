#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "storage_manager.h"

// Pins SD pour ta carte
#define SD_CS 5
#define SD_SCK 18
#define SD_MISO 19
#define SD_MOSI 23

static SPIClass sdSPI(VSPI);

bool storageInit() {
  Serial.println("[SD] init debut");

  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, sdSPI)) {
    Serial.println("[SD] ERREUR init");
    return false;
  }

  Serial.println("[SD] OK");
  return true;
}

std::vector<String> storageListGames() {
  std::vector<String> list;

  File root = SD.open("/games");
  if (!root) {
    Serial.println("[SD] dossier /games introuvable");
    return list;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      list.push_back(String(file.name()));
      Serial.print("[SD] jeu trouve: ");
      Serial.println(file.name());
    }
    file = root.openNextFile();
  }

  return list;
}