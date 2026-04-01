#include <Arduino.h>
#include "launcher_ui.h"
#include "display_manager.h"

static bool gFirstRender = true;

void launcherInit() {
  Serial.println("[LAUNCHER] init");
}

void launcherUpdate() {
  // Plus tard: navigation menu
}

void launcherRender() {
  if (!gFirstRender) return;
  gFirstRender = false;

  displayClear();
  displayDrawText(10, 10, "Raptor Launcher");
  displayDrawText(10, 30, "1. Jeux");
  displayDrawText(10, 50, "2. Parametres");
  displayDrawText(10, 70, "3. Test tactile");
  displayDrawText(10, 90, "4. Test audio");
}