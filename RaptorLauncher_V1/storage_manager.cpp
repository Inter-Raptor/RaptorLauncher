#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "storage_manager.h"

#define SD_CS   5
#define SD_SCK  18
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

  if (!root.isDirectory()) {
    Serial.println("[SD] /games n'est pas un dossier");
    return list;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      GameInfo game;

      String folderName = String(file.name());

      // file.name() peut renvoyer "Dino" ou "/Dino" selon les cas
      if (!folderName.startsWith("/")) {
        folderName = "/" + folderName;
      }

      game.folder = folderName;
      game.name = folderName;
      game.author = "";
      game.description = "";

      String metaPath = "/games" + folderName + "/meta.json";

      Serial.print("[SD] dossier jeu: ");
      Serial.println(folderName);

      Serial.print("[SD] ouverture meta: ");
      Serial.println(metaPath);

      File meta = SD.open(metaPath);
      if (meta) {
        String jsonText;
        while (meta.available()) {
          jsonText += (char)meta.read();
        }
        meta.close();

        Serial.println("[SD] contenu meta.json:");
        Serial.println(jsonText);

        StaticJsonDocument<512> doc;
        DeserializationError err = deserializeJson(doc, jsonText);

        if (err) {
          Serial.print("[JSON] erreur: ");
          Serial.println(err.c_str());
        } else {
          game.name = doc["name"] | folderName;
          game.author = doc["author"] | "";
          game.description = doc["description"] | "";

          Serial.print("[JSON] name = ");
          Serial.println(game.name);

          Serial.print("[JSON] author = ");
          Serial.println(game.author);

          Serial.print("[JSON] description = ");
          Serial.println(game.description);
        }
      } else {
        Serial.println("[SD] meta.json absent ou impossible a ouvrir");
      }

      list.push_back(game);
    }

    file = root.openNextFile();
  }

  return list;
}