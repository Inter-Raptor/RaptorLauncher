#include "raptor_game_sdk.h"
#include <cstring>

#define ENABLE_SOUND 0
#include "peanut_gb.h"

RaptorGameSDK sdk;

struct GbBootConfig {
  String romPath;
  String savePath;
};

static const char* GB_GAME_FOLDER = "PokemonBleu_GB";

static struct gb_s gGb;
static GbBootConfig gCfg;

static uint8_t* gRomData = nullptr;
static size_t gRomSize = 0;

static uint8_t* gCartRam = nullptr;
static size_t gCartRamSize = 0;

static uint16_t gFrame565[LCD_WIDTH * LCD_HEIGHT];
static bool gEmuReady = false;
static String gStatus = "init";

static uint8_t gbRomRead(struct gb_s* /*gb*/, const uint_fast32_t addr) {
  return (addr < gRomSize) ? gRomData[addr] : 0xFF;
}

static uint8_t gbCartRamRead(struct gb_s* /*gb*/, const uint_fast32_t addr) {
  if (addr >= gCartRamSize || gCartRam == nullptr) return 0xFF;
  return gCartRam[addr];
}

static void gbCartRamWrite(struct gb_s* /*gb*/, const uint_fast32_t addr, const uint8_t val) {
  if (addr >= gCartRamSize || gCartRam == nullptr) return;
  gCartRam[addr] = val;
}

static void gbError(struct gb_s* /*gb*/, const enum gb_error_e e, const uint16_t addr) {
  gStatus = String("GB error ") + String((int)e) + String(" @") + String(addr, HEX);
  for (;;) delay(1000);
}

static void gbLcdDrawLine(struct gb_s* /*gb*/, const uint8_t* pixels, const uint_fast8_t line) {
  static const uint16_t pal[4] = {
    0xFFFF, // blanc
    0xAD55, // gris clair
    0x52AA, // gris fonce
    0x0000  // noir
  };

  uint16_t* dst = &gFrame565[(size_t)line * LCD_WIDTH];
  for (int x = 0; x < LCD_WIDTH; ++x) {
    dst[x] = pal[pixels[x] & 0x03];
  }
}

static bool sdReadAll(const String& path, uint8_t** outData, size_t* outSize) {
  File f = SD.open(path, FILE_READ);
  if (!f) return false;

  size_t size = f.size();
  if (size == 0) {
    f.close();
    return false;
  }

  uint8_t* data = (uint8_t*)malloc(size);
  if (!data) {
    f.close();
    return false;
  }

  size_t n = f.read(data, size);
  f.close();
  if (n != size) {
    free(data);
    return false;
  }

  *outData = data;
  *outSize = size;
  return true;
}

static bool loadBootConfig(GbBootConfig& out) {
  String path = String("/games/") + GB_GAME_FOLDER + "/boot.json";
  File f = SD.open(path, FILE_READ);
  if (!f) return false;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  String defaultRom = String("/games/") + GB_GAME_FOLDER + "/roms/PokemonBleu.gb";
  String defaultSave = String("/games/") + GB_GAME_FOLDER + "/sauv.json";
  out.romPath = doc["rom_path"] | defaultRom;
  out.savePath = doc["save_path"] | defaultSave;
  return true;
}

static void loadSaveFromSd() {
  if (!gCartRam || gCartRamSize == 0) return;
  File f = SD.open(gCfg.savePath, FILE_READ);
  if (!f) return;

  size_t n = f.read(gCartRam, gCartRamSize);
  f.close();
  gStatus = String("Save load: ") + String((unsigned)n) + " bytes";
}

static void flushSaveToSd() {
  if (!gCartRam || gCartRamSize == 0) return;

  SD.remove(gCfg.savePath);
  File f = SD.open(gCfg.savePath, FILE_WRITE);
  if (!f) return;

  f.write(gCartRam, gCartRamSize);
  f.close();
}

static uint8_t joypadMaskFromInputs() {
  uint8_t m = 0;
  if (sdk.isHeld(BTN_A)) m |= JOYPAD_A;
  if (sdk.isHeld(BTN_B)) m |= JOYPAD_B;
  if (sdk.isHeld(BTN_SELECT)) m |= JOYPAD_SELECT;
  if (sdk.isHeld(BTN_START)) m |= JOYPAD_START;
  if (sdk.isHeld(BTN_RIGHT)) m |= JOYPAD_RIGHT;
  if (sdk.isHeld(BTN_LEFT)) m |= JOYPAD_LEFT;
  if (sdk.isHeld(BTN_UP)) m |= JOYPAD_UP;
  if (sdk.isHeld(BTN_DOWN)) m |= JOYPAD_DOWN;
  return m;
}

static void drawErrorScreen(const char* title, const String& detail) {
  sdk.clear();
  sdk.drawCenteredText(10, title, SDK_COLOR_WARN);
  sdk.drawSmallText(8, 40, detail.c_str(), SDK_COLOR_TEXT);
  sdk.drawSmallText(8, 220, "START = retour launcher", SDK_COLOR_TEXT);
}

static bool initEmulator() {
  if (!sdk.isSdReady()) {
    drawErrorScreen("SD non prete", "Verifie la carte SD.");
    return false;
  }

  if (!loadBootConfig(gCfg)) {
    drawErrorScreen("boot.json KO", "Fichier /games/PokemonBleu_GB/boot.json introuvable/invalide");
    return false;
  }

  if (!sdReadAll(gCfg.romPath, &gRomData, &gRomSize)) {
    drawErrorScreen("ROM KO", String("Introuvable: ") + gCfg.romPath);
    return false;
  }

  enum gb_init_error_e err = gb_init(&gGb, gbRomRead, gbCartRamRead, gbCartRamWrite, gbError, nullptr);
  if (err != GB_INIT_NO_ERROR) {
    drawErrorScreen("gb_init KO", String("Erreur code: ") + String((int)err));
    return false;
  }

  gb_init_lcd(&gGb, gbLcdDrawLine);

  if (gb_get_save_size_s(&gGb, &gCartRamSize) == 0 && gCartRamSize > 0) {
    gCartRam = (uint8_t*)malloc(gCartRamSize);
    if (!gCartRam) {
      drawErrorScreen("RAM KO", "Allocation cart RAM impossible");
      return false;
    }
    memset(gCartRam, 0, gCartRamSize);
    loadSaveFromSd();
  }

  gStatus = "GB init OK";
  return true;
}

void setup() {
  sdk.begin();
  gEmuReady = initEmulator();
}

void loop() {
  sdk.updateInputs();

  if (!gEmuReady) {
    if (sdk.isPressed(BTN_START)) sdk.requestReturnToLauncher();
    delay(16);
    return;
  }

  // START+SELECT = retour launcher avec sauvegarde
  if (sdk.isHeld(BTN_START) && sdk.isHeld(BTN_SELECT)) {
    flushSaveToSd();
    sdk.requestReturnToLauncher();
  }

  gGb.direct.joypad = joypadMaskFromInputs();
  gb_run_frame(&gGb);

  // Dessin ecran GB 160x144 centre sur ecran 320x240
  sdk.clear();
  int x = (sdk.width() - LCD_WIDTH) / 2;
  int y = (sdk.height() - LCD_HEIGHT) / 2;
  if (x < 0) x = 0;
  if (y < 0) y = 0;

  sdk.drawPixels565(x, y, LCD_WIDTH, LCD_HEIGHT, gFrame565);

  if (sdk.isPressed(BTN_A) && sdk.isPressed(BTN_B)) {
    flushSaveToSd();
    sdk.playBeep(1800, 40);
  }

  delay(1);
}
