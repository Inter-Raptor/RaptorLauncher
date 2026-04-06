#include "../../arduino_template/src/raptor_game_sdk.h"
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

static const int MAX_SCAN_ENTRIES = 40;
static const int MAX_DEVICE_ENTRIES = 128;
static const uint32_t WIFI_SCAN_REFRESH_MS = 10000;
static const uint32_t UI_REFRESH_MS = 120;

ScanEntry scanResults[MAX_SCAN_ENTRIES];
int scanCount = 0;
uint32_t lastScanMs = 0;

DeviceEntry devices[MAX_DEVICE_ENTRIES];
int deviceCount = 0;

ScreenMode mode = SCREEN_WIFI_SCAN;
int scrollY = 0;
int lastTouchY = 0;
bool dragging = false;

uint32_t lastUiMs = 0;
bool myNetConnected = false;
String myNetIp = "-";
String myNetSsid = "-";
String myNetError = "Pas connecte";
bool scanningDevices = false;
int scanProgress = 0;

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
  int viewportHeight = sdk.height() - 56;
  int maxScroll = contentHeight - viewportHeight;
  if (maxScroll < 0) maxScroll = 0;
  return maxScroll;
}

void handleTouchScroll(int maxScroll) {
  if (sdk.isTouchPressed()) {
    dragging = true;
    lastTouchY = sdk.touchY();
  }

  if (dragging && sdk.isTouchHeld()) {
    int y = sdk.touchY();
    int dy = y - lastTouchY;
    lastTouchY = y;
    scrollY -= dy;
    if (scrollY < 0) scrollY = 0;
    if (scrollY > maxScroll) scrollY = maxScroll;
  }

  if (sdk.isTouchReleased()) {
    dragging = false;
  }
}

void runWifiScan() {
  int found = WiFi.scanNetworks(false, true);
  if (found < 0) {
    scanCount = 0;
    return;
  }

  scanCount = (found > MAX_SCAN_ENTRIES) ? MAX_SCAN_ENTRIES : found;
  for (int i = 0; i < scanCount; ++i) {
    scanResults[i].ssid = WiFi.SSID(i);
    scanResults[i].rssi = WiFi.RSSI(i);
    scanResults[i].channel = (uint8_t)WiFi.channel(i);
    scanResults[i].encryption = WiFi.encryptionType(i);
  }
  WiFi.scanDelete();
}

bool loadCredentialsFromConfig(String& ssid, String& password) {
  if (!sdk.isSdReady()) {
    myNetError = "SD indisponible";
    return false;
  }

  String configPath = sdk.gameRootPath() + "/config.json";
  File f = SD.open(configPath, FILE_READ);
  if (!f) {
    myNetError = "config.json manquant";
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    myNetError = "config.json invalide";
    return false;
  }

  ssid = String((const char*)doc["ssid"]);
  password = String((const char*)doc["password"]);

  if (ssid.isEmpty()) {
    myNetError = "SSID vide";
    return false;
  }
  return true;
}

void discoverDevicesOnSubnet() {
  deviceCount = 0;
  scanningDevices = true;
  scanProgress = 0;

  if (!myNetConnected) {
    scanningDevices = false;
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

  pushDevice(localIp.toString(), "Cet appareil");
  if (gateway != INADDR_NONE) {
    pushDevice(gateway.toString(), "Routeur");
  }

  WiFiClient client;
  client.setTimeout(70);

  for (int host = 1; host <= 254 && deviceCount < MAX_DEVICE_ENTRIES; ++host) {
    scanProgress = host;
    IPAddress target(localIp[0], localIp[1], localIp[2], host);
    if (target == localIp || target == gateway) continue;

    bool alive = false;
    if (client.connect(target, 80, 60)) {
      alive = true;
      client.stop();
    } else if (client.connect(target, 443, 60)) {
      alive = true;
      client.stop();
    }

    if (alive) {
      pushDevice(target.toString(), "Actif (TCP)");
    }

    sdk.updateInputs();
    if (sdk.isPressed(BTN_B)) {
      break;
    }
  }

  scanningDevices = false;
}

void connectToMyNetwork() {
  String ssid;
  String password;
  if (!loadCredentialsFromConfig(ssid, password)) {
    myNetConnected = false;
    return;
  }

  myNetSsid = ssid;
  myNetError = "Connexion...";
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(150);
  WiFi.begin(ssid.c_str(), password.c_str());

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 12000) {
    sdk.updateInputs();
    if (sdk.isPressed(BTN_B)) {
      myNetError = "Connexion annulee";
      myNetConnected = false;
      WiFi.disconnect(true);
      return;
    }
    delay(25);
  }

  if (WiFi.status() == WL_CONNECTED) {
    myNetConnected = true;
    myNetIp = WiFi.localIP().toString();
    myNetError = "Connecte";
    discoverDevicesOnSubnet();
  } else {
    myNetConnected = false;
    myNetIp = "-";
    myNetError = "Echec connexion";
  }
}

