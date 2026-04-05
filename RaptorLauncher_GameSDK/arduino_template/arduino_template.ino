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
  sdk.playBeep(1200, 90);

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
    sdk.playBeep(2000, 50);
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

  sdk.drawSmallText(10, sdk.height() - 14, "START = retour launcher");

  if (sdk.isPressed(BTN_START)) {
    saveGame();
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
