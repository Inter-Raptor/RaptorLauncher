#include "../../arduino_template/src/raptor_game_sdk.h"

RaptorGameSDK sdk;

unsigned long lastBlink = 0;
bool ledOn = false;

void setup() {
  sdk.begin();
  sdk.clear();
  sdk.drawCenteredText(10, "SDK TestLab");
  sdk.drawSmallText(8, 34, "Validation API officielle");
  sdk.drawSmallText(8, 48, "START: quitter vers launcher");
}

void loop() {
  sdk.updateInputs();

  if (millis() - lastBlink > 500) {
    ledOn = !ledOn;
    lastBlink = millis();
    if (ledOn) sdk.setLedRgb(0, 40, 0);
    else sdk.ledOff();
  }

  if (sdk.isPressed(BTN_A)) {
    sdk.playBeep(1700, 40);
  }

  // Capacites optionnelles (ne cassent pas la compilation)
  if (sdk.hasBmpSupport()) {
    (void)sdk.drawBmp(sdk.assetPath("splash.bmp"), 0, 0);
  }
  if (sdk.hasPngSupport()) {
    (void)sdk.drawPng(sdk.assetPath("overlay.png"), 0, 0);
  }
  if (sdk.hasWavSupport()) {
    (void)sdk.playWav(sdk.assetPath("sfx/start.wav"));
  }

  sdk.fillRect(0, 70, sdk.width(), 80, SDK_COLOR_BG);

  char line[96];
  snprintf(line, sizeof(line), "Touch:%s (%d,%d)", sdk.isTouchHeld() ? "ON" : "OFF", sdk.touchX(), sdk.touchY());
  sdk.drawSmallText(8, 74, line);

  snprintf(line, sizeof(line), "SD:%s WiFi:%s", sdk.isSdReady() ? "OK" : "KO", sdk.wifiIsConnected() ? "ON" : "OFF");
  sdk.drawSmallText(8, 88, line);

  snprintf(line, sizeof(line), "BMP:%s PNG:%s WAV:%s MP3:%s",
           sdk.hasBmpSupport() ? "Y" : "N",
           sdk.hasPngSupport() ? "Y" : "N",
           sdk.hasWavSupport() ? "Y" : "N",
           sdk.hasMp3Support() ? "Y" : "N");
  sdk.drawSmallText(8, 102, line);

  String health = sdk.sdkHealthReport();
  sdk.drawSmallText(8, 116, health.c_str());

  if (sdk.isPressed(BTN_START)) {
    sdk.requestReturnToLauncher();
  }

  delay(16);
}
