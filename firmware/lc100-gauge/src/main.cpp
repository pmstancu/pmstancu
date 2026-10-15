// LC100 Gauge: Toyota Land Cruiser 100, 1HD-FTE.
// ESP32-S3 + 3 × GC9A01 + 3 × Pt100 + EGT tip K + Auber + AEM 30-0310.
//
// Core 0: task-urile de senzori (sensors.cpp).
// Core 1 (loop): display-uri, buton, buzzer, log pe SD, server web.
#include <Arduino.h>

#include "display.h"
#include "logger.h"
#include "pins.h"
#include "sensors.h"
#include "settings_store.h"
#include "shared.h"
#include "web.h"

#define FW_VERSION "2.0.0"

namespace {

constexpr uint32_t kDisplayPeriodMs = 50;
constexpr uint32_t kDebounceMs = 30;
constexpr uint32_t kLongPressMs = 3000;
constexpr uint32_t kBeepHalfPeriodMs = 300;

bool muted = false;

// Buton: apăsare scurtă = oprește buzzerul (până dispar alarmele critice),
// apăsare lungă (3 s) = pornește/oprește Wi-Fi.
void handleButton(const Snapshot& snap) {
  static bool stable = false, lastRaw = false, longDone = false;
  static uint32_t changedAt = 0, pressedAt = 0;
  bool raw = digitalRead(PIN_BUTTON) == LOW;
  uint32_t now = millis();
  if (raw != lastRaw) {
    lastRaw = raw;
    changedAt = now;
  }
  if (now - changedAt < kDebounceMs || raw == stable) {
    // Apăsare lungă detectată cât butonul e încă ținut.
    if (stable && !longDone && now - pressedAt >= kLongPressMs) {
      longDone = true;
      if (web::running()) {
        web::stop();
        display::message("WiFi OPRIT", nullptr);
      } else {
        web::start();
        Settings s = shared::settings();
        display::message(web::running() ? "WiFi PORNIT" : "WiFi EROARE", web::running() ? s.wifiSsid : nullptr);
      }
      delay(1500);
    }
    return;
  }
  stable = raw;
  if (stable) {
    pressedAt = now;
    longDone = false;
  } else if (!longDone && worstLevel(snap) == alarm::CRIT) {
    muted = true;
  }
}

void handleBuzzer(const Snapshot& snap) {
  bool crit = worstLevel(snap) == alarm::CRIT;
  if (!crit) muted = false;
  bool on = crit && !muted && (millis() / kBeepHalfPeriodMs) % 2 == 0;
  digitalWrite(PIN_BUZZER, on ? HIGH : LOW);
}

}  // namespace

void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  Serial.begin(115200);

  shared::init();
  shared::setSettings(settings_store::load());

  display::begin();
  display::message("LC100", "v" FW_VERSION);

  logger::begin();
  sensors::begin();
  delay(800);  // primele citiri de temperatură (~250 ms) înainte de primul cadru
}

void loop() {
  static uint32_t lastDisplay = 0;
  Snapshot snap = shared::snapshot();
  Settings s = shared::settings();

  handleButton(snap);
  handleBuzzer(snap);

  if (millis() - lastDisplay >= kDisplayPeriodMs) {
    lastDisplay = millis();
    display::update(snap, s, {web::running(), logger::ok() || !s.logEnabled, muted});
  }

  logger::tick(snap, s);
  web::tick();
  delay(1);
}
