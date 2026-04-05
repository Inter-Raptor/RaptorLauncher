#pragma once
#include <Arduino.h>

bool gbLaunchSavePendingRom(const String& romPath, const String& gameName);
bool gbLaunchLoadPendingRom(String& romPath, String& gameName);
