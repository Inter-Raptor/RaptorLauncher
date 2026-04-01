#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "storage_manager.h"

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

std::vector<GameInfo> storageListGames() {
  std::vector<GameInfo> list;

  File root = SD.open("/games");
  if (!root) {
    Serial.println("[SD] dossier /games introuvable");
    return list;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {

      String folder = String(file.name());
      String metaPath = "/games" + folder + "/meta.json";

      Serial.print("[SD] scan: ");
      Serial.println(metaPath);

      GameInfo game;
      game.folder = folder;
      game.name = folder;
      game.author = "";
      game.description = "";

      File meta = SD.open(metaPath);
      if (meta) {
        StaticJsonDocument<512> doc;
        DeserializationError err = deserializeJson(doc, meta);

        if (!err) {
          game.name = doc["name"] | folder;
          game.author = doc["author"] | "";
          game.description = doc["description"] | "";
        } else {
          Serial.println("[JSON] erreur lecture");
        }

        meta.close();
      } else {
        Serial.println("[SD] meta.json absent");
      }

      list.push_back(game);
    }

    file = root.openNextFile();
  }

  return list;
}