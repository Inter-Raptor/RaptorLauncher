#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>

static WebServer server(80);
static bool wifiActive = false;
static String currentIP = "-";

// ---------- PAGE HTML ----------
String htmlPage() {
  String html = "<html><body>";
  html += "<h2>RaptorLauncher</h2>";

  html += "<h3>Fichiers SD:</h3><ul>";

  File root = SD.open("/");
  File file = root.openNextFile();

  while (file) {
    html += "<li>";
    html += file.name();
    html += " <a href='/delete?f=" + String(file.name()) + "'>❌</a>";
    html += "</li>";
    file = root.openNextFile();
  }

  html += "</ul>";

  html += "<h3>Upload:</h3>";
  html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
  html += "<input type='file' name='data'>";
  html += "<input type='submit' value='Upload'>";
  html += "</form>";

  html += "</body></html>";
  return html;
}

// ---------- ROUTES ----------
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleDelete() {
  if (server.hasArg("f")) {
    String path = "/" + server.arg("f");
    SD.remove(path);
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

File uploadFile;

void handleUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    String filename = "/" + upload.filename;
    uploadFile = SD.open(filename, FILE_WRITE);
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
  }
  else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) uploadFile.close();
  }
}

void handleUploadDone() {
  server.sendHeader("Location", "/");
  server.send(303);
}

// ---------- WIFI ----------
void wifiManagerStart(const String& ssid, const String& pass) {
  WiFi.begin(ssid.c_str(), pass.c_str());

  Serial.println("[WIFI] connexion...");

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiActive = true;
    currentIP = WiFi.localIP().toString();

    Serial.println("\n[WIFI] connecte !");
    Serial.println(currentIP);

    // ROUTES
    server.on("/", handleRoot);
    server.on("/delete", handleDelete);
    server.on("/upload", HTTP_POST, handleUploadDone, handleUpload);

    server.begin();
  } else {
    Serial.println("\n[WIFI] echec");
  }
}

void wifiManagerStop() {
  server.stop();
  WiFi.disconnect(true);
  wifiActive = false;
  currentIP = "-";
}

bool wifiManagerIsActive() {
  return wifiActive;
}

String wifiManagerGetIP() {
  return currentIP;
}

// IMPORTANT
void wifiManagerLoop() {
  if (wifiActive) {
    server.handleClient();
  }
}