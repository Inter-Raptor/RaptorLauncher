#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include "wifi_manager.h"

static WebServer server(80);
static bool gWifiActive = false;

static void handleRoot() {
  String html;
  html += "<html><head><meta charset='utf-8'><title>RaptorLauncher</title></head><body>";
  html += "<h1>RaptorLauncher</h1>";
  html += "<p>WiFi actif</p>";
  html += "<h2>/games</h2><ul>";

  File root = SD.open("/games");
  if (root && root.isDirectory()) {
    File file = root.openNextFile();
    while (file) {
      html += "<li>";
      html += String(file.name());
      html += "</li>";
      file = root.openNextFile();
    }
  } else {
    html += "<li>Dossier /games introuvable</li>";
  }

  html += "</ul>";
  html += "</body></html>";

  server.send(200, "text/html; charset=utf-8", html);
}

void wifiManagerInit() {
  WiFi.mode(WIFI_OFF);
  gWifiActive = false;
}

bool wifiManagerStart(const String& ssid, const String& pass) {
  if (ssid.length() == 0) {
    Serial.println("[WIFI] SSID vide");
    return false;
  }

  Serial.println("[WIFI] connexion...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 30) {
    delay(500);
    Serial.print(".");
    timeout++;
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] connexion echouee");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    gWifiActive = false;
    return false;
  }

  server.on("/", handleRoot);
  server.begin();

  gWifiActive = true;

  Serial.print("[WIFI] connecte IP = ");
  Serial.println(WiFi.localIP());

  return true;
}

void wifiManagerStop() {
  server.stop();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  gWifiActive = false;
  Serial.println("[WIFI] OFF");
}

void wifiManagerUpdate() {
  if (gWifiActive) {
    server.handleClient();
  }
}

bool wifiManagerIsActive() {
  return gWifiActive;
}

String wifiManagerGetIP() {
  if (!gWifiActive) return "";
  return WiFi.localIP().toString();
}