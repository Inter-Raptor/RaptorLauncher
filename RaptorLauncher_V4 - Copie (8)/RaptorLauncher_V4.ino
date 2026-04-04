#include "config.h"
#include "types.h"
#include "display_manager.h"
#include "touch_manager.h"
#include "storage_manager.h"
#include "settings_manager.h"
#include "audio_manager.h"
#include "input_manager.h"
#include "launcher_ui.h"
#include "wifi_manager.h"
#include "led_manager.h"
#include "mcp23017_manager.h"

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println("=== RaptorLauncher V4 ===");

  displayInit();
  storageInit();

  settingsInit();

  touchInit();
  displaySetBrightness(map(settingsGet().brightness, 0, 100, 10, 255));

  audioInit();
  audioSetVolume(settingsGet().volume);
  ledManagerInit();
  ledManagerSetBrightness(settingsGet().led_brightness);
  mcp23017Init();

  wifiManagerInit();
  
  inputInit();
  launcherInit();

  Serial.println("Setup termine");
}

void loop() {
  wifiManagerUpdate();
  touchUpdate();   // <<< IMPORTANT : mise a jour tactile
  inputUpdate();
  launcherUpdate();
  launcherRender();
  delay(16);
}
