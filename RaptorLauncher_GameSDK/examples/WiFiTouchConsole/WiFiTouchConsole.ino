#include "raptor_game_sdk.h"
#include <ArduinoJson.h>
#include <WiFi.h>

RaptorGameSDK sdk;

enum ScreenMode {
  SCREEN_WIFI_SCAN = 0,
  SCREEN_MY_NETWORK = 1
};

struct ScanEntry {
  String ssid;
  int32_t rssi;
  uint8_t channel;
  wifi_auth_mode_t encryption;
};

struct DeviceEntry {
  String ip;
  String status;
};

struct Rect {
  int x;
  int y;
  int w;
  int h;
};

static const int MAX_SCAN_ENTRIES = 40;
static const int MAX_DEVICE_ENTRIES = 128;
static const uint16_t PROBE_PORTS[] = {80, 443, 53, 22, 445, 139, 1883, 554, 8008};
static const uint32_t WIFI_SCAN_REFRESH_MS = 10000;
static const uint32_t UI_THROTTLE_MS = 33;
static const uint32_t TOUCH_GUARD_MS = 1200;

static const int TOP_BAR_H = 24;
static const int ACTION_BAR_H = 26;
static const int CONTENT_START_Y = TOP_BAR_H + ACTION_BAR_H + 4;

ScanEntry scanResults[MAX_SCAN_ENTRIES];
int scanCount = 0;
uint32_t lastScanMs = 0;
bool initialScanPending = true;
bool wifiScanRunning = false;

DeviceEntry devices[MAX_DEVICE_ENTRIES];
int deviceCount = 0;

ScreenMode mode = SCREEN_WIFI_SCAN;
int scrollY = 0;
int lastTouchY = 0;
bool dragging = false;

uint32_t lastUiMs = 0;
uint32_t lastDrawMs = 0;
uint32_t bootMs = 0;
bool uiDirty = true;
bool touchInputArmed = false;

bool myNetConnected = false;
String myNetIp = "-";
String myNetSsid = "-";
String myNetError = "Pas connecte";
bool scanningDevices = false;
int scanProgress = 0;
int lanHost = 1;
WiFiClient lanClient;

bool pointInRect(int x, int y, const Rect& r) {
  return x >= r.x && x < (r.x + r.w) && y >= r.y && y < (r.y + r.h);
}

Rect tabWifiRect() {
  return {0, 0, sdk.width() / 2, TOP_BAR_H};
}

Rect tabMyNetRect() {
  return {sdk.width() / 2, 0, sdk.width() / 2 - 64, TOP_BAR_H};
}

Rect quitRect() {
  return {sdk.width() - 64, 0, 64, TOP_BAR_H};
}

Rect primaryActionRect() {
  return {0, TOP_BAR_H, sdk.width() / 2, ACTION_BAR_H};
}

Rect secondaryActionRect() {
  return {sdk.width() / 2, TOP_BAR_H, sdk.width() / 2, ACTION_BAR_H};
}

Rect cancelScanRect() {
  return {0, sdk.height() - 24, sdk.width(), 24};
}

String authToText(wifi_auth_mode_t auth) {
  switch (auth) {
    case WIFI_AUTH_OPEN: return "OPEN";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-ENT";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/3";
    default: return "?";
  }
}

int maxScrollForContent(int contentRows) {
  int contentHeight = contentRows * 14 + 8;
  int viewportHeight = sdk.height() - CONTENT_START_Y;
  int maxScroll = contentHeight - viewportHeight;
  if (maxScroll < 0) maxScroll = 0;
  return maxScroll;
}

bool handleTouchScroll(int maxScroll) {
  bool changed = false;

  if (sdk.isTouchPressed() && sdk.touchY() >= CONTENT_START_Y) {
    dragging = true;
    lastTouchY = sdk.touchY();
  }

  if (dragging && sdk.isTouchHeld()) {
    int y = sdk.touchY();
    int dy = y - lastTouchY;
    lastTouchY = y;
    int oldScroll = scrollY;
    scrollY -= dy;
    if (scrollY < 0) scrollY = 0;
    if (scrollY > maxScroll) scrollY = maxScroll;
    changed = (oldScroll != scrollY);
  }

  if (sdk.isTouchReleased()) {
    dragging = false;
  }

  return changed;
}

void startWifiScan() {
  if (wifiScanRunning) return;
  WiFi.scanDelete();
  // async = true pour ne pas bloquer le launcher au demarrage.
  WiFi.scanNetworks(true, true);
  wifiScanRunning = true;
  uiDirty = true;
}

