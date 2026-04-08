#include "audio.h"
#include "game_state.h"

void playJumpSound() {
  if (sdk.hasWavSupport()) {
    (void)sdk.playWav(sdk.assetPath("boing.wav"));
  } else {
    sdk.playBeep(1300, 45);
  }
}

void playDuckSound() {
  if (sdk.hasWavSupport()) {
    (void)sdk.playWav(sdk.assetPath("baisse.wav"));
  } else {
    sdk.playBeep(850, 30);
  }
}

void playCrashSound() {
  if (sdk.hasWavSupport()) {
    (void)sdk.playWav(sdk.assetPath("tomber.wav"));
  } else {
    sdk.playBeep(240, 160);
  }
}