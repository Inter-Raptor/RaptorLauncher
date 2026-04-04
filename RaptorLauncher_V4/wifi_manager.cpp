#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include "wifi_manager.h"

static WebServer server(80);
static bool gWifiActive = false;
static String gWifiIP = "";

static File gUploadFile;
static String gUploadDir = "/";

// --------------------------------------------------
// Helpers
// --------------------------------------------------
static String htmlHeader(const String& title) {
  String h;
  h += "<!doctype html><html><head><meta charset='utf-8'>";
  h += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  h += "<title>";
  h += title;
  h += "</title>";
  h += "<style>";
  h += "body{font-family:Arial,sans-serif;background:#111;color:#eee;padding:16px;}";
  h += "a{color:#6cf;text-decoration:none;}";
  h += "a:hover{text-decoration:underline;}";
  h += "button,input,textarea{font-size:16px;}";
  h += ".card{background:#1b1b1b;padding:12px;border-radius:10px;margin-bottom:14px;}";
  h += ".danger{color:#f88;}";
  h += ".ok{color:#8f8;}";
  h += ".muted{color:#aaa;}";
  h += "textarea{width:100%;height:360px;background:#000;color:#0f0;border:1px solid #444;}";
  h += "ul{padding-left:20px;}";
  h += "li{margin-bottom:8px;}";
  h += "</style></head><body>";
  return h;
}

static String htmlFooter() {
  return "</body></html>";
}

static String normalizePath(String path) {
  if (path.length() == 0) return "/";
  if (!path.startsWith("/")) path = "/" + path;
    while (path.length() > 1 && path.endsWith("/")) {
    path.remove(path.length() - 1);
  }
  return path;
}

static String getPathArg() {
  if (!server.hasArg("f")) return "";
  return normalizePath(server.arg("f"));
}

static String getDirArg() {
  if (!server.hasArg("d")) return "/";
  return normalizePath(server.arg("d"));
}

static String getBaseName(const String& path) {
  int p = path.lastIndexOf('/');
  if (p < 0) return path;
  return path.substring(p + 1);
}

static String getParentDir(const String& path) {
  if (path.length() <= 1) return "/";
  int p = path.lastIndexOf('/');
  if (p <= 0) return "/";
  return path.substring(0, p);
}

static String joinPath(const String& base, const String& name) {
  if (base == "/" || base.length() == 0) return "/" + name;
  return base + "/" + name;
}

static String htmlEscape(const String& s) {
  String out;
  out.reserve(s.length() + 16);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '&') out += "&amp;";
    else if (c == '<') out += "&lt;";
    else if (c == '>') out += "&gt;";
    else if (c == '"') out += "&quot;";
    else out += c;
  }
  return out;
}

static String urlEncode(const String& s) {
  String out;
  const char* hex = "0123456789ABCDEF";
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '-' || c == '_' || c == '.' || c == '~' || c == '/') {
      out += c;
    } else {
      out += '%';
      out += hex[(c >> 4) & 0x0F];
      out += hex[c & 0x0F];
    }
  }
  return out;
}

static String cleanName(String name) {
  name.trim();
  name.replace("\\", "");
  name.replace("/", "");
  return name;
}

static String contentTypeFromFilename(const String& path) {
  if (path.endsWith(".html")) return "text/html";
  if (path.endsWith(".htm"))  return "text/html";
  if (path.endsWith(".css"))  return "text/css";
  if (path.endsWith(".js"))   return "application/javascript";
  if (path.endsWith(".json")) return "application/json";
  if (path.endsWith(".txt"))  return "text/plain";
  if (path.endsWith(".bmp"))  return "image/bmp";
  if (path.endsWith(".raw"))  return "application/octet-stream";
  if (path.endsWith(".bin"))  return "application/octet-stream";
  return "application/octet-stream";
}

static void redirectTo(const String& path) {
  server.sendHeader("Location", path, true);
  server.send(303, "text/plain", "");
}

// --------------------------------------------------
// Arborescence
// --------------------------------------------------
static void listDirectory(String& html, const String& dirPath) {
  File root = SD.open(dirPath);
  if (!root || !root.isDirectory()) {
    html += "<li class='danger'>Dossier introuvable : ";
    html += htmlEscape(dirPath);
    html += "</li>";
    return;
  }

  File file = root.openNextFile();
  while (file) {
    String name = String(file.name());
    name = getBaseName(name);
    String fullPath = joinPath(dirPath, name);

    html += "<li>";
    html += htmlEscape(name);

    if (file.isDirectory()) {
      html += " <span class='muted'>(dossier)</span> ";
      html += "<a href='/?d=" + urlEncode(fullPath) + "'>Ouvrir</a>";
    } else {
      html += " [";
      html += file.size();
      html += " o] ";

      html += "<a href='/download?f=" + urlEncode(fullPath) + "'>Télécharger</a>";
      html += " | ";
      html += "<a class='danger' href='/delete?f=" + urlEncode(fullPath) + "&d=" + urlEncode(dirPath) + "' onclick=\"return confirm('Supprimer ";
      html += htmlEscape(fullPath);
      html += " ?');\">Supprimer</a>";
      html += " | ";
      html += "<a href='/rename?f=" + urlEncode(fullPath) + "&d=" + urlEncode(dirPath) + "'>Renommer</a>";

      if (fullPath.endsWith(".json")) {
        html += " | ";
        html += "<a href='/edit?f=" + urlEncode(fullPath) + "&d=" + urlEncode(dirPath) + "'>Éditer JSON</a>";
      }
    }

    html += "</li>";
    file = root.openNextFile();
  }
}

