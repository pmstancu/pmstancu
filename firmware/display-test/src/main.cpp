// Test pentru cele 3 display-uri GC9A01 (vezi docs/08-test-display.md).
//
// Etape (butonul de pe GPIO21 trece la următoarea; altfel se schimbă singure):
//   1. Identificare: fiecare ecran are altă culoare și își arată numărul și CS-ul.
//   2. Gauge-uri: desenarea reală a firmware-ului, cu valori simulate.
//   3. Viteză: redesenare forțată a tuturor ecranelor; rezultatele pe serial.
#include <Arduino.h>

#include <cmath>

#include "display.h"
#include "pins.h"
#include "settings.h"
#include "snapshot.h"

namespace {

constexpr uint32_t kStageMs[3] = {8000, 30000, 10000};
constexpr uint32_t kDebounceMs = 30;

uint8_t stage = 0;
uint32_t stageStart = 0;
// Măsurătorile etapei 3, resetate la fiecare intrare în etapă.
uint32_t frames = 0, windowStart = 0, worstUs = 0;
Settings settings = defaultSettings();

// Valori care se plimbă lent prin domeniu, ca să treacă prin avertizare și critic.
float wave(uint32_t ms, float lo, float hi, float periodS, float phase) {
  float x = std::sin(2.0f * static_cast<float>(M_PI) * (ms / 1000.0f / periodS) + phase);
  return lo + (hi - lo) * (x + 1.0f) / 2.0f;
}

Snapshot simulated(uint32_t ms) {
  Snapshot s;
  s.ms = ms;
  s.v[CH_ECT] = wave(ms, 70, 108, 20, 0.0f);
  s.v[CH_EOT] = wave(ms, 80, 125, 25, 1.0f);
  s.v[CH_IAT] = wave(ms, 25, 72, 15, 2.0f);
  s.v[CH_EGT] = wave(ms, 250, 760, 12, 0.5f);
  s.v[CH_BOOST] = wave(ms, -0.1f, 1.75f, 8, 0.0f);
  s.v[CH_LAMBDA] = wave(ms, 1.05f, 2.8f, 10, 3.0f);
  s.v[CH_VBAT] = wave(ms, 11.3f, 14.6f, 30, 0.0f);
  s.lambdaSrc = conv::LambdaSrc::Serial;

  // Între secunda 20 și 25 a etapei, uleiul apare „defect”.
  uint32_t t = (ms - stageStart) / 1000;
  if (t >= 20 && t < 25) {
    s.v[CH_EOT] = NAN;
    s.fault[CH_EOT] = FAULT_RANGE;
  }

  // Alarmele, cu aceleași praguri și histerezis ca firmware-ul principal.
  static alarm::Level lv[AL_COUNT] = {};
  for (uint8_t id = 0; id < AL_COUNT; id++)
    lv[id] = alarm::eval(settings.alarms[id], alarmDir(id), s.v[alarmChannel(id)], lv[id]);
  for (uint8_t id = 0; id < AL_COUNT; id++) {
    uint8_t ch = alarmChannel(id);
    s.level[ch] = alarm::worst(s.level[ch], lv[id]);
  }
  return s;
}

bool buttonPressed() {
  static bool stable = false, lastRaw = false;
  static uint32_t changedAt = 0;
  bool raw = digitalRead(PIN_BUTTON) == LOW;
  uint32_t now = millis();
  if (raw != lastRaw) {
    lastRaw = raw;
    changedAt = now;
  }
  if (now - changedAt < kDebounceMs || raw == stable) return false;
  stable = raw;
  return stable;  // doar la apăsare
}

void startStage(uint8_t s) {
  stage = s;
  stageStart = millis();
  Serial.printf("\n=== Etapa %u ===\n", stage + 1);
  if (stage == 0) {
    static const uint16_t colors[kScreens] = {0xF800, 0x07E0, 0x001F};  // RGB565: roșu, verde, albastru
    static const uint8_t cs[kScreens] = {PIN_CS_DISP1, PIN_CS_DISP2, PIN_CS_DISP3};
    static const char* names[kScreens] = {"ECRAN 1", "ECRAN 2", "ECRAN 3"};
    char line[24];
    for (uint8_t i = 0; i < kScreens; i++) {
      std::snprintf(line, sizeof line, "CS = GPIO%u", cs[i]);
      display::identify(i, colors[i], names[i], line);
      Serial.printf("Ecran %u: %s, CS GPIO%u\n", i + 1, i == 0 ? "ROSU" : (i == 1 ? "VERDE" : "ALBASTRU"), cs[i]);
    }
    Serial.println("Verifica: fiecare ecran are culoarea si numarul lui.");
  } else if (stage == 1) {
    display::forceRedraw();
    Serial.println("Gauge-uri cu valori simulate. Uleiul apare defect intre s 20 si 25.");
  } else {
    frames = 0;
    worstUs = 0;
    windowStart = millis();
    Serial.printf("Test viteza, SPI %lu Hz, buffer %u biti, heap liber %lu B\n",
                  static_cast<unsigned long>(SPI_FREQUENCY), display::bufferDepth(),
                  static_cast<unsigned long>(ESP.getFreeHeap()));
  }
}

}  // namespace

void setup() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  Serial.begin(115200);
  delay(1500);  // timp pentru deschiderea monitorului serial
  Serial.println("LC100 display-test");

  uint32_t heapBefore = ESP.getFreeHeap();
  display::begin();
  Serial.printf("Buffer display: %u biti (0 = alocare esuata), heap %lu -> %lu B\n",
                display::bufferDepth(), static_cast<unsigned long>(heapBefore),
                static_cast<unsigned long>(ESP.getFreeHeap()));
  startStage(0);
}

void loop() {
  uint32_t now = millis();
  if (buttonPressed() || now - stageStart >= kStageMs[stage]) startStage((stage + 1) % 3);

  if (stage == 1) {
    display::update(simulated(now), settings, {true, true, false});
    delay(50);  // aceeași perioadă ca firmware-ul principal
  } else if (stage == 2) {
    display::forceRedraw();
    uint32_t t0 = micros();
    display::update(simulated(now), settings, {true, true, false});
    uint32_t us = micros() - t0;
    if (us > worstUs) worstUs = us;
    frames++;
    if (now - windowStart >= 2000) {
      float fps = frames * 1000.0f / (now - windowStart);
      Serial.printf("3 ecrane: %.1f cadre/s, %.1f ms pe cadru complet (max %.1f ms)\n",
                    static_cast<double>(fps), static_cast<double>(1000.0f / fps),
                    static_cast<double>(worstUs / 1000.0f));
      frames = 0;
      worstUs = 0;
      windowStart = now;
    }
  }
}