void updateWifiScan() {
  if (!wifiScanRunning) return;

  int found = WiFi.scanComplete();
  if (found == WIFI_SCAN_RUNNING) return;

  if (found < 0) {
    scanCount = 0;
  } else {
    scanCount = (found > MAX_SCAN_ENTRIES) ? MAX_SCAN_ENTRIES : found;
    for (int i = 0; i < scanCount; ++i) {
      scanResults[i].ssid = WiFi.SSID(i);
      scanResults[i].rssi = WiFi.RSSI(i);
      scanResults[i].channel = (uint8_t)WiFi.channel(i);
      scanResults[i].encryption = WiFi.encryptionType(i);
    }
  }

  WiFi.scanDelete();
  wifiScanRunning = false;
  lastScanMs = millis();
  uiDirty = true;
}

void beginDeviceScan() {
  deviceCount = 0;
  scanningDevices = true;
  scanProgress = 0;
  lanHost = 1;
  lanClient.setTimeout(70);
  uiDirty = true;
}

void stopDeviceScan(const char* reason = nullptr) {
  scanningDevices = false;
  if (reason) myNetError = reason;
  uiDirty = true;
}

void updateDeviceScanStep() {
  if (!scanningDevices) return;
  if (!myNetConnected) {
    stopDeviceScan();
    return;
  }

  IPAddress localIp = WiFi.localIP();
  IPAddress gateway = WiFi.gatewayIP();

  auto pushDevice = [&](const String& ip, const String& status) {
    if (deviceCount >= MAX_DEVICE_ENTRIES) return;
    devices[deviceCount].ip = ip;
    devices[deviceCount].status = status;
    deviceCount++;
  };

  if (deviceCount == 0) {
    pushDevice(localIp.toString(), "Cet appareil");
    if (gateway != INADDR_NONE) {
      pushDevice(gateway.toString(), "Routeur");
    }
  }

  while (lanHost <= 254 && (IPAddress(localIp[0], localIp[1], localIp[2], lanHost) == localIp ||
         IPAddress(localIp[0], localIp[1], localIp[2], lanHost) == gateway)) {
    lanHost++;
  }

  if (lanHost > 254 || deviceCount >= MAX_DEVICE_ENTRIES) {
    stopDeviceScan();
    return;
  }

  scanProgress = lanHost;
  IPAddress target(localIp[0], localIp[1], localIp[2], lanHost);
  bool alive = false;
  uint16_t hitPort = 0;
  for (size_t pi = 0; pi < (sizeof(PROBE_PORTS) / sizeof(PROBE_PORTS[0])); ++pi) {
    uint16_t port = PROBE_PORTS[pi];
    if (lanClient.connect(target, port, 30)) {
      alive = true;
      hitPort = port;
      lanClient.stop();
      break;
    }
  }

  if (alive) {
    char status[28];
    snprintf(status, sizeof(status), "Actif (TCP:%u)", (unsigned)hitPort);
    pushDevice(target.toString(), String(status));
  }

  lanHost++;
  uiDirty = true;
}

bool loadWifiCredentialsFromSettings(String& ssid, String& pass) {
  JsonDocument doc;
  if (!sdk.loadLauncherSettings(doc)) {
    myNetError = "settings.json manquant";
    return false;
  }

  ssid = String((const char*)doc["wifi_ssid"]);
  pass = String((const char*)doc["wifi_pass"]);

  // Compatibilite anciens noms.
  if (ssid.isEmpty()) {
    ssid = String((const char*)doc["ssid"]);
    pass = String((const char*)doc["password"]);
  }

  if (ssid.isEmpty()) {
    myNetError = "wifi_ssid vide";
    return false;
  }
  return true;
}

void connectToMyNetwork() {
  String ssid;
  String pass;
  if (!loadWifiCredentialsFromSettings(ssid, pass)) {
    myNetConnected = false;
    myNetIp = "-";
    uiDirty = true;
    return;
  }

  myNetSsid = ssid;
  myNetError = "Connexion...";
  uiDirty = true;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
  delay(40);
  WiFi.begin(ssid.c_str(), pass.c_str());

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 12000) {
    sdk.updateInputs();
    if (sdk.isTouchPressed() && pointInRect(sdk.touchX(), sdk.touchY(), cancelScanRect())) {
      WiFi.disconnect(false, false);
      myNetError = "Connexion annulee";
      myNetConnected = false;
      myNetIp = "-";
      uiDirty = true;
      return;
    }
    delay(20);
  }

  if (WiFi.status() == WL_CONNECTED) {
    myNetConnected = true;
    myNetIp = WiFi.localIP().toString();
    myNetError = "Connecte";
    beginDeviceScan();
  } else {
    myNetConnected = false;
    myNetIp = "-";
    myNetError = "Echec connexion";
  }
  uiDirty = true;
}

