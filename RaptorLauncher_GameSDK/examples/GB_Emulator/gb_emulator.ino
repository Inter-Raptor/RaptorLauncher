#include "../../arduino_template/src/raptor_game_sdk.h"

RaptorGameSDK sdk;

struct GbBootConfig {
  String romPath;
  String savePath;
  bool audioEnabled;
  int scale;
};

static const char* GB_GAME_FOLDER = "PokemonBleu_GB";
static GbBootConfig gCfg;
static bool gCfgLoaded = false;
static bool gRomFound = false;

bool loadGbBootConfig(GbBootConfig& out) {
  if (!sdk.isSdReady()) return false;

  String bootPath = String("/games/") + GB_GAME_FOLDER + "/boot.json";
  File f = SD.open(bootPath, FILE_READ);
  if (!f) return false;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  out.romPath = doc["rom_path"] | String("/games/") + GB_GAME_FOLDER + "/roms/PokemonBleu.gb";
  out.savePath = doc["save_path"] | String("/games/") + GB_GAME_FOLDER + "/sauv.json";
  out.audioEnabled = doc["audio_enabled"] | true;
  out.scale = doc["scale"] | 2;

  if (out.scale < 1) out.scale = 1;
  if (out.scale > 4) out.scale = 4;
  return true;
}

bool sdFileExists(const String& path) {
  if (!sdk.isSdReady()) return false;
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  f.close();
  return true;
}

void drawStatusScreen() {
  sdk.clear();
  sdk.drawCenteredText(10, "GB Emulator (Template)");

  sdk.drawSmallText(8, 38, gCfgLoaded ? "boot.json: OK" : "boot.json: KO", gCfgLoaded ? SDK_COLOR_OK : SDK_COLOR_WARN);
  sdk.drawSmallText(8, 52, gRomFound ? "ROM .gb: OK" : "ROM .gb: introuvable", gRomFound ? SDK_COLOR_OK : SDK_COLOR_WARN);

  String line = String("ROM: ") + (gCfgLoaded ? gCfg.romPath : "(n/a)");
  sdk.drawSmallText(8, 70, line.c_str());

  line = String("SAVE: ") + (gCfgLoaded ? gCfg.savePath : "(n/a)");
  sdk.drawSmallText(8, 84, line.c_str());

  line = String("Scale:") + (gCfgLoaded ? String(gCfg.scale) : String("n/a")) +
         String(" Audio:") + (gCfgLoaded && gCfg.audioEnabled ? "ON" : "OFF");
  sdk.drawSmallText(8, 98, line.c_str());

  sdk.drawSmallText(8, 120, "A: beep test  START: retour launcher");

  if (!gRomFound) {
    sdk.drawSmallText(8, 140, "Copie PokemonBleu.gb dans /games/PokemonBleu_GB/roms", SDK_COLOR_WARN);
  } else {
    sdk.drawSmallText(8, 140, "Template pret: remplace ce code par ton coeur emulateur", SDK_COLOR_OK);
  }
}

void setup() {
  sdk.begin();

  gCfgLoaded = loadGbBootConfig(gCfg);
  gRomFound = gCfgLoaded && sdFileExists(gCfg.romPath);

  drawStatusScreen();
}

void loop() {
  sdk.updateInputs();

  if (sdk.isPressed(BTN_A)) {
    sdk.playBeep(1400, 80);
  }

  if (sdk.isPressed(BTN_START)) {
    sdk.requestReturnToLauncher();
  }

  delay(16);
}
