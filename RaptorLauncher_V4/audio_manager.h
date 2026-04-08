#pragma once

void audioInit();
void audioSetVolume(int volumePercent);
int  audioGetVolume();
void audioTestBeep();
bool audioPlayWav(const char* path, int maxDurationMs = 1000);
