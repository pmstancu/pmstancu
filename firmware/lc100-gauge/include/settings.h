// Setările modificabile din pagina web: valori implicite, validare,
// serializare JSON. Pur (ArduinoJson e header-only), testabil pe PC.
//
// Valorile marcate [VERIFICĂ] sunt presupuneri sau provin din surse
// neconfirmate. Vezi docs/07-de-verificat.md.
#pragma once

#include <ArduinoJson.h>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "alarms.h"
#include "channels.h"

// Alarmele: câte una pe canal, plus baterie „mare” (supratensiune).
enum AlarmId : uint8_t {
  AL_ECT = 0, AL_EOT, AL_IAT, AL_EGT, AL_BOOST, AL_LAMBDA, AL_VBAT_LOW, AL_VBAT_HIGH, AL_COUNT
};

inline const char* alarmKey(uint8_t id) {
  static const char* const k[AL_COUNT] = {"ect", "eot", "iat", "egt", "boost", "lambda", "vbat_low", "vbat_high"};
  return id < AL_COUNT ? k[id] : "?";
}

// Direcția e fixă pentru fiecare alarmă (nu se configurează).
inline alarm::Dir alarmDir(uint8_t id) {
  return (id == AL_LAMBDA || id == AL_VBAT_LOW) ? alarm::Dir::Low : alarm::Dir::High;
}

inline uint8_t alarmChannel(uint8_t id) {
  return id == AL_VBAT_LOW || id == AL_VBAT_HIGH ? static_cast<uint8_t>(CH_VBAT) : id;
}

constexpr uint8_t kScreens = 3;

enum class AemUnit : uint8_t { Auto = 0, Afr = 1, Lambda = 2 };

struct Settings {
  alarm::Spec alarms[AL_COUNT];

  // Pt100 / MAX31865
  float rtdRref;      // rezistența de referință de pe placă [VERIFICĂ]: 430 Ω Adafruit
  float rtdR0;        // 100 pentru Pt100
  float rtdTrim[3];   // corecție aditivă, °C (ECT, EOT, IAT)

  // Auber, relativ, -0.3…3.3 bar, 0.1…4.85 V la 5 V (fișa Auber, de la utilizator)
  float boostVMin, boostVMax, boostPMin, boostPMax;
  bool boostRatiometric;
  float boostZero;    // corecție de zero, bar (se scade)

  // AEM 30-0310
  float afrAt0V, afrAt5V;  // [VERIFICĂ] manualul AEM (8.5–18.0 vine de la un retailer)
  float afrStoich;         // AEM raportează AFR pe scara benzinei: λ = AFR / 14.7
  bool aemSerial;
  uint32_t aemBaud;        // [VERIFICĂ] 9600 după surse secundare
  AemUnit aemUnit;

  // Divizoare (factor = (Rsus + Rjos) / Rjos), calibrabile cu multimetrul
  float div5v;     // semnale 0-5 V: 10k / 15k
  float divVbat;   // baterie: 100k / 22k

  // Display: canalul principal (arc) și secundar pe fiecare ecran
  uint8_t screenMain[kScreens];
  uint8_t screenSub[kScreens];
  bool boostPsi;

  // Wi-Fi și log
  char wifiSsid[33];
  char wifiPass[64];
  uint16_t wifiIdleMin;    // oprire automată după minute fără cereri (0 = niciodată)
  bool logEnabled;
  uint16_t logPeriodMs;
};

