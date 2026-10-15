// Canalele măsurate. Fără dependențe Arduino (testabil pe PC).
#pragma once

#include <cstdint>

enum Ch : uint8_t {
  CH_ECT = 0,  // temperatură lichid răcire, °C
  CH_EOT,      // temperatură ulei, °C
  CH_IAT,      // temperatură aer după intercooler, °C
  CH_EGT,      // temperatură gaze evacuare pre-turbo, °C
  CH_BOOST,    // presiune turbo relativă, bar
  CH_LAMBDA,   // λ
  CH_VBAT,     // tensiune baterie, V
  CH_COUNT
};

// Identificatori scurți: chei JSON, coloane CSV, setări.
inline const char* chKey(uint8_t ch) {
  static const char* const k[CH_COUNT] = {"ect", "eot", "iat", "egt", "boost", "lambda", "vbat"};
  return ch < CH_COUNT ? k[ch] : "?";
}

// Etichete pe display (fără diacritice: fonturile TFT_eSPI au doar ASCII).
inline const char* chLabel(uint8_t ch) {
  static const char* const l[CH_COUNT] = {"APA", "ULEI", "AER", "EGT", "BOOST", "LAMBDA", "BAT"};
  return ch < CH_COUNT ? l[ch] : "?";
}

inline int8_t chFromKey(const char* key) {
  if (!key) return -1;
  for (uint8_t i = 0; i < CH_COUNT; i++) {
    const char* a = chKey(i);
    const char* b = key;
    while (*a && *a == *b) { a++; b++; }
    if (*a == 0 && *b == 0) return static_cast<int8_t>(i);
  }
  return -1;
}

// Coduri de defect raportate pentru fiecare canal.
enum Fault : uint8_t {
  FAULT_OK = 0,
  FAULT_NO_CHIP = 0xFF,    // cipul nu răspunde
  FAULT_RANGE = 0xFE,      // valoare fizic imposibilă (fir rupt, scurtcircuit)
  FAULT_NO_REF = 0xFD,     // referința de 5 V lipsește (senzor ratiometric)
  // Altă valoare: registrul de defect al MAX31865 / MAX31856.
};