void drawTopBar() {
  sdk.fillRect(0, 0, sdk.width(), 24, SDK_COLOR_ACCENT);
  sdk.drawSmallText(6, 6, mode == SCREEN_WIFI_SCAN ? "Menu 1: WiFi autour" : "Menu 2: Mon reseau", SDK_COLOR_BG, SDK_COLOR_ACCENT);

  sdk.fillRect(0, 24, sdk.width(), 26, SDK_COLOR_BG);
  sdk.drawSmallText(4, 28, "A: scan/refresh  B: retour  X: menu1  Y: menu2");
}

void drawWifiScanScreen() {
  int startY = 54;
  int rows = (scanCount > 0) ? scanCount : 1;
  int maxScroll = maxScrollForContent(rows);
  handleTouchScroll(maxScroll);

  sdk.fillRect(0, startY, sdk.width(), sdk.height() - startY, SDK_COLOR_BG);

  char header[64];
  snprintf(header, sizeof(header), "Reseaux detectes: %d", scanCount);
  sdk.drawSmallText(6, startY + 2, header, SDK_COLOR_OK, SDK_COLOR_BG);

  int yBase = startY + 18 - scrollY;
  for (int i = 0; i < scanCount; ++i) {
    int y = yBase + i * 14;
    if (y < startY || y > sdk.height() - 12) continue;

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
  int startY = 54;
  int baseRows = 4 + deviceCount;
  int rows = (baseRows > 6) ? baseRows : 6;
  int maxScroll = maxScrollForContent(rows);
  handleTouchScroll(maxScroll);

  sdk.fillRect(0, startY, sdk.width(), sdk.height() - startY, SDK_COLOR_BG);

  int y = startY + 2 - scrollY;
  char line[128];

  snprintf(line, sizeof(line), "SSID: %s", myNetSsid.c_str());
  if (y >= startY && y <= sdk.height() - 10) sdk.drawSmallText(6, y, line);
  y += 14;

  snprintf(line, sizeof(line), "Etat: %s", myNetError.c_str());
  if (y >= startY && y <= sdk.height() - 10) sdk.drawSmallText(6, y, line, myNetConnected ? SDK_COLOR_OK : SDK_COLOR_WARN, SDK_COLOR_BG);
  y += 14;

  snprintf(line, sizeof(line), "IP locale: %s", myNetIp.c_str());
  if (y >= startY && y <= sdk.height() - 10) sdk.drawSmallText(6, y, line);
  y += 18;

  if (scanningDevices) {
    snprintf(line, sizeof(line), "Scan appareils: %d/254 (B pour stop)", scanProgress);
    if (y >= startY && y <= sdk.height() - 10) sdk.drawSmallText(6, y, line, SDK_COLOR_ACCENT, SDK_COLOR_BG);
    y += 14;
  }

  if (deviceCount == 0) {
    if (y >= startY && y <= sdk.height() - 10) sdk.drawSmallText(6, y, "Aucun appareil detecte.");
    return;
  }

  if (y >= startY && y <= sdk.height() - 10) sdk.drawSmallText(6, y, "Appareils vus:", SDK_COLOR_OK, SDK_COLOR_BG);
  y += 14;

  for (int i = 0; i < deviceCount; ++i) {
    int lineY = y + i * 14;
    if (lineY < startY || lineY > sdk.height() - 10) continue;
    snprintf(line, sizeof(line), "%02d %s - %s", i + 1, devices[i].ip.c_str(), devices[i].status.c_str());
    sdk.drawSmallText(6, lineY, line);
  }
}

void setup() {
  sdk.begin();
  sdk.clear();
  sdk.drawCenteredText(6, "WiFi Touch Console");
  sdk.drawSmallText(4, 28, "Menu tactile: scan WiFi + reseau local");
  sdk.drawSmallText(4, 42, "X/Y changent de menu, glisser pour scroll");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(120);

  runWifiScan();
  lastScanMs = millis();
}

void loop() {
  sdk.updateInputs();

  if (sdk.isPressed(BTN_START)) {
    sdk.requestReturnToLauncher();
    return;
  }

  if (sdk.isPressed(BTN_X)) {
    mode = SCREEN_WIFI_SCAN;
    scrollY = 0;
    sdk.playBeep(1450, 25);
  }

  if (sdk.isPressed(BTN_Y)) {
    mode = SCREEN_MY_NETWORK;
    scrollY = 0;
    sdk.playBeep(1600, 25);
  }

  if (mode == SCREEN_WIFI_SCAN) {
    if (sdk.isPressed(BTN_A) || millis() - lastScanMs > WIFI_SCAN_REFRESH_MS) {
      runWifiScan();
      lastScanMs = millis();
    }
    drawTopBar();
    drawWifiScanScreen();
  } else {
    if (sdk.isPressed(BTN_A)) {
      connectToMyNetwork();
    }
    drawTopBar();
    drawMyNetworkScreen();
  }

  if (millis() - lastUiMs < UI_REFRESH_MS) {
    delay(6);
  }
  lastUiMs = millis();
}
