#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "touch_manager.h"
#include "settings_manager.h"

// ======================================================
// ESP32-2432S032 - Tactile resistif XPT2046
// ======================================================

static const int TOUCH_CS   = 33;
static const int TOUCH_IRQ  = 36;
static const int TOUCH_CLK  = 14;
static const int TOUCH_MISO = 12;
static const int TOUCH_MOSI = 13;

static const int SCREEN_W = 320;
static const int SCREEN_H = 240;

static int gRawXMin = 200;
static int gRawXMax = 3800;
static int gRawYMin = 200;
static int gRawYMax = 3800;

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

void touchSetCalibration(int xmin, int xmax, int ymin, int ymax) {
  gRawXMin = xmin;
  gRawXMax = xmax;
  gRawYMin = ymin;
  gRawYMax = ymax;
}

void touchGetCalibration(int &xmin, int &xmax, int &ymin, int &ymax) {
  xmin = gRawXMin;
  xmax = gRawXMax;
  ymin = gRawYMin;
  ymax = gRawYMax;
}

void touchResetCalibration() {
  gRawXMin = 200;
  gRawXMax = 3800;
  gRawYMin = 200;
  gRawYMax = 3800;
}

void touchInit() {
  Serial.println("[TOUCH] init debut");

  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);

  pinMode(TOUCH_IRQ, INPUT);

  touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);

  if (!ts.begin(touchSPI)) {
    Serial.println("[TOUCH] ERREUR XPT2046 begin");
  } else {
    Serial.println("[TOUCH] XPT2046 begin OK");
  }

  ts.setRotation(1);

  // charge calibration depuis settings
  touchSetCalibration(
    settingsGet().touch_x_min,
    settingsGet().touch_x_max,
    settingsGet().touch_y_min,
    settingsGet().touch_y_max
  );

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

bool touchReadRaw(int &x, int &y, int &z) {
  int irq = digitalRead(TOUCH_IRQ);

  if (irq != 0) {
    return false;
  }

  TS_Point p = ts.getPoint();

  x = p.x;
  y = p.y;
  z = p.z;

  if (p.z < 150) return false;
  if ((p.x == 0 && p.y == 0) || (p.x >= 4095 && p.y >= 4095)) return false;

  return true;
}

bool touchPressed(int &x, int &y) {
  int rawX, rawY, rawZ;
  if (!touchReadRaw(rawX, rawY, rawZ)) {
    return false;
  }

  x = mapAndClamp(rawX, gRawXMin, gRawXMax, 0, SCREEN_W - 1);
  y = mapAndClamp(rawY, gRawYMin, gRawYMax, SCREEN_H - 1, 0);

  return true;
}