// --------------------------------------------------
// Routes
// --------------------------------------------------
static void handleRoot() {
    String currentDir = getDirArg();

  String html = htmlHeader("RaptorLauncher");
  html += "<h1>RaptorLauncher</h1>";

  html += "<div class='card'>";
  html += "<div class='ok'>Wi-Fi actif</div>";
  html += "<div>IP : ";
  html += gWifiIP;
  html += "</div>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h2>Dossier courant</h2>";
  html += "<div><code>";
  html += htmlEscape(currentDir);
  html += "</code></div>";
  if (currentDir != "/") {
    html += "<a href='/?d=" + urlEncode(getParentDir(currentDir)) + "'>⬅ Retour dossier parent</a>";
  }
  html += "</div>";

  html += "<div class='card'>";
  html += "<h2>Uploader / remplacer un fichier</h2>";
  html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
  html += "<input type='hidden' name='d' value='" + htmlEscape(currentDir) + "'>";
  html += "<input type='file' name='data'>";
  html += "<button type='submit'>Envoyer</button>";
  html += "</form>";
  html += "<p>Le fichier est envoyé dans le dossier courant. ";
  html += "Si un fichier du même nom existe déjà, il sera remplacé.</p>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h2>Nouveau dossier</h2>";
  html += "<form method='POST' action='/mkdir'>";
  html += "<input type='hidden' name='d' value='" + htmlEscape(currentDir) + "'>";
  html += "<input type='text' name='name' placeholder='Nom du dossier'>";
  html += "<button type='submit'>Créer</button>";
  html += "</form>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h2>Fichiers</h2><ul>";
  listDirectory(html, currentDir);
  html += "</ul></div>";

  html += htmlFooter();
  server.send(200, "text/html; charset=utf-8", html);
}

static void handleDownload() {
  String path = getPathArg();
  if (path.length() == 0 || !SD.exists(path)) {
    server.send(404, "text/plain; charset=utf-8", "Fichier introuvable");
    return;
  }

  File file = SD.open(path, FILE_READ);
  if (!file) {
    server.send(500, "text/plain; charset=utf-8", "Impossible d'ouvrir le fichier");
    return;
  }

  server.sendHeader("Content-Disposition", "attachment; filename=\"" + getBaseName(path) + "\"");
  server.streamFile(file, contentTypeFromFilename(path));
  file.close();
}

static void handleDelete() {
  String path = getPathArg();
  String dir = getDirArg();
  if (path.length() == 0) {
    server.send(400, "text/plain; charset=utf-8", "Paramètre manquant");
    return;
  }

  if (SD.exists(path)) {
    SD.remove(path);
  }

  redirectTo("/?d=" + urlEncode(dir));
}

static void handleEditJson() {
  String path = getPathArg();
  String dir = getDirArg();
  if (path.length() == 0 || !SD.exists(path) || !path.endsWith(".json")) {
    server.send(404, "text/plain; charset=utf-8", "JSON introuvable");
    return;
  }

  File file = SD.open(path, FILE_READ);
  if (!file) {
    server.send(500, "text/plain; charset=utf-8", "Impossible d'ouvrir le JSON");
    return;
  }

  String content;
  while (file.available()) {
    content += (char)file.read();
  }
  file.close();

  String html = htmlHeader("Éditer JSON");
  html += "<h1>Éditer JSON</h1>";
  html += "<div class='card'>";
  html += "<div>Fichier : ";
  html += htmlEscape(path);
  html += "</div>";
  html += "<form method='POST' action='/savejson'>";
  html += "<input type='hidden' name='f' value='" + htmlEscape(path) + "'>";
  html += "<input type='hidden' name='d' value='" + htmlEscape(dir) + "'>";
  html += "<textarea name='content'>";
  html += htmlEscape(content);
  html += "</textarea><br><br>";
  html += "<button type='submit'>Enregistrer</button> ";
  html += "<a href='/?d=" + urlEncode(dir) + "'>Retour</a>";;
  html += "</form>";
  html += "</div>";
  html += htmlFooter();

  server.send(200, "text/html; charset=utf-8", html);
}

