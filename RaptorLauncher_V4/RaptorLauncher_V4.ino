#include "config.h"
#include "types.h"
#include "display_manager.h"
#include "touch_manager.h"
#include "storage_manager.h"
#include "audio_manager.h"
#include "input_manager.h"
#include "launcher_ui.h"

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println("=== RaptorLauncher V4 ===");

  displayInit();
  touchInit();
  storageInit();
  audioInit();
  inputInit();
  launcherInit();

  Serial.println("Setup termine");
}

void loop() {
  inputUpdate();
  launcherUpdate();
  launcherRender();
  delay(16); // ~60 fps
}