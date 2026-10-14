// Pure conversion and alarm logic. No Arduino dependencies so it can be
// unit-tested on the host (pio test -e native).
#pragma once

#include <cmath>
#include <cstdint>

namespace conv {

constexpr float kPsiPerBar = 14.5038f;

inline float barToPsi(float bar) { return bar * kPsiPerBar; }

// Undo a resistive divider: voltage at the sensor = voltage at ADC * factor.
inline float undoDivider(float adcVolts, float factor) { return adcVolts * factor; }

// Linear sensor: maps [vMin, vMax] -> [pMin, pMax]. When ratiometric, the
// signal is first rescaled to what it would read at a nominal 5.00 V supply.
inline float linearPressure(float signalV, float supplyV, bool ratiometric,
                            float vMin, float vMax, float pMin, float pMax) {
  float v = signalV;
  if (ratiometric) {
    if (!(supplyV > 4.0f && supplyV < 6.0f)) return NAN;  // reference missing/implausible
    v = signalV * (5.0f / supplyV);
  }
  return pMin + (v - vMin) * (pMax - pMin) / (vMax - vMin);
}

// Boost relative to atmosphere, in bar.
inline float boostBar(float pressureBar, bool sensorAbsolute, float atmosphericBar) {
  return sensorAbsolute ? pressureBar - atmosphericBar : pressureBar;
}

// Wideband controller analog output, linear 0-5 V.
inline float afrFromVolts(float v, float afrAt0V, float afrAt5V) {
  return afrAt0V + v * (afrAt5V - afrAt0V) / 5.0f;
}

inline float lambdaFromAfr(float afr, float stoich) { return afr / stoich; }

// Exponential moving average; NaN input leaves the state unchanged,
// NaN state is replaced by the first valid sample.
inline float ema(float state, float sample, float alpha) {
  if (std::isnan(sample)) return state;
  if (std::isnan(state)) return sample;
  return state + alpha * (sample - state);
}

enum class Level : uint8_t { Ok = 0, Warn = 1, Crit = 2 };

enum class Direction : uint8_t { High, Low };

struct AlarmSpec {
  Direction dir;
  float warn;
  float crit;
  float hyst;
};

// Alarm evaluation with hysteresis: a level is entered when the threshold is
// crossed and only left once the value has moved `hyst` back past it.
// A NaN value (sensor fault) keeps the previous level.
inline Level evalAlarm(const AlarmSpec& s, float value, Level prev) {
  if (std::isnan(value)) return prev;
  const bool high = s.dir == Direction::High;
  auto beyond = [&](float thr) { return high ? value >= thr : value <= thr; };
  auto backFrom = [&](float thr) { return high ? value < thr - s.hyst : value > thr + s.hyst; };

  if (beyond(s.crit)) return Level::Crit;
  if (prev == Level::Crit && !backFrom(s.crit)) return Level::Crit;
  if (beyond(s.warn)) return Level::Warn;
  if (prev >= Level::Warn && !backFrom(s.warn)) return Level::Warn;
  return Level::Ok;
}

// Two-sided alarm (e.g. battery voltage): worst of the low and high sides.
struct DualAlarmState {
  Level low = Level::Ok;
  Level high = Level::Ok;
  Level worst() const { return low > high ? low : high; }
};

}  // namespace conv
