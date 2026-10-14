// Shared data model between the sensor tasks, the display and telemetry.
#pragma once

#include <cstdint>
#include "conversions.h"

enum Channel : uint8_t {
  CH_ECT = 0,  // engine coolant temperature, °C
  CH_EOT,      // engine oil temperature, °C
  CH_IAT,      // intake air temperature after intercooler, °C
  CH_EGT,      // exhaust gas temperature pre-turbo, °C
  CH_BOOST,    // boost, bar relative to atmosphere
  CH_AFR,      // AFR on the gasoline scale (lambda = AFR / 14.7)
  CH_VBAT,     // battery voltage, V
  CH_COUNT
};

// Keys used in the JSON telemetry; the CM5 app depends on these names.
static const char* const kChannelKeys[CH_COUNT] = {"ect", "eot", "iat", "egt", "boost", "afr", "vbat"};

// Fault code meaning: 0 = ok, 0xFF = chip not responding,
// anything else = raw fault register of the MAX31865/MAX31856.
constexpr uint8_t kFaultNoChip = 0xFF;

struct Snapshot {
  uint32_t ms = 0;
  float value[CH_COUNT];           // NaN when unavailable
  conv::Level level[CH_COUNT];
  uint8_t fault[CH_COUNT];
};
