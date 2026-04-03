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
    root.close();
    return list;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      GameInfo game;

      String folderName = String(file.name());

      if (folderName.startsWith("/games/")) {
        folderName.remove(0, 6);
      }
      if (!folderName.startsWith("/")) {
        folderName = "/" + folderName;
      }

      game.folder = folderName;

      game.name = folderName.substring(1);
      game.author = "";
      game.description = "";
      game.type = "mixed";

      game.icon = "";
      game.iconW = 0;
      game.iconH = 0;

      game.title = "";
      game.titleW = 0;
      game.titleH = 0;

      game.bin = "game.bin";
      game.save = "sauv.json";

      String metaPath = "/games" + folderName + "/meta.json";

      Serial.print("[SD] dossier jeu: ");
      Serial.println(folderName);

      Serial.print("[SD] ouverture meta: ");
      Serial.println(metaPath);

      File meta = SD.open(metaPath, FILE_READ);
      if (meta) {
        String jsonText;
        while (meta.available()) {
          jsonText += (char)meta.read();
        }
        meta.close();

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, jsonText);

        if (err) {
          Serial.print("[JSON] erreur: ");
          Serial.println(err.c_str());
        } else {
          game.name        = doc["name"]        | game.name;
          game.author      = doc["author"]      | "";
          game.description = doc["description"] | "";
          game.type        = doc["type"]        | "mixed";

          game.icon        = doc["icon"]        | "";
          game.iconW       = doc["icon_w"]      | 0;
          game.iconH       = doc["icon_h"]      | 0;

          game.title       = doc["title"]       | "";
          game.titleW      = doc["title_w"]     | 0;
          game.titleH      = doc["title_h"]     | 0;

          game.bin         = doc["bin"]         | "game.bin";
          game.save        = doc["save"]        | "sauv.json";

          Serial.print("[JSON] name = ");
          Serial.println(game.name);

          Serial.print("[JSON] icon = ");
          Serial.println(game.icon);

          Serial.print("[JSON] icon_w = ");
          Serial.println(game.iconW);

          Serial.print("[JSON] icon_h = ");
          Serial.println(game.iconH);

          Serial.print("[JSON] title = ");
          Serial.println(game.title);

          Serial.print("[JSON] title_w = ");
          Serial.println(game.titleW);

          Serial.print("[JSON] title_h = ");
          Serial.println(game.titleH);

          Serial.print("[JSON] bin = ");
          Serial.println(game.bin);

          Serial.print("[JSON] save = ");
          Serial.println(game.save);
        }
      } else {
        Serial.println("[SD] meta.json absent ou impossible a ouvrir");
      }

      list.push_back(game);
    }

    file = root.openNextFile();
  }

  root.close();
  return list;
}