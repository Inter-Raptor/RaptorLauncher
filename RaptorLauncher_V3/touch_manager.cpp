#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "touch_manager.h"

// ======================================================
// ESP32-2432S032 - Tactile resistif XPT2046
// D'apres le schema fourni :
// TP_CLK = IO14
// TP_CS  = IO33
// TP_DIN = IO13
// TP_OUT = IO12
// TP_IRQ = IO36
// ======================================================

static const int TOUCH_CS   = 33;
static const int TOUCH_IRQ  = 36;
static const int TOUCH_CLK  = 14;
static const int TOUCH_MISO = 12;
static const int TOUCH_MOSI = 13;

static const int SCREEN_W = 320;
static const int SCREEN_H = 240;

// Calibration de depart
// A ajuster ensuite si le tactile repond mais est inverse/decale
static const int RAW_X_MIN = 200;
static const int RAW_X_MAX = 3800;
static const int RAW_Y_MIN = 200;
static const int RAW_Y_MAX = 3800;

// Bus SPI du tactile
static SPIClass touchSPI(VSPI);
static XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

static int mapAndClamp(int v, int inMin, int inMax, int outMin, int outMax) {
  long r = map(v, inMin, inMax, outMin, outMax);

  long minOut = (outMin < outMax) ? outMin : outMax;
  long maxOut = (outMin > outMax) ? outMin : outMax;

  if (r < minOut) r = minOut;
  if (r > maxOut) r = maxOut;

  return (int)r;
}

void touchInit() {
  Serial.println("[TOUCH] init debut");

  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);

  // GPIO36 = input only, pas de pullup interne
  pinMode(TOUCH_IRQ, INPUT);

  touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);

  if (!ts.begin(touchSPI)) {
    Serial.println("[TOUCH] ERREUR XPT2046 begin");
  } else {
    Serial.println("[TOUCH] XPT2046 begin OK");
  }

  ts.setRotation(1);

  Serial.print("[TOUCH] CS=");
  Serial.print(TOUCH_CS);
  Serial.print(" IRQ=");
  Serial.print(TOUCH_IRQ);
  Serial.print(" CLK=");
  Serial.print(TOUCH_CLK);
  Serial.print(" MISO=");
  Serial.print(TOUCH_MISO);
  Serial.print(" MOSI=");
  Serial.println(TOUCH_MOSI);

  Serial.println("[TOUCH] init fin");
}

bool touchPressed(int &x, int &y) {
  int irq = digitalRead(TOUCH_IRQ);

  // Sur XPT2046, IRQ est normalement actif a l'etat bas
  if (irq != 0) {
    return false;
  }

  TS_Point p = ts.getPoint();

  Serial.print("[XPT RAW] x=");
  Serial.print(p.x);
  Serial.print(" y=");
  Serial.print(p.y);
  Serial.print(" z=");
  Serial.println(p.z);

  // Filtres anti-lectures invalides
  if (p.z < 150) {
    return false;
  }

  if ((p.x == 0 && p.y == 0) || (p.x >= 4095 && p.y >= 4095)) {
    return false;
  }

  // Mapping provisoire en paysage
  // On ajustera ensuite si X/Y sont inverses
x = mapAndClamp(p.x, RAW_X_MIN, RAW_X_MAX, 0, SCREEN_W - 1);
y = mapAndClamp(p.y, RAW_Y_MIN, RAW_Y_MAX, SCREEN_H - 1, 0);

  Serial.print("[XPT MAP] x=");
  Serial.print(x);
  Serial.print(" y=");
  Serial.println(y);

  return true;
}