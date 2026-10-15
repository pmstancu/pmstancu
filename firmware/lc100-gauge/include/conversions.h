// Conversii pure (fără Arduino), testate cu `pio test -e native`.
#pragma once

#include <cmath>
#include <cstdint>

namespace conv {

constexpr float kPsiPerBar = 14.5038f;

inline float barToPsi(float bar) { return bar * kPsiPerBar; }

// Anulează un divizor rezistiv: tensiunea la sursă = tensiunea la ADC * factor.
inline float undoDivider(float adcVolts, float factor) { return adcVolts * factor; }

// Pt100 cu MAX31865: raport ADC (15 biți) -> rezistență.
inline float rtdResistance(uint16_t raw15, float rref) { return raw15 * rref / 32768.0f; }

// Callendar–Van Dusen (IEC 60751), aceeași metodă ca în biblioteca Adafruit:
// soluție exactă pentru T >= 0 °C, polinom de aproximare sub 0 °C.
inline float rtdTemperature(float resistance, float r0) {
  constexpr float A = 3.9083e-3f;
  constexpr float B = -5.775e-7f;
  float z1 = -A;
  float z2 = A * A - 4.0f * B;
  float z3 = 4.0f * B / r0;
  float z4 = 2.0f * B;
  float t = (std::sqrt(z2 + z3 * resistance) + z1) / z4;
  if (t >= 0.0f) return t;

  float rpoly = resistance / r0 * 100.0f;  // normalizat la Pt100
  t = -242.02f;
  t += 2.2228f * rpoly;
  rpoly *= resistance / r0 * 100.0f;
  t += 2.5859e-3f * rpoly;
  rpoly *= resistance / r0 * 100.0f;
  t -= 4.8260e-6f * rpoly;
  rpoly *= resistance / r0 * 100.0f;
  t -= 2.8183e-8f * rpoly;
  rpoly *= resistance / r0 * 100.0f;
  t += 1.5243e-10f * rpoly;
  return t;
}

// Senzor liniar de presiune. Dacă e ratiometric, semnalul se readuce mai întâi
// la ce ar fi la o alimentare nominală de 5.00 V. Întoarce NaN dacă
// referința lipsește sau e neplauzibilă.
inline float linearPressure(float signalV, float supplyV, bool ratiometric,
                            float vMin, float vMax, float pMin, float pMax) {
  float v = signalV;
  if (ratiometric) {
    if (!(supplyV > 4.5f && supplyV < 5.5f)) return NAN;
    v = signalV * (5.0f / supplyV);
  }
  return pMin + (v - vMin) * (pMax - pMin) / (vMax - vMin);
}

// Ieșirea analogică a controlerului wideband, liniară 0–5 V.
inline float afrFromVolts(float v, float afrAt0V, float afrAt5V) {
  return afrAt0V + v * (afrAt5V - afrAt0V) / 5.0f;
}

inline float lambdaFromAfr(float afr, float stoich) { return afr / stoich; }

// Medie exponențială. Un eșantion NaN lasă starea neschimbată; o stare NaN
// e înlocuită de primul eșantion valid.
inline float ema(float state, float sample, float alpha) {
  if (std::isnan(sample)) return state;
  if (std::isnan(state)) return sample;
  return state + alpha * (sample - state);
}

// Sursa valorii λ afișate.
enum class LambdaSrc : uint8_t { None = 0, Serial = 1, Analog = 2 };

// Capătul de scală al ieșirii analogice: valoarea reală poate fi dincolo.
enum class Clip : int8_t { Below = -1, None = 0, Above = 1 };

struct LambdaChoice {
  float lambda;
  LambdaSrc src;
  Clip clip;
};

// Serialul AEM e prioritar cât timp e proaspăt; altfel ieșirea analogică.
// `analogV` este tensiunea la ieșirea AEM (după anularea divizorului).
inline LambdaChoice chooseLambda(float serialLambda, uint32_t serialAgeMs, uint32_t maxAgeMs,
                                 float analogLambda, float analogV) {
  if (!std::isnan(serialLambda) && serialAgeMs <= maxAgeMs)
    return {serialLambda, LambdaSrc::Serial, Clip::None};
  if (std::isnan(analogLambda)) return {NAN, LambdaSrc::None, Clip::None};
  Clip c = Clip::None;
  if (analogV >= 4.95f) c = Clip::Above;
  else if (analogV <= 0.05f) c = Clip::Below;
  return {analogLambda, LambdaSrc::Analog, c};
}

}  // namespace conv
