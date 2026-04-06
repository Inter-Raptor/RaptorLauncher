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
static const char* GB_GAME_FOLDER_FALLBACK = "pokemon_gb";

static struct gb_s gGb;
static GbBootConfig gCfg;
static String gResolvedGameFolder = GB_GAME_FOLDER;

static File gRomFile;
static size_t gRomSize = 0;
static uint8_t* gRomData = nullptr;
static bool gRomLoadedInMemory = false;

static constexpr int ROM_BANK_CACHE_SLOTS = 8;
struct RomBankCacheSlot {
  uint32_t bankIndex;
  uint32_t age;
  size_t len;
  bool valid;
  uint8_t data[0x4000];
};
static RomBankCacheSlot gRomBankCache[ROM_BANK_CACHE_SLOTS];
static uint32_t gRomBankAgeCounter = 1;

static uint8_t* gCartRam = nullptr;
static size_t gCartRamSize = 0;

static uint16_t gFrame565[LCD_WIDTH * LCD_HEIGHT];
static bool gEmuReady = false;
static String gStatus = "init";
static bool gFrameHasContent = false;
static uint32_t gUiTick = 0;

static bool readRomBankToBuffer(uint32_t bankIndex, uint8_t* buffer, size_t& outLen) {
  if (!gRomFile) return false;
  uint32_t offset = bankIndex * 0x4000u;
  if (offset >= gRomSize) {
    outLen = 0;
    return false;
  }
  if (!gRomFile.seek(offset)) {
    outLen = 0;
    return false;
  }
  outLen = gRomFile.read(buffer, 0x4000);
  return outLen > 0;
}

static RomBankCacheSlot* getRomBankSlot(uint32_t bankIndex) {
  RomBankCacheSlot* oldest = &gRomBankCache[0];
  for (int i = 0; i < ROM_BANK_CACHE_SLOTS; ++i) {
    RomBankCacheSlot* slot = &gRomBankCache[i];
    if (slot->valid && slot->bankIndex == bankIndex) {
      slot->age = gRomBankAgeCounter++;
      return slot;
    }
    if (!slot->valid) oldest = slot;
    else if (slot->age < oldest->age) oldest = slot;
  }

  size_t n = 0;
  if (!readRomBankToBuffer(bankIndex, oldest->data, n)) return nullptr;
  oldest->bankIndex = bankIndex;
  oldest->len = n;
  oldest->valid = true;
  oldest->age = gRomBankAgeCounter++;
  return oldest;
}

static uint8_t gbRomRead(struct gb_s* /*gb*/, const uint_fast32_t addr) {
  if (addr >= gRomSize) return 0xFF;

  if (gRomLoadedInMemory && gRomData) {
    return gRomData[addr];
  }

  const uint32_t bankIndex = (uint32_t)(addr >> 14); // /0x4000
  const uint16_t bankOffset = (uint16_t)(addr & 0x3FFFu);

  RomBankCacheSlot* slot = getRomBankSlot(bankIndex);
  if (!slot || bankOffset >= slot->len) return 0xFF;
  return slot->data[bankOffset];
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
    uint8_t shade = pixels[x] & 0x03;
    if (shade != 0) gFrameHasContent = true;
    dst[x] = pal[shade];
  }
}

static bool loadBootConfig(GbBootConfig& out) {
  String primaryPath = String("/games/") + GB_GAME_FOLDER + "/boot.json";
  String fallbackPath = String("/games/") + GB_GAME_FOLDER_FALLBACK + "/boot.json";

  File f = SD.open(primaryPath, FILE_READ);
  if (!f) {
    f = SD.open(fallbackPath, FILE_READ);
    if (!f) return false;
    gResolvedGameFolder = GB_GAME_FOLDER_FALLBACK;
  } else {
    gResolvedGameFolder = GB_GAME_FOLDER;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  String defaultRom = String("/games/") + gResolvedGameFolder + "/roms/PokemonBleu.gb";
  String defaultSave = String("/games/") + gResolvedGameFolder + "/sauv.json";
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

  gRomFile = SD.open(gCfg.romPath, FILE_READ);
  if (!gRomFile) {
    drawErrorScreen("ROM KO", String("Introuvable: ") + gCfg.romPath);
    return false;
  }
  gRomSize = gRomFile.size();
  if (gRomSize == 0) {
    drawErrorScreen("ROM KO", String("Fichier vide: ") + gCfg.romPath);
    gRomFile.close();
    return false;
  }

  gRomData = (uint8_t*)malloc(gRomSize);
  if (gRomData) {
    size_t n = gRomFile.read(gRomData, gRomSize);
    if (n == gRomSize) {
      gRomLoadedInMemory = true;
      gRomFile.close();
      gStatus = String("ROM en RAM: ") + String((unsigned)gRomSize) + " bytes";
    } else {
      free(gRomData);
      gRomData = nullptr;
      gRomLoadedInMemory = false;
      gRomFile.seek(0);
      gStatus = String("ROM stream SD: ") + String((unsigned)n) + "/" + String((unsigned)gRomSize);
    }
  } else {
    gRomLoadedInMemory = false;
    gStatus = "ROM stream SD: cache 8 banques";
  }

  enum gb_init_error_e err = gb_init(&gGb, gbRomRead, gbCartRamRead, gbCartRamWrite, gbError, nullptr);
  if (err != GB_INIT_NO_ERROR) {
    drawErrorScreen("gb_init KO", String("Erreur code: ") + String((int)err));
    return false;
  }

  gb_init_lcd(&gGb, gbLcdDrawLine);
  gGb.direct.frame_skip = true;

  if (gb_get_save_size_s(&gGb, &gCartRamSize) == 0 && gCartRamSize > 0) {
    gCartRam = (uint8_t*)malloc(gCartRamSize);
    if (!gCartRam) {
      drawErrorScreen("RAM KO", "Allocation cart RAM impossible");
      return false;
    }
    memset(gCartRam, 0, gCartRamSize);
    loadSaveFromSd();
  }

  if (gStatus == "init") gStatus = "GB init OK";
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

  int x = (sdk.width() - LCD_WIDTH) / 2;
  int y = (sdk.height() - LCD_HEIGHT) / 2;
  if (x < 0) x = 0;
  if (y < 0) y = 0;

  gFrameHasContent = false;
  gb_run_frame(&gGb);

  sdk.drawPixels565(x, y, LCD_WIDTH, LCD_HEIGHT, gFrame565);
  if ((++gUiTick & 31u) == 0u) {
    sdk.drawSmallText(4, 4, gStatus.c_str(), SDK_COLOR_TEXT, SDK_COLOR_BG);
  }
  if (!gFrameHasContent) {
    sdk.drawSmallText(6, 6, "Frame GB vide -> verifier ROM/boot.json", SDK_COLOR_WARN, SDK_COLOR_BG);
  }

  if (sdk.isPressed(BTN_A) && sdk.isPressed(BTN_B)) {
    flushSaveToSd();
    sdk.playBeep(1800, 40);
  }

  delay(1);
}
