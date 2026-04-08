#include <Arduino.h>
#include "audio_manager.h"

static const int SPEAKER_PIN = 26;
static const int AUDIO_FREQ_DEFAULT = 2000;
static const int AUDIO_RESOLUTION = 8;   // 0..255
static const int AUDIO_MAX_DUTY = 255;

static int gVolumePercent = 80;

static int clampValue(int v, int minV, int maxV) {
  if (v < minV) return minV;
  if (v > maxV) return maxV;
  return v;
}

static int volumeToDuty(int volumePercent) {
  volumePercent = clampValue(volumePercent, 0, 100);

  if (volumePercent <= 0) return 0;

  float v = volumePercent / 100.0f;

  // courbe douce : très faible en bas, plus progressive ensuite
  int duty = (int)(v * v * 180.0f);

  if (volumePercent > 0 && duty < 1) duty = 1;

  return clampValue(duty, 0, AUDIO_MAX_DUTY);
}

void audioInit() {
  Serial.println("[AUDIO] init");

  ledcAttach(SPEAKER_PIN, AUDIO_FREQ_DEFAULT, AUDIO_RESOLUTION);
  ledcWrite(SPEAKER_PIN, 0);
}

void audioSetVolume(int volumePercent) {
  gVolumePercent = clampValue(volumePercent, 0, 100);

  Serial.print("[AUDIO] volume = ");
  Serial.println(gVolumePercent);
}

int audioGetVolume() {
  return gVolumePercent;
}

static void playTone(int freq, int durationMs) {
  int duty = volumeToDuty(gVolumePercent);

  if (duty <= 0) {
    delay(durationMs);
    return;
  }

  ledcWriteTone(SPEAKER_PIN, freq);
  ledcWrite(SPEAKER_PIN, duty);

  delay(durationMs);

  ledcWrite(SPEAKER_PIN, 0);
  ledcWriteTone(SPEAKER_PIN, 0);
  delay(30);
}

void audioTestBeep() {
  Serial.println("[AUDIO] test beep");
  playTone(1000, 150);
  playTone(1400, 150);
  playTone(1800, 200);
}