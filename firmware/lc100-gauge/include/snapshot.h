// Starea curentă a tuturor canalelor + formatare CSV / JSON. Pur, testabil pe PC.
#pragma once

#include <ArduinoJson.h>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "alarms.h"
#include "channels.h"
#include "conversions.h"

struct Snapshot {
  uint32_t ms = 0;                  // millis() la ultima actualizare
  float v[CH_COUNT];                // valori; NaN = indisponibil
  uint8_t fault[CH_COUNT] = {};     // vezi enum Fault
  alarm::Level level[CH_COUNT] = {};
  conv::LambdaSrc lambdaSrc = conv::LambdaSrc::None;
  conv::Clip lambdaClip = conv::Clip::None;
  float aemVolts = NAN;             // tensiunea la ieșirea analogică AEM
  float ref5v = NAN;                // alimentarea senzorului de presiune
  Snapshot() {
    for (float& x : v) x = NAN;
  }
};

inline alarm::Level worstLevel(const Snapshot& s) {
  alarm::Level w = alarm::OK;
  for (uint8_t i = 0; i < CH_COUNT; i++) w = alarm::worst(w, s.level[i]);
  return w;
}

// Zecimale pentru afișare și log.
inline uint8_t chDecimals(uint8_t ch) {
  switch (ch) {
    case CH_BOOST: return 2;
    case CH_LAMBDA: return 2;
    case CH_VBAT: return 1;
    case CH_EGT: return 0;
    default: return 1;
  }
}

// Antetul CSV. Unitățile sunt fixe în log (bar), indiferent de afișaj.
inline int csvHeader(char* out, size_t cap) {
  return std::snprintf(out, cap,
                       "ms,ect_c,eot_c,iat_c,egt_c,boost_bar,lambda,vbat_v,lambda_src,aem_v,ref5v,alarm\n");
}

// O linie CSV. Valorile lipsă rămân goale.
inline int csvRow(const Snapshot& s, char* out, size_t cap) {
  int n = std::snprintf(out, cap, "%lu", static_cast<unsigned long>(s.ms));
  auto add = [&](float x, int dec) {
    if (n < 0 || static_cast<size_t>(n) >= cap) return;
    if (std::isnan(x)) n += std::snprintf(out + n, cap - n, ",");
    else n += std::snprintf(out + n, cap - n, ",%.*f", dec, static_cast<double>(x));
  };
  for (uint8_t i = 0; i < CH_COUNT; i++) add(s.v[i], chDecimals(i));
  if (n > 0 && static_cast<size_t>(n) < cap)
    n += std::snprintf(out + n, cap - n, ",%u", static_cast<unsigned>(s.lambdaSrc));
  add(s.aemVolts, 3);
  add(s.ref5v, 3);
  if (n > 0 && static_cast<size_t>(n) < cap)
    n += std::snprintf(out + n, cap - n, ",%u\n", static_cast<unsigned>(worstLevel(s)));
  return n;
}

inline void snapshotToJson(const Snapshot& s, JsonObject o) {
  o["ms"] = s.ms;
  JsonObject v = o["v"].to<JsonObject>();
  JsonObject l = o["l"].to<JsonObject>();
  JsonObject f = o["f"].to<JsonObject>();
  for (uint8_t i = 0; i < CH_COUNT; i++) {
    if (std::isnan(s.v[i])) v[chKey(i)] = nullptr;
    else v[chKey(i)] = s.v[i];
    l[chKey(i)] = static_cast<uint8_t>(s.level[i]);
    f[chKey(i)] = s.fault[i];
  }
  o["lambda_src"] = static_cast<uint8_t>(s.lambdaSrc);
  o["lambda_clip"] = static_cast<int8_t>(s.lambdaClip);
  if (!std::isnan(s.aemVolts)) o["aem_v"] = s.aemVolts;
  if (!std::isnan(s.ref5v)) o["ref5v"] = s.ref5v;
}