inline Settings defaultSettings() {
  Settings s{};
  // Praguri de pornire, NU limite oficiale Toyota/GTurbo. [VERIFICĂ]
  s.alarms[AL_ECT]       = {true, 100.0f, 105.0f, 2.0f};
  s.alarms[AL_EOT]       = {true, 120.0f, 130.0f, 2.0f};
  s.alarms[AL_IAT]       = {true, 60.0f, 70.0f, 2.0f};
  s.alarms[AL_EGT]       = {true, 650.0f, 720.0f, 15.0f};
  s.alarms[AL_BOOST]     = {true, 1.59f, 1.72f, 0.05f};  // ~23 / ~25 psi; turbina e la 22 psi
  // λ mic = amestec bogat (fum, EGT mare). Dezactivată implicit: pragul potrivit
  // pentru motorul tău trebuie stabilit din log-uri. [VERIFICĂ]
  s.alarms[AL_LAMBDA]    = {false, 1.20f, 1.10f, 0.03f};
  s.alarms[AL_VBAT_LOW]  = {true, 12.0f, 11.5f, 0.2f};
  s.alarms[AL_VBAT_HIGH] = {true, 14.9f, 15.5f, 0.2f};

  s.rtdRref = 430.0f;
  s.rtdR0 = 100.0f;
  for (float& t : s.rtdTrim) t = 0.0f;

  s.boostVMin = 0.1f;
  s.boostVMax = 4.85f;
  s.boostPMin = -0.3f;
  s.boostPMax = 3.3f;
  s.boostRatiometric = true;
  s.boostZero = 0.0f;

  s.afrAt0V = 8.5f;
  s.afrAt5V = 18.0f;
  s.afrStoich = 14.7f;
  s.aemSerial = true;
  s.aemBaud = 9600;
  s.aemUnit = AemUnit::Auto;

  s.div5v = 25.0f / 15.0f;
  s.divVbat = 122.0f / 22.0f;

  s.screenMain[0] = CH_EGT;   s.screenSub[0] = CH_IAT;
  s.screenMain[1] = CH_BOOST; s.screenSub[1] = CH_LAMBDA;
  s.screenMain[2] = CH_ECT;   s.screenSub[2] = CH_EOT;
  s.boostPsi = true;

  std::strncpy(s.wifiSsid, "LC100-Gauge", sizeof(s.wifiSsid) - 1);
  std::strncpy(s.wifiPass, "landcruiser100", sizeof(s.wifiPass) - 1);
  s.wifiIdleMin = 10;
  s.logEnabled = true;
  s.logPeriodMs = 100;
  return s;
}

// ---------------------------------------------------------------------------
// Validare. Întoarce nullptr dacă e OK, altfel un mesaj (pentru pagina web).
// ---------------------------------------------------------------------------
inline bool finiteIn(float v, float lo, float hi) { return std::isfinite(v) && v >= lo && v <= hi; }

