// Evaluarea alarmelor cu histerezis. Pur, testabil pe PC.
#pragma once

#include <cmath>
#include <cstdint>

namespace alarm {

enum Level : uint8_t { OK = 0, WARN = 1, CRIT = 2 };

enum class Dir : uint8_t { High, Low };

struct Spec {
  bool enabled;
  float warn;
  float crit;
  float hyst;
};

// Un nivel se activează la trecerea pragului și se dezactivează doar după ce
// valoarea s-a întors cu `hyst` dincolo de prag. NaN (senzor defect) păstrează
// nivelul anterior; defectul e semnalat separat.
inline Level eval(const Spec& s, Dir dir, float value, Level prev) {
  if (!s.enabled) return OK;
  if (std::isnan(value)) return prev;
  const bool high = dir == Dir::High;
  auto beyond = [&](float thr) { return high ? value >= thr : value <= thr; };
  auto backFrom = [&](float thr) { return high ? value < thr - s.hyst : value > thr + s.hyst; };

  if (beyond(s.crit)) return CRIT;
  if (prev == CRIT && !backFrom(s.crit)) return CRIT;
  if (beyond(s.warn)) return WARN;
  if (prev >= WARN && !backFrom(s.warn)) return WARN;
  return OK;
}

inline Level worst(Level a, Level b) { return a > b ? a : b; }

}  // namespace alarm
