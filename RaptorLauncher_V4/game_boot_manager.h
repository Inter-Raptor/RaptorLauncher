#pragma once
#include <Arduino.h>

bool gameBootInit();
bool gameBootLaunchFromPath(const String& binPath);
bool gameBootMarkReturnToLauncher();
void gameBootHandleLauncherBoot();
void gameBootMarkLauncherStable();
bool gameBootIsPathBlocked(const String& binPath, String& reason);