inline const char* validate(const Settings& s) {
  for (uint8_t i = 0; i < AL_COUNT; i++) {
    const alarm::Spec& a = s.alarms[i];
    if (!finiteIn(a.warn, -50, 1300) || !finiteIn(a.crit, -50, 1300) || !finiteIn(a.hyst, 0, 100))
      return "prag de alarma in afara domeniului";
    const bool high = alarmDir(i) == alarm::Dir::High;
    if (high ? a.crit < a.warn : a.crit > a.warn)
      return "pragul critic trebuie sa fie dincolo de avertizare";
  }
  if (!finiteIn(s.rtdRref, 300, 500)) return "RREF trebuie sa fie 300-500 ohm";
  if (!finiteIn(s.rtdR0, 50, 1100)) return "R0 invalid";
  for (float t : s.rtdTrim)
    if (!finiteIn(t, -10, 10)) return "corectia Pt100 trebuie sa fie intre -10 si 10 C";
  if (!finiteIn(s.boostVMin, 0, 5) || !finiteIn(s.boostVMax, 0, 5) || s.boostVMax - s.boostVMin < 1.0f)
    return "tensiunile senzorului de presiune sunt invalide";
  if (!finiteIn(s.boostPMin, -1.1f, 10) || !finiteIn(s.boostPMax, -1, 20) || s.boostPMax <= s.boostPMin)
    return "domeniul senzorului de presiune este invalid";
  if (!finiteIn(s.boostZero, -0.5f, 0.5f)) return "corectia de zero boost trebuie sa fie +/-0.5 bar";
  if (!finiteIn(s.afrAt0V, 5, 30) || !finiteIn(s.afrAt5V, 5, 100) || s.afrAt5V <= s.afrAt0V)
    return "scalarea AEM este invalida";
  if (!finiteIn(s.afrStoich, 5, 20)) return "AFR stoichiometric invalid";
  if (s.aemBaud < 1200 || s.aemBaud > 115200) return "baud AEM invalid";
  if (static_cast<uint8_t>(s.aemUnit) > 2) return "unitate AEM invalida";
  if (!finiteIn(s.div5v, 1, 3)) return "divizorul 5 V trebuie sa fie 1-3";
  if (!finiteIn(s.divVbat, 3, 10)) return "divizorul bateriei trebuie sa fie 3-10";
  for (uint8_t i = 0; i < kScreens; i++)
    if (s.screenMain[i] >= CH_COUNT || s.screenSub[i] >= CH_COUNT) return "canal de display invalid";
  size_t ssidLen = strnlen(s.wifiSsid, sizeof(s.wifiSsid));
  size_t passLen = strnlen(s.wifiPass, sizeof(s.wifiPass));
  if (ssidLen < 1 || ssidLen > 32) return "SSID trebuie sa aiba 1-32 caractere";
  if (passLen < 8 || passLen > 63) return "parola Wi-Fi trebuie sa aiba 8-63 caractere";
  if (s.wifiIdleMin > 240) return "oprirea Wi-Fi trebuie sa fie 0-240 minute";
  if (s.logPeriodMs < 50 || s.logPeriodMs > 10000) return "perioada de log trebuie sa fie 50-10000 ms";
  return nullptr;
}

// ---------------------------------------------------------------------------
// JSON
// ---------------------------------------------------------------------------
inline void toJson(const Settings& s, JsonObject o, bool includePassword) {
  JsonObject al = o["alarms"].to<JsonObject>();
  for (uint8_t i = 0; i < AL_COUNT; i++) {
    JsonObject a = al[alarmKey(i)].to<JsonObject>();
    a["on"] = s.alarms[i].enabled;
    a["warn"] = s.alarms[i].warn;
    a["crit"] = s.alarms[i].crit;
    a["hyst"] = s.alarms[i].hyst;
  }
  o["rtd_rref"] = s.rtdRref;
  o["rtd_r0"] = s.rtdR0;
  JsonArray tr = o["rtd_trim"].to<JsonArray>();
  for (float t : s.rtdTrim) tr.add(t);
  o["boost_vmin"] = s.boostVMin;
  o["boost_vmax"] = s.boostVMax;
  o["boost_pmin"] = s.boostPMin;
  o["boost_pmax"] = s.boostPMax;
  o["boost_ratio"] = s.boostRatiometric;
  o["boost_zero"] = s.boostZero;
  o["afr_0v"] = s.afrAt0V;
  o["afr_5v"] = s.afrAt5V;
  o["afr_stoich"] = s.afrStoich;
  o["aem_serial"] = s.aemSerial;
  o["aem_baud"] = s.aemBaud;
  o["aem_unit"] = static_cast<uint8_t>(s.aemUnit);
  o["div_5v"] = s.div5v;
  o["div_vbat"] = s.divVbat;
  JsonArray sc = o["screens"].to<JsonArray>();
  for (uint8_t i = 0; i < kScreens; i++) {
    JsonObject e = sc.add<JsonObject>();
    e["main"] = chKey(s.screenMain[i]);
    e["sub"] = chKey(s.screenSub[i]);
  }
  o["boost_psi"] = s.boostPsi;
  o["wifi_ssid"] = s.wifiSsid;
  if (includePassword) o["wifi_pass"] = s.wifiPass;
  o["wifi_idle_min"] = s.wifiIdleMin;
  o["log_on"] = s.logEnabled;
  o["log_ms"] = s.logPeriodMs;
}

