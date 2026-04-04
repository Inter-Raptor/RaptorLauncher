#pragma once
#include <Arduino.h>

void wifiManagerInit();
void wifiManagerUpdate();

bool wifiManagerStart(const String& ssid, const String& pass);
void wifiManagerStop();
bool wifiManagerIsActive();
String wifiManagerGetIP();