// Logica de afișare fără grafică: ce canal apare unde, scale, formatare.
// Pur, testabil pe PC.
#pragma once

#include <cmath>
#include <cstdint>
#include <cstdio>

#include "channels.h"
#include "conversions.h"
#include "settings.h"
#include "snapshot.h"

struct ScreenPair {
  uint8_t main;
  uint8_t sub;
};

// Așezarea efectivă. Dacă un canal în alarmă CRITICĂ nu apare pe niciun
// ecran, el înlocuiește valoarea secundară de pe ultimul ecran, ca să nu
// treacă neobservat.
inline void effectiveLayout(const Settings& s, const Snapshot& snap, ScreenPair out[kScreens]) {
  for (uint8_t i = 0; i < kScreens; i++) out[i] = {s.screenMain[i], s.screenSub[i]};
  for (uint8_t ch = 0; ch < CH_COUNT; ch++) {
    if (snap.level[ch] != alarm::CRIT) continue;
    bool shown = false;
    for (uint8_t i = 0; i < kScreens; i++) shown |= out[i].main == ch || out[i].sub == ch;
    if (!shown) {
      out[kScreens - 1].sub = ch;
      return;
    }
  }
}

struct GaugeRange {
  float lo;
  float hi;
};

// Scala arcului, în unitatea de afișare.
inline GaugeRange gaugeRange(uint8_t ch, bool psi) {
  switch (ch) {
    case CH_ECT: return {40, 120};
    case CH_EOT: return {40, 150};
    case CH_IAT: return {0, 80};
    case CH_EGT: return {0, 900};
    case CH_BOOST: return psi ? GaugeRange{-5, 30} : GaugeRange{-0.3f, 2.0f};
    case CH_LAMBDA: return {0.8f, 3.0f};
    case CH_VBAT: return {10, 16};
    default: return {0, 1};
  }
}

// Valoarea în unitatea de afișare.
inline float displayValue(uint8_t ch, float v, bool psi) {
  return (ch == CH_BOOST && psi) ? conv::barToPsi(v) : v;
}

inline const char* displayUnit(uint8_t ch, bool psi) {
  switch (ch) {
    case CH_BOOST: return psi ? "psi" : "bar";
    case CH_LAMBDA: return "";
    case CH_VBAT: return "V";
    default: return "C";
  }
}

// Fracția 0..1 din arc.
inline float gaugeFraction(uint8_t ch, float v, bool psi) {
  if (std::isnan(v)) return 0;
  GaugeRange r = gaugeRange(ch, psi);
  float f = (displayValue(ch, v, psi) - r.lo) / (r.hi - r.lo);
  return f < 0 ? 0 : (f > 1 ? 1 : f);
}

// Text pentru valoare. „---” pentru senzor indisponibil; la λ analogic
// la capăt de scală se adaugă „>” sau „<”.
inline void formatValue(uint8_t ch, float v, bool psi, conv::Clip clip, char* out, size_t cap) {
  if (std::isnan(v)) {
    std::snprintf(out, cap, "---");
    return;
  }
  float d = displayValue(ch, v, psi);
  int dec = chDecimals(ch);
  if (ch == CH_BOOST && psi) dec = 1;
  const char* prefix = "";
  if (ch == CH_LAMBDA && clip == conv::Clip::Above) prefix = ">";
  if (ch == CH_LAMBDA && clip == conv::Clip::Below) prefix = "<";
  std::snprintf(out, cap, "%s%.*f", prefix, dec, static_cast<double>(d));
}
