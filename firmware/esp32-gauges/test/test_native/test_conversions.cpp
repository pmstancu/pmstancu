#include <unity.h>

#include <cmath>

#include "conversions.h"

using namespace conv;

void setUp() {}
void tearDown() {}

void test_linear_pressure_endpoints() {
  // 0.5-4.5 V over 0-100 psi (6.895 bar), not ratiometric
  TEST_ASSERT_FLOAT_WITHIN(1e-4, 0.0f, linearPressure(0.5f, 5.0f, false, 0.5f, 4.5f, 0.0f, 6.895f));
  TEST_ASSERT_FLOAT_WITHIN(1e-4, 6.895f, linearPressure(4.5f, 5.0f, false, 0.5f, 4.5f, 0.0f, 6.895f));
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 3.4475f, linearPressure(2.5f, 5.0f, false, 0.5f, 4.5f, 0.0f, 6.895f));
}

void test_linear_pressure_ratiometric() {
  // Supply sagged to 4.8 V: a 2.4 V signal is mid-scale, same as 2.5 V at 5.0 V.
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 3.4475f, linearPressure(2.4f, 4.8f, true, 0.5f, 4.5f, 0.0f, 6.895f));
  // Missing reference -> NaN instead of a wrong number
  TEST_ASSERT_TRUE(std::isnan(linearPressure(2.4f, 0.0f, true, 0.5f, 4.5f, 0.0f, 6.895f)));
}

void test_boost_absolute_vs_gauge() {
  TEST_ASSERT_FLOAT_WITHIN(1e-4, 1.5f, boostBar(1.5f, false, 1.013f));
  TEST_ASSERT_FLOAT_WITHIN(1e-4, 1.5f, boostBar(2.513f, true, 1.013f));
}

void test_psi() { TEST_ASSERT_FLOAT_WITHIN(0.01f, 22.0f, barToPsi(1.5168f)); }

void test_divider() { TEST_ASSERT_FLOAT_WITHIN(1e-4, 5.0f, undoDivider(3.0f, 25.0f / 15.0f)); }

void test_afr_and_lambda() {
  TEST_ASSERT_FLOAT_WITHIN(1e-4, 8.5f, afrFromVolts(0.0f, 8.5f, 18.0f));
  TEST_ASSERT_FLOAT_WITHIN(1e-4, 18.0f, afrFromVolts(5.0f, 8.5f, 18.0f));
  TEST_ASSERT_FLOAT_WITHIN(1e-4, 1.0f, lambdaFromAfr(14.7f, 14.7f));
}

void test_ema() {
  TEST_ASSERT_EQUAL_FLOAT(10.0f, ema(NAN, 10.0f, 0.5f));
  TEST_ASSERT_EQUAL_FLOAT(15.0f, ema(10.0f, 20.0f, 0.5f));
  TEST_ASSERT_EQUAL_FLOAT(10.0f, ema(10.0f, NAN, 0.5f));
}

void test_alarm_high_with_hysteresis() {
  const AlarmSpec egt{Direction::High, 650, 720, 15};
  Level l = Level::Ok;
  l = evalAlarm(egt, 600, l);
  TEST_ASSERT_EQUAL(Level::Ok, l);
  l = evalAlarm(egt, 655, l);
  TEST_ASSERT_EQUAL(Level::Warn, l);
  l = evalAlarm(egt, 640, l);  // inside hysteresis band
  TEST_ASSERT_EQUAL(Level::Warn, l);
  l = evalAlarm(egt, 634, l);
  TEST_ASSERT_EQUAL(Level::Ok, l);
  l = evalAlarm(egt, 730, l);
  TEST_ASSERT_EQUAL(Level::Crit, l);
  l = evalAlarm(egt, 710, l);  // still within 15 of crit
  TEST_ASSERT_EQUAL(Level::Crit, l);
  l = evalAlarm(egt, 700, l);
  TEST_ASSERT_EQUAL(Level::Warn, l);
  l = evalAlarm(egt, NAN, l);  // fault keeps level
  TEST_ASSERT_EQUAL(Level::Warn, l);
}

void test_alarm_low_side() {
  const AlarmSpec vlow{Direction::Low, 12.0f, 11.5f, 0.2f};
  Level l = Level::Ok;
  l = evalAlarm(vlow, 13.8f, l);
  TEST_ASSERT_EQUAL(Level::Ok, l);
  l = evalAlarm(vlow, 11.9f, l);
  TEST_ASSERT_EQUAL(Level::Warn, l);
  l = evalAlarm(vlow, 11.4f, l);
  TEST_ASSERT_EQUAL(Level::Crit, l);
  l = evalAlarm(vlow, 11.6f, l);
  TEST_ASSERT_EQUAL(Level::Crit, l);
  l = evalAlarm(vlow, 12.1f, l);
  TEST_ASSERT_EQUAL(Level::Warn, l);
  l = evalAlarm(vlow, 12.3f, l);
  TEST_ASSERT_EQUAL(Level::Ok, l);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_linear_pressure_endpoints);
  RUN_TEST(test_linear_pressure_ratiometric);
  RUN_TEST(test_boost_absolute_vs_gauge);
  RUN_TEST(test_psi);
  RUN_TEST(test_divider);
  RUN_TEST(test_afr_and_lambda);
  RUN_TEST(test_ema);
  RUN_TEST(test_alarm_high_with_hysteresis);
  RUN_TEST(test_alarm_low_side);
  return UNITY_END();
}
