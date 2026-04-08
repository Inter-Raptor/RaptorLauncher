#pragma once
#include <Arduino.h>

bool gameBootInit();
bool gameBootLaunchFromPath(const String& binPath);
bool gameBootMarkReturnToLauncher();
