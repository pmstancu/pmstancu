#include "sensors.h"

#include <Adafruit_ADS1X15.h>
#include <Adafruit_MAX31856.h>
#include <Adafruit_MAX31865.h>
#include <Arduino.h>
#include <Wire.h>

#include "aem_parser.h"
#include "alarms.h"
#include "conversions.h"
#include "pins.h"
#include "shared.h"

namespace sensors {
namespace {

constexpr uint32_t kFastPeriodMs = 25;
constexpr uint32_t kTempPeriodMs = 250;
constexpr uint32_t kAemMaxAgeMs = 1000;
constexpr float kAlphaFast = 0.35f;
constexpr float kAlphaSlow = 0.15f;
constexpr float kAlphaTemp = 0.5f;
// Bitii de defect MAX31856 care invalidează citirea: CJRANGE, TCRANGE, OVUV, OPEN.
// TCHIGH/TCLOW (praguri programabile) sunt ignorați.
constexpr uint8_t kEgtFaultMask = 0xC3;

Adafruit_MAX31865 rtd[3] = {
    Adafruit_MAX31865(PIN_CS_ECT, PIN_SENS_MOSI, PIN_SENS_MISO, PIN_SENS_SCK),
    Adafruit_MAX31865(PIN_CS_EOT, PIN_SENS_MOSI, PIN_SENS_MISO, PIN_SENS_SCK),
    Adafruit_MAX31865(PIN_CS_IAT, PIN_SENS_MOSI, PIN_SENS_MISO, PIN_SENS_SCK),
};
Adafruit_MAX31856 egt(PIN_CS_EGT, PIN_SENS_MOSI, PIN_SENS_MISO, PIN_SENS_SCK);
#if ADC_CHIP_ADS1015
Adafruit_ADS1015 ads;
constexpr uint16_t kAdcRate = RATE_ADS1015_490SPS;
#else
Adafruit_ADS1115 ads;
constexpr uint16_t kAdcRate = RATE_ADS1115_475SPS;
#endif
bool adsOk = false;

// Rezultatele task-ului lent, citite de task-ul rapid.
portMUX_TYPE tempMux = portMUX_INITIALIZER_UNLOCKED;
float tempVal[4] = {NAN, NAN, NAN, NAN};  // ECT, EOT, IAT, EGT
uint8_t tempFault[4] = {FAULT_NO_CHIP, FAULT_NO_CHIP, FAULT_NO_CHIP, FAULT_NO_CHIP};

void publishTemp(int i, float v, uint8_t fault) {
  portENTER_CRITICAL(&tempMux);
  tempVal[i] = v;
  tempFault[i] = fault;
  portEXIT_CRITICAL(&tempMux);
}

void tempTask(void*) {
  for (auto& r : rtd) r.begin(MAX31865_3WIRE);
  bool egtOk = egt.begin();
  if (egtOk) {
    egt.setThermocoupleType(MAX31856_TCTYPE_K);
    egt.setNoiseFilter(MAX31856_NOISE_FILTER_50HZ);
    egt.setConversionMode(MAX31856_CONTINUOUS);
  }
  float filt[4] = {NAN, NAN, NAN, NAN};

  for (;;) {
    uint32_t start = millis();
    Settings s = shared::settings();

    for (int i = 0; i < 3; i++) {
      uint16_t raw = rtd[i].readRTD();  // blochează ~75 ms (bias + conversie)
      uint8_t fault = rtd[i].readFault();
      float t = NAN;
      if (fault) {
        rtd[i].clearFault();
      } else {
        t = conv::rtdTemperature(conv::rtdResistance(raw, s.rtdRref), s.rtdR0) + s.rtdTrim[i];
        // Pt100 pe motor: în afara -40…250 °C e fir rupt, scurtcircuit sau cip absent.
        if (!(t > -40.0f && t < 250.0f)) {
          t = NAN;
          fault = FAULT_RANGE;
        }
      }
      filt[i] = std::isnan(t) ? NAN : conv::ema(filt[i], t, kAlphaTemp);
      publishTemp(i, filt[i], fault);
    }

    if (egtOk) {
      float t = egt.readThermocoupleTemperature();
      uint8_t fault = egt.readFault() & kEgtFaultMask;
      if (fault || std::isnan(t) || t < -50.0f || t > 1300.0f) {
        if (!fault) fault = FAULT_RANGE;
        t = NAN;
      }
      filt[3] = std::isnan(t) ? NAN : conv::ema(filt[3], t, kAlphaTemp);
      publishTemp(3, filt[3], fault);
    }

    uint32_t el = millis() - start;
    vTaskDelay(pdMS_TO_TICKS(el < kTempPeriodMs ? kTempPeriodMs - el : 1));
  }
}

float readVolts(uint8_t ch) {
  if (!adsOk) return NAN;
  return ads.computeVolts(ads.readADC_SingleEnded(ch));
}

void fastTask(void*) {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  adsOk = ads.begin(ADC_ADDR, &Wire);
  if (adsOk) {
    ads.setGain(GAIN_ONE);  // ±4.096 V; intrările sunt sub 3.3 V după divizoare
    ads.setDataRate(kAdcRate);
  }

  Settings s = shared::settings();
  uint32_t seenVersion = shared::settingsVersion();
  bool serialOn = false;
  uint32_t serialBaud = 0;
  AemLineParser parser;
  float serialLambda = NAN;
  uint32_t serialAt = 0;

  float boost = NAN, vbat = NAN, aemV = NAN;
  alarm::Level lv[AL_COUNT] = {};

  for (;;) {
    uint32_t start = millis();

    uint32_t ver = shared::settingsVersion();
    if (ver != seenVersion) {
      s = shared::settings();
      seenVersion = ver;
    }
    // (Re)pornește UART-ul AEM dacă s-a schimbat configurația.
    if (s.aemSerial && (!serialOn || serialBaud != s.aemBaud)) {
      if (serialOn) Serial1.end();
      Serial1.begin(s.aemBaud, SERIAL_8N1, PIN_AEM_RX, -1);
      serialOn = true;
      serialBaud = s.aemBaud;
    } else if (!s.aemSerial && serialOn) {
      Serial1.end();
      serialOn = false;
    }

    // --- serial AEM ---
    if (serialOn) {
      while (Serial1.available() > 0) {
        int c = Serial1.read();
        if (c < 0) break;
        shared::aemRawPush(static_cast<uint8_t>(c));
        float l;
        if (parser.feed(static_cast<char>(c), s.aemUnit, s.afrStoich, &l)) {
          serialLambda = l;
          serialAt = millis();
        }
      }
    }

    // --- analogice ---
    float vAem = conv::undoDivider(readVolts(ADC_CH_AEM), s.div5v);
    float vBoost = conv::undoDivider(readVolts(ADC_CH_BOOST), s.div5v);
    float vBat = conv::undoDivider(readVolts(ADC_CH_VBAT), s.divVbat);
    float vRef = conv::undoDivider(readVolts(ADC_CH_5VREF), s.div5v);

    Snapshot snap;
    snap.ms = millis();
    snap.ref5v = vRef;

    float p = conv::linearPressure(vBoost, vRef, s.boostRatiometric, s.boostVMin, s.boostVMax,
                                   s.boostPMin, s.boostPMax);
    // Sub 0.05 V nu există semnal (senzorul dă minim 0.1 V): fir rupt.
    if (!std::isnan(p) && vBoost < 0.05f) p = NAN;
    if (std::isnan(p)) {
      boost = NAN;
      snap.fault[CH_BOOST] = !adsOk ? FAULT_NO_CHIP
                             : (s.boostRatiometric && !(vRef > 4.5f && vRef < 5.5f)) ? FAULT_NO_REF
                                                                                     : FAULT_RANGE;
    } else {
      boost = conv::ema(boost, p - s.boostZero, kAlphaFast);
    }
    snap.v[CH_BOOST] = boost;

    if (!adsOk) {
      vbat = NAN;
      snap.fault[CH_VBAT] = FAULT_NO_CHIP;
    } else {
      vbat = conv::ema(vbat, vBat, kAlphaSlow);
    }
    snap.v[CH_VBAT] = vbat;

    aemV = std::isnan(vAem) ? NAN : conv::ema(aemV, vAem, kAlphaFast);
    snap.aemVolts = aemV;
    float analogLambda = std::isnan(aemV)
                             ? NAN
                             : conv::lambdaFromAfr(conv::afrFromVolts(aemV, s.afrAt0V, s.afrAt5V), s.afrStoich);
    uint32_t age = serialOn ? millis() - serialAt : UINT32_MAX;
    conv::LambdaChoice lc = conv::chooseLambda(serialOn ? serialLambda : NAN, age, kAemMaxAgeMs,
                                               analogLambda, aemV);
    snap.v[CH_LAMBDA] = lc.lambda;
    snap.lambdaSrc = lc.src;
    snap.lambdaClip = lc.clip;
    if (lc.src == conv::LambdaSrc::None) snap.fault[CH_LAMBDA] = adsOk ? FAULT_RANGE : FAULT_NO_CHIP;

    // --- temperaturi (de la task-ul lent) ---
    portENTER_CRITICAL(&tempMux);
    for (int i = 0; i < 4; i++) {
      snap.v[CH_ECT + i] = tempVal[i];
      snap.fault[CH_ECT + i] = tempFault[i];
    }
    portEXIT_CRITICAL(&tempMux);

    // --- alarme ---
    for (uint8_t id = 0; id < AL_COUNT; id++)
      lv[id] = alarm::eval(s.alarms[id], alarmDir(id), snap.v[alarmChannel(id)], lv[id]);
    for (uint8_t ch = 0; ch < CH_COUNT; ch++) snap.level[ch] = alarm::OK;
    for (uint8_t id = 0; id < AL_COUNT; id++) {
      uint8_t ch = alarmChannel(id);
      snap.level[ch] = alarm::worst(snap.level[ch], lv[id]);
    }

    shared::setSnapshot(snap);

    uint32_t el = millis() - start;
    vTaskDelay(pdMS_TO_TICKS(el < kFastPeriodMs ? kFastPeriodMs - el : 1));
  }
}

}  // namespace

void begin() {
  xTaskCreatePinnedToCore(tempTask, "temps", 4096, nullptr, 2, nullptr, 0);
  xTaskCreatePinnedToCore(fastTask, "fast", 6144, nullptr, 3, nullptr, 0);
}

}  // namespace sensors