namespace settings_json {
template <typename T>
inline void readIf(JsonObjectConst o, const char* key, T& out) {
  JsonVariantConst v = o[key];
  if (!v.isNull() && v.is<T>()) out = v.as<T>();
}
// Numerele JSON pot veni ca întregi; acceptăm orice număr pentru câmpurile float.
inline void readFloat(JsonObjectConst o, const char* key, float& out) {
  JsonVariantConst v = o[key];
  if (!v.isNull() && (v.is<float>() || v.is<long>())) out = v.as<float>();
}
inline void readStr(JsonObjectConst o, const char* key, char* out, size_t cap) {
  JsonVariantConst v = o[key];
  if (v.is<const char*>()) {
    std::strncpy(out, v.as<const char*>(), cap - 1);
    out[cap - 1] = 0;
  }
}
}  // namespace settings_json

// Aplică peste `s` doar câmpurile prezente (actualizare parțială).
// Întoarce false dacă un nume de canal e necunoscut.
inline bool fromJson(JsonObjectConst o, Settings& s) {
  using namespace settings_json;
  JsonObjectConst al = o["alarms"];
  if (!al.isNull()) {
    for (uint8_t i = 0; i < AL_COUNT; i++) {
      JsonObjectConst a = al[alarmKey(i)];
      if (a.isNull()) continue;
      readIf(a, "on", s.alarms[i].enabled);
      readFloat(a, "warn", s.alarms[i].warn);
      readFloat(a, "crit", s.alarms[i].crit);
      readFloat(a, "hyst", s.alarms[i].hyst);
    }
  }
  readFloat(o, "rtd_rref", s.rtdRref);
  readFloat(o, "rtd_r0", s.rtdR0);
  JsonArrayConst tr = o["rtd_trim"];
  if (!tr.isNull()) {
    for (size_t i = 0; i < 3 && i < tr.size(); i++)
      if (tr[i].is<float>() || tr[i].is<long>()) s.rtdTrim[i] = tr[i].as<float>();
  }
  readFloat(o, "boost_vmin", s.boostVMin);
  readFloat(o, "boost_vmax", s.boostVMax);
  readFloat(o, "boost_pmin", s.boostPMin);
  readFloat(o, "boost_pmax", s.boostPMax);
  readIf(o, "boost_ratio", s.boostRatiometric);
  readFloat(o, "boost_zero", s.boostZero);
  readFloat(o, "afr_0v", s.afrAt0V);
  readFloat(o, "afr_5v", s.afrAt5V);
  readFloat(o, "afr_stoich", s.afrStoich);
  readIf(o, "aem_serial", s.aemSerial);
  readIf(o, "aem_baud", s.aemBaud);
  if (o["aem_unit"].is<uint8_t>()) s.aemUnit = static_cast<AemUnit>(o["aem_unit"].as<uint8_t>());
  readFloat(o, "div_5v", s.div5v);
  readFloat(o, "div_vbat", s.divVbat);
  JsonArrayConst sc = o["screens"];
  if (!sc.isNull()) {
    for (size_t i = 0; i < kScreens && i < sc.size(); i++) {
      int8_t m = chFromKey(sc[i]["main"] | "");
      int8_t b = chFromKey(sc[i]["sub"] | "");
      if (m < 0 || b < 0) return false;
      s.screenMain[i] = static_cast<uint8_t>(m);
      s.screenSub[i] = static_cast<uint8_t>(b);
    }
  }
  readIf(o, "boost_psi", s.boostPsi);
  readStr(o, "wifi_ssid", s.wifiSsid, sizeof(s.wifiSsid));
  readStr(o, "wifi_pass", s.wifiPass, sizeof(s.wifiPass));
  readIf(o, "wifi_idle_min", s.wifiIdleMin);
  readIf(o, "log_on", s.logEnabled);
  readIf(o, "log_ms", s.logPeriodMs);
  return true;
}
