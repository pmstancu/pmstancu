#include "web.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <WebServer.h>
#include <WiFi.h>

#include "logger.h"
#include "logname.h"
#include "settings_store.h"
#include "shared.h"
#include "web_page.h"

namespace web {
namespace {

WebServer server(80);
bool isRunning = false;
bool routesAdded = false;
uint32_t lastRequest = 0;

void touch() { lastRequest = millis(); }

void sendJson(int code, JsonDocument& doc) {
  String out;
  serializeJson(doc, out);
  server.send(code, "application/json", out);
}

void sendError(int code, const char* msg) {
  JsonDocument d;
  d["error"] = msg;
  sendJson(code, d);
}

void handleRoot() {
  touch();
  server.send_P(200, "text/html; charset=utf-8", kIndexHtml);
}

void handleLive() {
  touch();
  JsonDocument d;
  snapshotToJson(shared::snapshot(), d.to<JsonObject>());
  d["sd"] = logger::ok();
  d["log"] = logger::currentFile();
  sendJson(200, d);
}

void handleGetSettings() {
  touch();
  JsonDocument d;
  // Parola Wi-Fi nu se trimite înapoi; câmpul gol în pagină înseamnă „neschimbată”.
  toJson(shared::settings(), d.to<JsonObject>(), false);
  sendJson(200, d);
}

void handlePostSettings() {
  touch();
  JsonDocument in;
  if (deserializeJson(in, server.arg("plain"))) return sendError(400, "JSON invalid");
  Settings s = shared::settings();
  if (!fromJson(in.as<JsonObjectConst>(), s)) return sendError(400, "canal necunoscut");
  if (const char* err = validate(s)) return sendError(400, err);
  if (!settings_store::save(s)) return sendError(500, "salvarea in memorie a esuat");
  shared::setSettings(s);
  JsonDocument d;
  d["ok"] = true;
  sendJson(200, d);
}

void handleResetSettings() {
  touch();
  settings_store::erase();
  Settings s = defaultSettings();
  shared::setSettings(s);
  JsonDocument d;
  d["ok"] = true;
  sendJson(200, d);
}

void handleLogs() {
  touch();
  JsonDocument d;
  JsonArray arr = d["files"].to<JsonArray>();
  File root = SD.open("/");
  if (root) {
    for (File f = root.openNextFile(); f; f = root.openNextFile()) {
      const char* n = f.name();
      if (*n == '/') n++;
      if (isLogName(n)) {
        JsonObject o = arr.add<JsonObject>();
        o["name"] = n;
        o["size"] = static_cast<uint32_t>(f.size());
      }
      f.close();
    }
    root.close();
  }
  d["current"] = logger::currentFile();
  sendJson(200, d);
}

// Numele vine de la client: se acceptă doar „log_NNNN.csv”.
bool logPathFromArg(char* path, size_t cap) {
  String n = server.arg("name");
  if (!isLogName(n.c_str())) return false;
  std::snprintf(path, cap, "/%s", n.c_str());
  return true;
}

void handleLogGet() {
  touch();
  char path[24];
  if (!logPathFromArg(path, sizeof path)) return sendError(400, "nume invalid");
  File f = SD.open(path, FILE_READ);
  if (!f) return sendError(404, "fisier inexistent");
  server.sendHeader("Content-Disposition", String("attachment; filename=") + (path + 1));
  server.streamFile(f, "text/csv");
  f.close();
}

void handleLogDelete() {
  touch();
  char path[24];
  if (!logPathFromArg(path, sizeof path)) return sendError(400, "nume invalid");
  if (std::strcmp(path, logger::currentFile()) == 0) return sendError(409, "fisierul curent nu se poate sterge");
  if (!SD.remove(path)) return sendError(404, "stergerea a esuat");
  JsonDocument d;
  d["ok"] = true;
  sendJson(200, d);
}

void handleAemRaw() {
  touch();
  JsonDocument d;
  d["hex"] = shared::aemRawHex();
  sendJson(200, d);
}

void addRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/live", HTTP_GET, handleLive);
  server.on("/api/settings", HTTP_GET, handleGetSettings);
  server.on("/api/settings", HTTP_POST, handlePostSettings);
  server.on("/api/settings/reset", HTTP_POST, handleResetSettings);
  server.on("/api/logs", HTTP_GET, handleLogs);
  server.on("/api/logs/get", HTTP_GET, handleLogGet);
  server.on("/api/logs/delete", HTTP_POST, handleLogDelete);
  server.on("/api/aem/raw", HTTP_GET, handleAemRaw);
  server.onNotFound([] { sendError(404, "nu exista"); });
  routesAdded = true;
}

}  // namespace

void start() {
  if (isRunning) return;
  Settings s = shared::settings();
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(s.wifiSsid, s.wifiPass)) {
    WiFi.mode(WIFI_OFF);
    return;
  }
  if (!routesAdded) addRoutes();
  server.begin();
  isRunning = true;
  touch();
}

void stop() {
  if (!isRunning) return;
  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  isRunning = false;
}

bool running() { return isRunning; }

void tick() {
  if (!isRunning) return;
  server.handleClient();
  uint16_t idleMin = shared::settings().wifiIdleMin;
  // Nu se oprește cât timp un telefon e conectat, chiar dacă nu face cereri.
  if (idleMin > 0 && WiFi.softAPgetStationNum() == 0 && millis() - lastRequest > idleMin * 60000UL) stop();
}

}  // namespace web