void drawTopBars() {
  Rect wifiR = tabWifiRect();
  Rect netR = tabMyNetRect();
  Rect quitR = quitRect();

  sdk.fillRect(0, 0, sdk.width(), TOP_BAR_H, SDK_COLOR_BG);

  sdk.fillRect(wifiR.x, wifiR.y, wifiR.w, wifiR.h, mode == SCREEN_WIFI_SCAN ? SDK_COLOR_ACCENT : SDK_COLOR_WARN);
  sdk.drawSmallText(wifiR.x + 6, wifiR.y + 7, "Menu 1 WiFi", mode == SCREEN_WIFI_SCAN ? SDK_COLOR_BG : SDK_COLOR_TEXT, mode == SCREEN_WIFI_SCAN ? SDK_COLOR_ACCENT : SDK_COLOR_WARN);

  sdk.fillRect(netR.x, netR.y, netR.w, netR.h, mode == SCREEN_MY_NETWORK ? SDK_COLOR_ACCENT : SDK_COLOR_WARN);
  sdk.drawSmallText(netR.x + 6, netR.y + 7, "Menu 2 Reseau", mode == SCREEN_MY_NETWORK ? SDK_COLOR_BG : SDK_COLOR_TEXT, mode == SCREEN_MY_NETWORK ? SDK_COLOR_ACCENT : SDK_COLOR_WARN);

  sdk.fillRect(quitR.x, quitR.y, quitR.w, quitR.h, SDK_COLOR_ERROR);
  sdk.drawSmallText(quitR.x + 14, quitR.y + 7, "Quitter", SDK_COLOR_BG, SDK_COLOR_ERROR);

  Rect p = primaryActionRect();
  Rect s = secondaryActionRect();
  sdk.fillRect(p.x, p.y, p.w, p.h, SDK_COLOR_BG);
  sdk.fillRect(s.x, s.y, s.w, s.h, SDK_COLOR_BG);

  if (mode == SCREEN_WIFI_SCAN) {
    sdk.drawSmallText(p.x + 6, p.y + 8, "Scanner WiFi", SDK_COLOR_OK, SDK_COLOR_BG);
    sdk.drawSmallText(s.x + 6, s.y + 8, wifiScanRunning ? "Scan..." : "Auto 10s", SDK_COLOR_TEXT, SDK_COLOR_BG);
  } else {
    sdk.drawSmallText(p.x + 6, p.y + 8, "Se connecter", SDK_COLOR_OK, SDK_COLOR_BG);
    sdk.drawSmallText(s.x + 6, s.y + 8, scanningDevices ? "Scan en cours..." : "Rescanner LAN", SDK_COLOR_TEXT, SDK_COLOR_BG);
  }

  sdk.fillRect(0, TOP_BAR_H - 1, sdk.width(), 1, SDK_COLOR_WARN);
  sdk.fillRect(0, TOP_BAR_H + ACTION_BAR_H - 1, sdk.width(), 1, SDK_COLOR_WARN);
}

void drawWifiScanScreen() {
  int rows = (scanCount > 0) ? scanCount : 1;
  int maxScroll = maxScrollForContent(rows);
  if (handleTouchScroll(maxScroll)) uiDirty = true;

  sdk.fillRect(0, CONTENT_START_Y, sdk.width(), sdk.height() - CONTENT_START_Y, SDK_COLOR_BG);

  char header[64];
  snprintf(header, sizeof(header), "Reseaux detectes: %d", scanCount);
  sdk.drawSmallText(6, CONTENT_START_Y + 2, header, SDK_COLOR_OK, SDK_COLOR_BG);

  int yBase = CONTENT_START_Y + 18 - scrollY;
  for (int i = 0; i < scanCount; ++i) {
    int y = yBase + i * 14;
    if (y < CONTENT_START_Y || y > sdk.height() - 12) continue;

    char line[128];
    const String ssid = scanResults[i].ssid.isEmpty() ? String("<SSID cache>") : scanResults[i].ssid;
    snprintf(line, sizeof(line), "%02d %-14s %4lddBm CH%02u %s",
             i + 1,
             ssid.substring(0, 14).c_str(),
             (long)scanResults[i].rssi,
             (unsigned)scanResults[i].channel,
             authToText(scanResults[i].encryption).c_str());
    sdk.drawSmallText(6, y, line);
  }
}

