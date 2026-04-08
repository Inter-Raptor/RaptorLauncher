#include <Arduino.h>
#include <SD.h>
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

static uint32_t readLE32(File& f) {
  uint8_t b[4] = {0, 0, 0, 0};
  if (f.read(b, 4) != 4) return 0;
  return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

static uint16_t readLE16(File& f) {
  uint8_t b[2] = {0, 0};
  if (f.read(b, 2) != 2) return 0;
  return (uint16_t)b[0] | ((uint16_t)b[1] << 8);
}

bool audioPlayWav(const char* path, int maxDurationMs) {
  File wav = SD.open(path, FILE_READ);
  if (!wav) {
    Serial.printf("[AUDIO] WAV absent: %s\n", path);
    return false;
  }

  char riff[4], wave[4];
  if (wav.read((uint8_t*)riff, 4) != 4 || readLE32(wav) == 0 || wav.read((uint8_t*)wave, 4) != 4) {
    Serial.println("[AUDIO] WAV header invalide");
    wav.close();
    return false;
  }
  if (strncmp(riff, "RIFF", 4) != 0 || strncmp(wave, "WAVE", 4) != 0) {
    Serial.println("[AUDIO] format non WAVE");
    wav.close();
    return false;
  }

  uint16_t audioFormat = 0, channels = 0, bitsPerSample = 0;
  uint32_t sampleRate = 0, dataSize = 0, dataStart = 0;

  while (wav.available()) {
    char chunkId[4];
    if (wav.read((uint8_t*)chunkId, 4) != 4) break;
    uint32_t chunkSize = readLE32(wav);

    if (strncmp(chunkId, "fmt ", 4) == 0) {
      audioFormat = readLE16(wav);
      channels = readLE16(wav);
      sampleRate = readLE32(wav);
      (void)readLE32(wav);
      (void)readLE16(wav);
      bitsPerSample = readLE16(wav);
      if (chunkSize > 16) wav.seek(wav.position() + (chunkSize - 16));
    } else if (strncmp(chunkId, "data", 4) == 0) {
      dataStart = wav.position();
      dataSize = chunkSize;
      break;
    } else {
      wav.seek(wav.position() + chunkSize);
    }
  }

  if (audioFormat != 1 || channels != 1 || (bitsPerSample != 8 && bitsPerSample != 16) || sampleRate == 0 || dataSize == 0) {
    Serial.println("[AUDIO] WAV non supporte (PCM mono 8/16 bits requis)");
    wav.close();
    return false;
  }

  int dutyMax = volumeToDuty(gVolumePercent);
  if (dutyMax <= 0) {
    wav.close();
    return true;
  }

  uint32_t bytesPerSample = bitsPerSample / 8;
  uint32_t samplePeriodUs = 1000000UL / sampleRate;
  uint32_t bytesPlayed = 0;
  uint32_t startMs = millis();
  wav.seek(dataStart);

  while (bytesPlayed + bytesPerSample <= dataSize) {
    if (millis() - startMs >= (uint32_t)maxDurationMs) break;

    int sampleNorm = 0;
    if (bitsPerSample == 8) {
      int v = wav.read();
      if (v < 0) break;
      sampleNorm = v - 128;
      bytesPlayed += 1;
    } else {
      uint16_t lohi = readLE16(wav);
      int16_t s16 = (int16_t)lohi;
      sampleNorm = s16 / 256;
      bytesPlayed += 2;
    }

    int amp = abs(sampleNorm);
    int duty = (amp * dutyMax) / 128;
    ledcWrite(SPEAKER_PIN, duty);
    delayMicroseconds(samplePeriodUs);
  }

  ledcWrite(SPEAKER_PIN, 0);
  wav.close();
  Serial.printf("[AUDIO] WAV joue: %s\n", path);
  return true;
}
