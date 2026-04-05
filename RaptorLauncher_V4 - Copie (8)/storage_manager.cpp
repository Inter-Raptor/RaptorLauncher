#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "storage_manager.h"

#define SD_CS   5
#define SD_SCK  18
#define SD_MISO 19
#define SD_MOSI 23

static SPIClass sdSPI(HSPI);

static bool hasGbExtension(const String& path) {
  return path.endsWith(".gb") || path.endsWith(".gbc");
}

static void readJsonMeta(GameInfo& game, const String& metaPath) {
  File meta = SD.open(metaPath, FILE_READ);
  if (!meta) {
    Serial.println("[SD] meta/game json absent ou impossible a ouvrir");
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, meta);
  meta.close();

  if (err) {
    Serial.print("[JSON] erreur: ");
    Serial.println(err.c_str());
    return;
  }

  game.name        = doc["name"]        | doc["title"] | game.name;
  game.author      = doc["author"]      | "";
  game.description = doc["description"] | "";
  game.type        = doc["type"]        | "mixed";

  game.icon        = doc["icon"]        | "";
  game.iconW       = doc["icon_w"]      | game.iconW;
  game.iconH       = doc["icon_h"]      | game.iconH;

  game.title       = doc["title"]       | "";
  game.titleW      = doc["title_w"]     | game.titleW;
  game.titleH      = doc["title_h"]     | game.titleH;

  game.bin         = doc["bin"]         | game.bin;
  game.save        = doc["save"]        | game.save;
  game.rom         = doc["rom"]         | "";
  game.runner      = doc["runner"]      | "gb_runner.bin";
}

static void addGamesFromRoot(const char* basePath, const char* defaultMetaName, std::vector<GameInfo>& outList) {
  File root = SD.open(basePath);
  if (!root) {
    Serial.print("[SD] dossier introuvable: ");
    Serial.println(basePath);
    return;
  }

  if (!root.isDirectory()) {
    Serial.print("[SD] chemin non dossier: ");
    Serial.println(basePath);
    root.close();
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      GameInfo game;

      String folderName = String(file.name());

      if (folderName.startsWith(String(basePath) + "/")) {
        folderName.remove(0, String(basePath).length());
      }
      if (!folderName.startsWith("/")) {
        folderName = "/" + folderName;
      }

      game.folder = folderName;
      game.rootPath = basePath;

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
      game.rom = "";
      game.runner = "gb_runner.bin";

      String metaPath = String(basePath) + folderName + "/" + defaultMetaName;
      readJsonMeta(game, metaPath);

      if (game.type == "emulationGB") {
        if (!hasGbExtension(game.rom)) {
          Serial.print("[JSON] ROM ignoree (extension invalide): ");
          Serial.println(game.rom);
          file = root.openNextFile();
          continue;
        }
        if (game.bin.length() == 0 || game.bin == "game.bin") {
          game.bin = game.runner;
        }
      }

      outList.push_back(game);
    }

    file = root.openNextFile();
  }

  root.close();
}

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

  addGamesFromRoot("/games", "meta.json", list);
  addGamesFromRoot("/emulationGB", "game.json", list);

  return list;
}