void drawMyNetworkScreen() {
  int baseRows = 4 + deviceCount;
  int rows = (baseRows > 6) ? baseRows : 6;
  int maxScroll = maxScrollForContent(rows);
  if (handleTouchScroll(maxScroll)) uiDirty = true;

  sdk.fillRect(0, CONTENT_START_Y, sdk.width(), sdk.height() - CONTENT_START_Y, SDK_COLOR_BG);

  int y = CONTENT_START_Y + 2 - scrollY;
  char line[128];

  snprintf(line, sizeof(line), "SSID: %s", myNetSsid.c_str());
  if (y >= CONTENT_START_Y && y <= sdk.height() - 10) sdk.drawSmallText(6, y, line);
  y += 14;

  snprintf(line, sizeof(line), "Etat: %s", myNetError.c_str());
  if (y >= CONTENT_START_Y && y <= sdk.height() - 10) sdk.drawSmallText(6, y, line, myNetConnected ? SDK_COLOR_OK : SDK_COLOR_WARN, SDK_COLOR_BG);
  y += 14;

  snprintf(line, sizeof(line), "IP locale: %s", myNetIp.c_str());
  if (y >= CONTENT_START_Y && y <= sdk.height() - 10) sdk.drawSmallText(6, y, line);
  y += 18;

  if (scanningDevices) {
    snprintf(line, sizeof(line), "Scan appareils: %d/254", scanProgress);
    if (y >= CONTENT_START_Y && y <= sdk.height() - 10) sdk.drawSmallText(6, y, line, SDK_COLOR_ACCENT, SDK_COLOR_BG);
    y += 14;
  }

  if (deviceCount == 0) {
    if (y >= CONTENT_START_Y && y <= sdk.height() - 10) sdk.drawSmallText(6, y, "Aucun appareil detecte.");
  } else {
    if (y >= CONTENT_START_Y && y <= sdk.height() - 10) sdk.drawSmallText(6, y, "Appareils vus:", SDK_COLOR_OK, SDK_COLOR_BG);
    y += 14;

    for (int i = 0; i < deviceCount; ++i) {
      int lineY = y + i * 14;
      if (lineY < CONTENT_START_Y || lineY > sdk.height() - 10) continue;
      snprintf(line, sizeof(line), "%02d %s - %s", i + 1, devices[i].ip.c_str(), devices[i].status.c_str());
      sdk.drawSmallText(6, lineY, line);
    }
  }

  if (scanningDevices || myNetError == "Connexion...") {
    Rect c = cancelScanRect();
    sdk.fillRect(c.x, c.y, c.w, c.h, SDK_COLOR_ERROR);
    sdk.drawSmallText(8, c.y + 7, "Touchez ici pour annuler", SDK_COLOR_BG, SDK_COLOR_ERROR);
  }
}

void drawScreen() {
  drawTopBars();
  if (mode == SCREEN_WIFI_SCAN) {
    drawWifiScanScreen();
  } else {
    drawMyNetworkScreen();
  }
}

void handleTapActions() {
  if (!touchInputArmed) {
    if (millis() - bootMs < TOUCH_GUARD_MS) {
      return;
    }
    if (!sdk.isTouchHeld() || sdk.isTouchReleased()) {
      touchInputArmed = true;
    }
    return;
  }

  if (!sdk.isTouchPressed()) return;

  int tx = sdk.touchX();
  int ty = sdk.touchY();

  if (pointInRect(tx, ty, quitRect())) {
    sdk.requestReturnToLauncher();
    return;
  }

  if (pointInRect(tx, ty, tabWifiRect())) {
    mode = SCREEN_WIFI_SCAN;
    scrollY = 0;
    uiDirty = true;
    return;
  }

  if (pointInRect(tx, ty, tabMyNetRect())) {
    mode = SCREEN_MY_NETWORK;
    scrollY = 0;
    uiDirty = true;
    return;
  }

  if (pointInRect(tx, ty, primaryActionRect())) {
    if (mode == SCREEN_WIFI_SCAN) {
      startWifiScan();
    } else {
      connectToMyNetwork();
    }
    uiDirty = true;
    return;
  }

  if (pointInRect(tx, ty, secondaryActionRect()) && mode == SCREEN_MY_NETWORK && myNetConnected && !scanningDevices) {
    beginDeviceScan();
    uiDirty = true;
  }
}

void setup() {
  sdk.begin();
  sdk.clear();
  bootMs = millis();

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
  scanCount = 0;
  initialScanPending = true;
  uiDirty = true;
}

void loop() {
  sdk.updateInputs();
  updateWifiScan();
  updateDeviceScanStep();

  if (scanningDevices && sdk.isTouchPressed() && pointInRect(sdk.touchX(), sdk.touchY(), cancelScanRect())) {
    stopDeviceScan("Scan annule");
  }

  handleTapActions();

  if (initialScanPending && millis() - bootMs > 500) {
    startWifiScan();
    initialScanPending = false;
  }

  if (!initialScanPending && mode == SCREEN_WIFI_SCAN && !wifiScanRunning && (millis() - lastScanMs > WIFI_SCAN_REFRESH_MS)) {
    startWifiScan();
  }

  if (uiDirty && (millis() - lastDrawMs >= UI_THROTTLE_MS)) {
    drawScreen();
    uiDirty = false;
    lastDrawMs = millis();
  }

  if (millis() - lastUiMs < 4) delay(1);
  lastUiMs = millis();
}
