// ESP32-S3 gauge controller for Toyota Land Cruiser 100 (1HD-FTE).
// Reads 3x Pt100 (MAX31865), EGT type K (MAX31856), boost/AFR/battery (ADS1115),
// drives a GC9A01 round display and streams JSON lines over USB to the CM5.

#include <Arduino.h>

#include <cmath>

#include "config.h"
#include "display.h"
#include "sensors.h"

namespace {

uint32_t gLastTelemetry = 0, gLastDisplay = 0, gLastHello = 0;
bool gMuted = false;
conv::Level gWorstPrev = conv::Level::Ok;

void sendHello() { Serial.printf("{\"hello\":\"esp32-gauges\",\"fw\":\"%s\"}\n", FW_VERSION); }

// One JSON object per line: {"t":ms,"v":{...},"l":{...},"f":{...}}. NaN -> null.
void sendTelemetry(const Snapshot& s) {
  char buf[512];
  int n = snprintf(buf, sizeof(buf), "{\"t\":%lu,\"v\":{", static_cast<unsigned long>(s.ms));
  for (int i = 0; i < CH_COUNT; ++i) {
    const char* sep = i ? "," : "";
    n += std::isnan(s.value[i]) ? snprintf(buf + n, sizeof(buf) - n, "%s\"%s\":null", sep, kChannelKeys[i])
                                : snprintf(buf + n, sizeof(buf) - n, "%s\"%s\":%.3f", sep, kChannelKeys[i], s.value[i]);
  }
  n += snprintf(buf + n, sizeof(buf) - n, "},\"l\":{");
  for (int i = 0; i < CH_COUNT; ++i)
    n += snprintf(buf + n, sizeof(buf) - n, "%s\"%s\":%u", i ? "," : "", kChannelKeys[i],
                  static_cast<unsigned>(s.level[i]));
  n += snprintf(buf + n, sizeof(buf) - n, "},\"f\":{");
  for (int i = 0; i < CH_COUNT; ++i)
    n += snprintf(buf + n, sizeof(buf) - n, "%s\"%s\":%u", i ? "," : "", kChannelKeys[i], s.fault[i]);
  snprintf(buf + n, sizeof(buf) - n, "}}\n");
  // Don't block the display loop when the CM5 is off or not reading.
  if (Serial.availableForWrite() >= static_cast<int>(strlen(buf))) Serial.print(buf);
}

// Short press: next page. Long press (>1.5 s): mute buzzer until alarms clear.
void handleButton(uint32_t now) {
  static bool wasDown = false;
  static uint32_t downAt = 0;
  static bool longHandled = false;
  const bool down = digitalRead(PIN_BUTTON) == LOW;
  if (down && !wasDown) {
    downAt = now;
    longHandled = false;
  }
  if (down && !longHandled && now - downAt > 1500) {
    gMuted = true;
    longHandled = true;
  }
  if (!down && wasDown && !longHandled && now - downAt > 30) displayNextPage();
  wasDown = down;
}

// Warn: one short beep on entry. Crit: repeating beeps until muted or cleared.
void handleBuzzer(const Snapshot& s, uint32_t now) {
  conv::Level worst = conv::Level::Ok;
  for (int i = 0; i < CH_COUNT; ++i)
    if (s.level[i] > worst) worst = s.level[i];

  if (worst == conv::Level::Ok) gMuted = false;
  bool on = false;
  if (!gMuted) {
    static uint32_t warnBeepUntil = 0;
    if (worst == conv::Level::Warn && gWorstPrev == conv::Level::Ok) warnBeepUntil = now + 150;
    if (worst == conv::Level::Crit) on = (now / 250) % 2;
    else on = now < warnBeepUntil;
  }
  digitalWrite(PIN_BUZZER, on ? HIGH : LOW);
  gWorstPrev = worst;
}

}  // namespace

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  displayBegin();
  sensorsBegin();
  sendHello();
}

void loop() {
  const uint32_t now = millis();
  handleButton(now);

  const Snapshot s = sensorsSnapshot();
  handleBuzzer(s, now);

  if (now - gLastDisplay >= DISPLAY_PERIOD_MS) {
    gLastDisplay = now;
    displayRender(s, now);
  }
  if (now - gLastTelemetry >= TELEMETRY_PERIOD_MS) {
    gLastTelemetry = now;
    sendTelemetry(s);
  }
  if (now - gLastHello >= 10000) {
    gLastHello = now;
    sendHello();
  }
  delay(1);
}
