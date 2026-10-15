#include "settings_store.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>

namespace settings_store {
namespace {
constexpr const char* kNamespace = "lc100";
constexpr const char* kKey = "cfg";
}  // namespace

Settings load() {
  Settings s = defaultSettings();
  Preferences p;
  if (!p.begin(kNamespace, true)) return s;
  String text = p.getString(kKey, "");
  p.end();
  if (text.length() == 0) return s;

  JsonDocument doc;
  if (deserializeJson(doc, text)) return defaultSettings();
  Settings loaded = s;
  if (!fromJson(doc.as<JsonObjectConst>(), loaded) || validate(loaded)) return defaultSettings();
  return loaded;
}

bool save(const Settings& s) {
  JsonDocument doc;
  toJson(s, doc.to<JsonObject>(), true);
  String text;
  serializeJson(doc, text);
  Preferences p;
  if (!p.begin(kNamespace, false)) return false;
  bool ok = p.putString(kKey, text) == text.length();
  p.end();
  return ok;
}

void erase() {
  Preferences p;
  if (!p.begin(kNamespace, false)) return;
  p.remove(kKey);
  p.end();
}

}  // namespace settings_store
