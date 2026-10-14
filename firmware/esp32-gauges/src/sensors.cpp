#include "sensors.h"

#include <Adafruit_ADS1X15.h>
#include <Adafruit_MAX31856.h>
#include <Adafruit_MAX31865.h>
#include <Arduino.h>
#include <Wire.h>

#include <cmath>

#include "config.h"

using conv::AlarmSpec;
using conv::Direction;
using conv::Level;

namespace {

// Software SPI on dedicated pins keeps the sensors off the display's bus.
Adafruit_MAX31865 rtdEct(PIN_CS_ECT, PIN_SENS_MOSI, PIN_SENS_MISO, PIN_SENS_SCK);
Adafruit_MAX31865 rtdEot(PIN_CS_EOT, PIN_SENS_MOSI, PIN_SENS_MISO, PIN_SENS_SCK);
Adafruit_MAX31865 rtdIat(PIN_CS_IAT, PIN_SENS_MOSI, PIN_SENS_MISO, PIN_SENS_SCK);
Adafruit_MAX31856 tcEgt(PIN_CS_EGT, PIN_SENS_MOSI, PIN_SENS_MISO, PIN_SENS_SCK);
Adafruit_ADS1115 ads;

struct RtdSlot {
  Adafruit_MAX31865* dev;
  Channel ch;
};
RtdSlot rtds[] = {{&rtdEct, CH_ECT}, {&rtdEot, CH_EOT}, {&rtdIat, CH_IAT}};

const AlarmSpec kAlarm[CH_COUNT] = {
    /* ECT   */ {Direction::High, ALARM_ECT_WARN, ALARM_ECT_CRIT, HYST_TEMP},
    /* EOT   */ {Direction::High, ALARM_EOT_WARN, ALARM_EOT_CRIT, HYST_TEMP},
    /* IAT   */ {Direction::High, ALARM_IAT_WARN, ALARM_IAT_CRIT, HYST_TEMP},
    /* EGT   */ {Direction::High, ALARM_EGT_WARN, ALARM_EGT_CRIT, HYST_EGT},
    /* BOOST */ {Direction::High, ALARM_BOOST_WARN, ALARM_BOOST_CRIT, HYST_BOOST},
    /* AFR   */ {Direction::High, INFINITY, INFINITY, 0.0f},  // no alarm: diesel AFR depends on load
    /* VBAT  */ {Direction::High, ALARM_VBAT_HIGH_WARN, ALARM_VBAT_HIGH_CRIT, HYST_VBAT},
};
const AlarmSpec kVbatLow = {Direction::Low, ALARM_VBAT_LOW_WARN, ALARM_VBAT_LOW_CRIT, HYST_VBAT};

portMUX_TYPE gLock = portMUX_INITIALIZER_UNLOCKED;
Snapshot gSnap;
conv::DualAlarmState gVbatAlarm;
bool gAdsOk = false;

// Plausibility windows; values outside are treated as sensor faults.
bool plausibleRtd(float c) { return c > -50.0f && c < 250.0f; }
bool plausibleEgt(float c) { return c > -50.0f && c < 1300.0f; }

void publish(Channel ch, float value, uint8_t fault) {
  taskENTER_CRITICAL(&gLock);
  gSnap.value[ch] = value;
  gSnap.fault[ch] = fault;
  if (ch == CH_VBAT) {
    gVbatAlarm.low = conv::evalAlarm(kVbatLow, value, gVbatAlarm.low);
    gVbatAlarm.high = conv::evalAlarm(kAlarm[CH_VBAT], value, gVbatAlarm.high);
    gSnap.level[ch] = gVbatAlarm.worst();
  } else {
    gSnap.level[ch] = conv::evalAlarm(kAlarm[ch], value, gSnap.level[ch]);
  }
  gSnap.ms = millis();
  taskEXIT_CRITICAL(&gLock);
}

float readCurrent(Channel ch) {
  taskENTER_CRITICAL(&gLock);
  float v = gSnap.value[ch];
  taskEXIT_CRITICAL(&gLock);
  return v;
}

// --- Temperatures: MAX31865 conversions block ~75 ms each, so they get their own task.
void temperatureTask(void*) {
  for (;;) {
    for (auto& r : rtds) {
      float c = r.dev->temperature(RTD_NOMINAL, RTD_RREF);
      // FAULT_NONE: just read the status register. The default (AUTO) runs a
      // detection cycle that leaves the bias on, which self-heats the Pt100.
      uint8_t fault = r.dev->readFault(MAX31865_FAULT_NONE);
      if (fault) r.dev->clearFault();
      if (!fault && !plausibleRtd(c)) fault = kFaultNoChip;
      publish(r.ch, fault ? NAN : c, fault);

      // EGT is read between each RTD so it updates ~3x as often.
      float egt = tcEgt.readThermocoupleTemperature();
      uint8_t tcFault = tcEgt.readFault();
      if (!tcFault && !plausibleEgt(egt)) tcFault = kFaultNoChip;
      publish(CH_EGT, tcFault ? NAN : egt, tcFault);
    }
    vTaskDelay(1);
  }
}

// --- Analog: AFR, boost, battery voltage via ADS1115.
float adsVolts(uint8_t ch) { return ads.computeVolts(ads.readADC_SingleEnded(ch)); }

void analogTask(void*) {
  TickType_t last = xTaskGetTickCount();
  for (;;) {
    if (!gAdsOk) {
      gAdsOk = ads.begin(ADS1115_ADDR, &Wire);
      if (gAdsOk) {
        ads.setGain(GAIN_ONE);  // ±4.096 V full scale; inputs never exceed 3.3 V
        ads.setDataRate(RATE_ADS1115_475SPS);
      } else {
        publish(CH_AFR, NAN, kFaultNoChip);
        publish(CH_BOOST, NAN, kFaultNoChip);
        publish(CH_VBAT, NAN, kFaultNoChip);
        vTaskDelay(pdMS_TO_TICKS(1000));
        continue;
      }
    }

    const float supply5v = conv::undoDivider(adsVolts(ADC_CH_5VREF), DIV_5V_SIGNALS);
    const float boostSig = conv::undoDivider(adsVolts(ADC_CH_BOOST), DIV_5V_SIGNALS);
    const float afrSig = conv::undoDivider(adsVolts(ADC_CH_AFR), DIV_5V_SIGNALS);
    const float vbat = conv::undoDivider(adsVolts(ADC_CH_VBAT), DIV_VBAT);

    const float pBar = conv::linearPressure(boostSig, supply5v, BOOST_RATIOMETRIC, BOOST_V_MIN, BOOST_V_MAX,
                                            BOOST_P_MIN_BAR, BOOST_P_MAX_BAR);
    const float boost = conv::boostBar(pBar, BOOST_SENSOR_ABSOLUTE, ATMOSPHERIC_BAR);
    const float afr = conv::afrFromVolts(afrSig, AFR_AT_0V, AFR_AT_5V);

    // A disconnected 0.5-4.5 V sensor reads near 0 V: report it as a fault.
    const bool boostWired = boostSig > 0.2f;
    publish(CH_BOOST, boostWired ? conv::ema(readCurrent(CH_BOOST), boost, EMA_ALPHA_FAST) : NAN,
            boostWired && !std::isnan(boost) ? 0 : kFaultNoChip);
    // 0 V would mean AFR 8.5 (far richer than a diesel ever runs): treat as unpowered/disconnected.
    const bool afrWired = afrSig > 0.05f;
    publish(CH_AFR, afrWired ? conv::ema(readCurrent(CH_AFR), afr, EMA_ALPHA_FAST) : NAN,
            afrWired ? 0 : kFaultNoChip);
    publish(CH_VBAT, conv::ema(readCurrent(CH_VBAT), vbat, EMA_ALPHA_SLOW), 0);

    vTaskDelayUntil(&last, pdMS_TO_TICKS(ANALOG_PERIOD_MS));
  }
}

}  // namespace

