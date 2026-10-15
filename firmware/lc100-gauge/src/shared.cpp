#include "shared.h"

namespace shared {
namespace {

SemaphoreHandle_t gMutex = nullptr;
Snapshot gSnap;
Settings gSettings = defaultSettings();
uint32_t gSettingsVersion = 1;
RawLog<96> gAemRaw;

struct Lock {
  Lock() { xSemaphoreTake(gMutex, portMAX_DELAY); }
  ~Lock() { xSemaphoreGive(gMutex); }
};

}  // namespace

void init() { gMutex = xSemaphoreCreateMutex(); }

Snapshot snapshot() {
  Lock l;
  return gSnap;
}

void setSnapshot(const Snapshot& s) {
  Lock l;
  gSnap = s;
}

Settings settings() {
  Lock l;
  return gSettings;
}

uint32_t settingsVersion() {
  Lock l;
  return gSettingsVersion;
}

void setSettings(const Settings& s) {
  Lock l;
  gSettings = s;
  gSettingsVersion++;
}

void aemRawPush(uint8_t b) {
  Lock l;
  gAemRaw.push(b);
}

String aemRawHex() {
  Lock l;
  String out;
  out.reserve(gAemRaw.size() * 3);
  static const char* hex = "0123456789ABCDEF";
  for (size_t i = 0; i < gAemRaw.size(); i++) {
    uint8_t b = gAemRaw.at(i);
    out += hex[b >> 4];
    out += hex[b & 0xF];
    out += ' ';
  }
  return out;
}

}  // namespace shared