static void handleSaveJson() {
  if (!server.hasArg("f") || !server.hasArg("content")) {
    server.send(400, "text/plain; charset=utf-8", "Paramètres manquants");
    return;
  }

  String path = normalizePath(server.arg("f"));
  String dir = getDirArg();
  String content = server.arg("content");

  if (SD.exists(path)) {
    SD.remove(path);
  }

  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    server.send(500, "text/plain; charset=utf-8", "Impossible d'écrire le fichier");
    return;
  }

  file.print(content);
  file.close();

  redirectTo("/?d=" + urlEncode(dir));
}

static void handleMkdir() {
  if (!server.hasArg("name")) {
    server.send(400, "text/plain; charset=utf-8", "Nom manquant");
    return;
  }

  String dir = getDirArg();
  String name = cleanName(server.arg("name"));
  if (name.length() == 0) {
    server.send(400, "text/plain; charset=utf-8", "Nom invalide");
    return;
  }

  String fullPath = joinPath(dir, name);
  if (!SD.exists(fullPath)) {
    SD.mkdir(fullPath);
  }

  redirectTo("/?d=" + urlEncode(dir));
}

static void handleRenameForm() {
  String path = getPathArg();
  String dir = getDirArg();
  if (path.length() == 0 || !SD.exists(path)) {
    server.send(404, "text/plain; charset=utf-8", "Fichier introuvable");
    return;
  }

  String html = htmlHeader("Renommer");
  html += "<h1>Renommer</h1>";
  html += "<div class='card'>";
  html += "<div>Fichier : <code>" + htmlEscape(path) + "</code></div>";
  html += "<form method='POST' action='/rename'>";
  html += "<input type='hidden' name='f' value='" + htmlEscape(path) + "'>";
  html += "<input type='hidden' name='d' value='" + htmlEscape(dir) + "'>";
  html += "<input type='text' name='newname' value='" + htmlEscape(getBaseName(path)) + "'>";
  html += "<button type='submit'>Renommer</button> ";
  html += "<a href='/?d=" + urlEncode(dir) + "'>Annuler</a>";
  html += "</form></div>";
  html += htmlFooter();
  server.send(200, "text/html; charset=utf-8", html);
}

static void handleRenameAction() {
  if (!server.hasArg("f") || !server.hasArg("newname")) {
    server.send(400, "text/plain; charset=utf-8", "Paramètres manquants");
    return;
  }

  String source = normalizePath(server.arg("f"));
  String dir = getDirArg();
  String newName = cleanName(server.arg("newname"));
  if (newName.length() == 0) {
    server.send(400, "text/plain; charset=utf-8", "Nouveau nom invalide");
    return;
  }

  String target = joinPath(getParentDir(source), newName);
  if (!SD.exists(source)) {
    server.send(404, "text/plain; charset=utf-8", "Source introuvable");
    return;
  }
  if (source != target) {
    SD.rename(source, target);
  }

  redirectTo("/?d=" + urlEncode(dir));
}

static void handleUpload() {
  HTTPUpload& upload = server.upload();


  if (upload.status == UPLOAD_FILE_START) {
    gUploadDir = getDirArg();
    String name = getBaseName(upload.filename);
    name = cleanName(name);
    if (name.length() == 0) {
      return;
    }
    String filename = joinPath(gUploadDir, name);

    if (SD.exists(filename)) {
      SD.remove(filename);
    }

    gUploadFile = SD.open(filename, FILE_WRITE);
    Serial.print("[WIFI] upload start: ");
    Serial.println(filename);
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (gUploadFile) {
      gUploadFile.write(upload.buf, upload.currentSize);
    }
  }
  else if (upload.status == UPLOAD_FILE_END) {
    if (gUploadFile) {
      gUploadFile.close();
    }
    Serial.print("[WIFI] upload end, size=");
    Serial.println(upload.totalSize);
  }
}

static void handleUploadDone() {
  String dir = getDirArg();
  redirectTo("/?d=" + urlEncode(dir));
}

// --------------------------------------------------
// API
// --------------------------------------------------
void wifiManagerInit() {
  WiFi.mode(WIFI_OFF);
  gWifiActive = false;
  gWifiIP = "";
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
    gWifiIP = "";
    return false;
  }

  gWifiActive = true;
  gWifiIP = WiFi.localIP().toString();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/download", HTTP_GET, handleDownload);
  server.on("/delete", HTTP_GET, handleDelete);
  server.on("/edit", HTTP_GET, handleEditJson);
  server.on("/savejson", HTTP_POST, handleSaveJson);
  server.on("/mkdir", HTTP_POST, handleMkdir);
  server.on("/rename", HTTP_GET, handleRenameForm);
  server.on("/rename", HTTP_POST, handleRenameAction);
  server.on("/upload", HTTP_POST, handleUploadDone, handleUpload);

  server.begin();

  Serial.print("[WIFI] connecte IP = ");
  Serial.println(gWifiIP);

  return true;
}

void wifiManagerStop() {
  server.stop();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  gWifiActive = false;
  gWifiIP = "";
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
  return gWifiIP;
}