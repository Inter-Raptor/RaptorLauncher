#include "src/raptor_game_sdk.h"

RaptorGameSDK sdk;

// -----------------------------------------------------------------------------
// Ton jeu: variables
// -----------------------------------------------------------------------------
int playerX = 150;
int playerY = 110;
int score = 0;

void loadSave() {
  JsonDocument doc;
  if (sdk.loadJson(doc)) {
    score = doc["score"] | 0;
    playerX = doc["player_x"] | 150;
    playerY = doc["player_y"] | 110;
  }
}

void saveGame() {
  JsonDocument doc;
  doc["score"] = score;
  doc["player_x"] = playerX;
  doc["player_y"] = playerY;
  sdk.saveJson(doc); // sauvegarde dans /games/<SDK_GAME_FOLDER_NAME>/sauv.json
}

void gameInit() {
  sdk.clear();
  sdk.drawCenteredText(20, "Mon Jeu RaptorLauncher");
  sdk.drawSmallText(10, 50, "A: +1 point  B: reset");
  sdk.drawSmallText(10, 62, "SELECT: sauver  START: quitter");
  sdk.drawSmallText(10, 74, "Touch: deplace le carre");
  sdk.drawSmallText(10, 98, "AUDIO/BMP/PNG wrappers actifs");

  String err;
  if (!sdk.validateGameMeta("/games/MonJeu/meta.json", err)) {
    String msg = String("Meta KO: ") + err;
    sdk.drawSmallText(10, 86, msg.c_str());
  }
  sdk.setLedRgb(0, 40, 0);
  if (sdk.wifiConnectFromSettings()) {
    sdk.playBeep(1800, 60);
  }

  // Exemples wrappers avances (optionnels selon libs/format)
  (void)sdk.drawBmp(sdk.assetPath("splash.bmp"), 0, 0);
  if (sdk.hasPngSupport()) {
    (void)sdk.drawPng(sdk.assetPath("overlay.png"), 0, 0);
  }
  if (sdk.hasWavSupport()) {
    (void)sdk.playWav(sdk.assetPath("sfx/start.wav"));
  } else {
    sdk.playBeep(1200, 90);
  }

  loadSave();
}

void gameUpdate() {
  sdk.updateInputs();

  if (sdk.isPressed(BTN_LEFT))  playerX -= 3;
  if (sdk.isPressed(BTN_RIGHT)) playerX += 3;
  if (sdk.isPressed(BTN_UP))    playerY -= 3;
  if (sdk.isPressed(BTN_DOWN))  playerY += 3;

  if (sdk.isPressed(BTN_A)) {
    score++;
    if (sdk.hasMp3Support()) {
      (void)sdk.playMp3(sdk.assetPath("music/click.mp3"));
    } else {
      sdk.playBeep(2000, 50);
    }
  }

  if (sdk.isPressed(BTN_B)) {
    score = 0;
    sdk.playBeep(700, 120);
  }

  if (sdk.isTouchHeld()) {
    playerX = sdk.touchX() - 6;
    playerY = sdk.touchY() - 6;
  }

  if (sdk.isPressed(BTN_SELECT)) {
    saveGame();
    sdk.playBeep(1500, 70);
  }

  playerX = constrain(playerX, 0, sdk.width() - 12);
  playerY = constrain(playerY, 0, sdk.height() - 12);

  sdk.clear();
  sdk.drawRect(playerX, playerY, 12, 12, SDK_COLOR_ACCENT);

  char line[64];
  snprintf(line, sizeof(line), "Score: %d", score);
  sdk.drawSmallText(10, 10, line);

  snprintf(line, sizeof(line), "Save: %s", sdk.saveJsonPath().c_str());
  sdk.drawSmallText(10, 24, line);

  snprintf(line, sizeof(line), "Touch: %s (%d,%d)", sdk.isTouchHeld() ? "ON" : "OFF", sdk.touchX(), sdk.touchY());
  sdk.drawSmallText(10, 38, line);

  int ldr = sdk.readLightPercent();
  snprintf(line, sizeof(line), "LDR:%d%% SD:%s PNG:%s WAV:%s", ldr, sdk.isSdReady() ? "OK" : "KO", sdk.hasPngSupport()?"Y":"N", sdk.hasWavSupport()?"Y":"N");
  sdk.drawSmallText(10, 52, line);

  String health = sdk.sdkHealthReport();
  sdk.drawSmallText(10, sdk.height() - 26, health.c_str());
  sdk.drawSmallText(10, sdk.height() - 14, "START = retour launcher");

  if (sdk.isPressed(BTN_START)) {
    saveGame();
    sdk.wifiDisconnect();
    sdk.ledOff();
    sdk.requestReturnToLauncher();
  }

  delay(16); // ~60 FPS
}

void setup() {
  sdk.begin();
  gameInit();
}

void loop() {
  gameUpdate();
}
