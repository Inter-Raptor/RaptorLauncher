#include <Arduino.h>
#include "launcher_ui.h"
#include "display_manager.h"
#include "touch_manager.h"

static bool gDrawn = false;
static int gTouchX = -1;
static int gTouchY = -1;
static bool gHasTouch = false;

void launcherInit() {
  Serial.println("[LAUNCHER] init");
}

void launcherUpdate() {
  int x, y;
  if (touchPressed(x, y)) {
    gTouchX = x;
    gTouchY = y;
    gHasTouch = true;

    Serial.print("[TOUCH] x=");
    Serial.print(x);
    Serial.print(" y=");
    Serial.println(y);
  }
}

void launcherRender() {
  if (!gDrawn) {
    gDrawn = true;
    displayClear();
    displayDrawText(10, 10, "Raptor Launcher");
    displayDrawText(10, 40, "Jeux");
    displayDrawText(10, 70, "Parametres");
    displayDrawText(10, 100, "Test tactile");
    displayDrawText(10, 130, "Test audio");
  }

  if (gHasTouch) {
    char buf[64];
    snprintf(buf, sizeof(buf), "Touch: %d , %d   ", gTouchX, gTouchY);
    displayDrawText(10, 200, buf);
  }
}