#include <Arduino.h>
#include "launcher_ui.h"
#include "display_manager.h"
#include "touch_manager.h"

static bool gDrawn = false;
static int gTouchX = -1;
static int gTouchY = -1;
static bool gHasTouch = false;
static int gSelected = -1;

static int menuHitTest(int x, int y) {
  if (y >= 35 && y <= 60)   return 0; // Jeux
  if (y >= 65 && y <= 90)   return 1; // Parametres
  if (y >= 95 && y <= 120)  return 2; // Test tactile
  if (y >= 125 && y <= 150) return 3; // Test audio
  return -1;
}

static void drawMenu() {
  displayClear();

  displayDrawText(10, 10, "Raptor Launcher");

  displayDrawText(10, 40,  gSelected == 0 ? "> Jeux" : "  Jeux");
  displayDrawText(10, 70,  gSelected == 1 ? "> Parametres" : "  Parametres");
  displayDrawText(10, 100, gSelected == 2 ? "> Test tactile" : "  Test tactile");
  displayDrawText(10, 130, gSelected == 3 ? "> Test audio" : "  Test audio");

  char buf[64];
  snprintf(buf, sizeof(buf), "Touch: %d,%d   ", gTouchX, gTouchY);
  displayDrawText(10, 200, buf);
}

void launcherInit() {
  Serial.println("[LAUNCHER] init");
}

void launcherUpdate() {
  int x, y;
  if (touchPressed(x, y)) {
    gTouchX = x;
    gTouchY = y;
    gHasTouch = true;

    int hit = menuHitTest(x, y);
    if (hit != gSelected) {
      gSelected = hit;
      gDrawn = false;
    }

    Serial.print("[TOUCH] x=");
    Serial.print(x);
    Serial.print(" y=");
    Serial.print(y);
    Serial.print(" hit=");
    Serial.println(hit);
  }
}

void launcherRender() {
  if (!gDrawn) {
    gDrawn = true;
    drawMenu();
  }
}