void sensorsBegin() {
  for (int i = 0; i < CH_COUNT; ++i) {
    gSnap.value[i] = NAN;
    gSnap.level[i] = Level::Ok;
    gSnap.fault[i] = kFaultNoChip;
  }

  const max31865_numwires_t wires = RTD_WIRES == 2   ? MAX31865_2WIRE
                                    : RTD_WIRES == 4 ? MAX31865_4WIRE
                                                     : MAX31865_3WIRE;
  for (auto& r : rtds) r.dev->begin(wires);

  tcEgt.begin();
  tcEgt.setThermocoupleType(MAX31856_TCTYPE_K);
  tcEgt.setNoiseFilter(MAX31856_NOISE_FILTER_50HZ);
  tcEgt.setConversionMode(MAX31856_CONTINUOUS);

  // With MISO pulled up, a missing/disconnected board reads 0xFF (fault)
  // instead of 0x00, which would decode as a plausible 0 °C.
  pinMode(PIN_SENS_MISO, INPUT_PULLUP);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);

  xTaskCreatePinnedToCore(temperatureTask, "temps", 4096, nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(analogTask, "analog", 4096, nullptr, 2, nullptr, 0);
}

Snapshot sensorsSnapshot() {
  taskENTER_CRITICAL(&gLock);
  Snapshot s = gSnap;
  taskEXIT_CRITICAL(&gLock);
  return s;
}
