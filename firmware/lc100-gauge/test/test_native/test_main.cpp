// Teste pentru logica pură. Rulează pe PC: pio test -e native
#include <unity.h>

#include <cstring>
#include <string>

#include "aem_parser.h"
#include "alarms.h"
#include "conversions.h"
#include "display_logic.h"
#include "logname.h"
#include "settings.h"
#include "snapshot.h"

void setUp() {}
void tearDown() {}

// ---------------------------------------------------------------- conversii
void test_rtd_temperature() {
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, conv::rtdTemperature(100.0f, 100.0f));
  // Tabel IEC 60751: 138.51 Ω la 100 °C, 80.31 Ω la -50 °C
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 100.0f, conv::rtdTemperature(138.51f, 100.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.2f, -50.0f, conv::rtdTemperature(80.31f, 100.0f));
}

void test_rtd_resistance() {
  // 100 Ω cu RREF 430 => raw = 100/430*32768
  uint16_t raw = static_cast<uint16_t>(100.0f / 430.0f * 32768.0f + 0.5f);
  TEST_ASSERT_FLOAT_WITHIN(0.02f, 100.0f, conv::rtdResistance(raw, 430.0f));
}

void test_auber_pressure() {
  Settings s = defaultSettings();
  auto p = [&](float v, float sup) {
    return conv::linearPressure(v, sup, true, s.boostVMin, s.boostVMax, s.boostPMin, s.boostPMax);
  };
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.3f, p(0.1f, 5.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.3f, p(4.85f, 5.0f));
  // 0 bar relativ: V = 0.1 + 0.3/3.6*4.75 = 0.4958 V
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, p(0.49583f, 5.0f));
  // Ratiometric: alimentare 4.9 V, semnal scalat proporțional => aceeași presiune
  TEST_ASSERT_FLOAT_WITHIN(0.001f, p(2.0f, 5.0f), p(2.0f * 4.9f / 5.0f, 4.9f));
  // Referință lipsă => NaN
  TEST_ASSERT_TRUE(std::isnan(p(2.0f, 0.2f)));
}

void test_afr_and_lambda() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 8.5f, conv::afrFromVolts(0.0f, 8.5f, 18.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 18.0f, conv::afrFromVolts(5.0f, 8.5f, 18.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, conv::lambdaFromAfr(14.7f, 14.7f));
}

void test_ema() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, conv::ema(NAN, 5.0f, 0.5f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 7.5f, conv::ema(5.0f, 10.0f, 0.5f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, conv::ema(5.0f, NAN, 0.5f));
}

void test_choose_lambda() {
  auto c = conv::chooseLambda(1.8f, 200, 1000, 1.2f, 4.99f);
  TEST_ASSERT_EQUAL(conv::LambdaSrc::Serial, c.src);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.8f, c.lambda);
  // Serial vechi => analogic, la capăt de scală
  c = conv::chooseLambda(1.8f, 5000, 1000, 1.22f, 4.99f);
  TEST_ASSERT_EQUAL(conv::LambdaSrc::Analog, c.src);
  TEST_ASSERT_EQUAL(conv::Clip::Above, c.clip);
  c = conv::chooseLambda(NAN, 0, 1000, NAN, NAN);
  TEST_ASSERT_EQUAL(conv::LambdaSrc::None, c.src);
}

// ---------------------------------------------------------------- alarme
void test_alarm_high_hysteresis() {
  alarm::Spec s{true, 100, 105, 2};
  alarm::Level l = alarm::OK;
  l = alarm::eval(s, alarm::Dir::High, 99.9f, l);  TEST_ASSERT_EQUAL(alarm::OK, l);
  l = alarm::eval(s, alarm::Dir::High, 100.0f, l); TEST_ASSERT_EQUAL(alarm::WARN, l);
  l = alarm::eval(s, alarm::Dir::High, 98.5f, l);  TEST_ASSERT_EQUAL(alarm::WARN, l);  // în histerezis
  l = alarm::eval(s, alarm::Dir::High, 106.0f, l); TEST_ASSERT_EQUAL(alarm::CRIT, l);
  l = alarm::eval(s, alarm::Dir::High, 104.0f, l); TEST_ASSERT_EQUAL(alarm::CRIT, l);
  l = alarm::eval(s, alarm::Dir::High, 102.9f, l); TEST_ASSERT_EQUAL(alarm::WARN, l);
  l = alarm::eval(s, alarm::Dir::High, NAN, l);    TEST_ASSERT_EQUAL(alarm::WARN, l);
  l = alarm::eval(s, alarm::Dir::High, 97.9f, l);  TEST_ASSERT_EQUAL(alarm::OK, l);
}

void test_alarm_low_and_disabled() {
  alarm::Spec s{true, 12.0f, 11.5f, 0.2f};
  alarm::Level l = alarm::eval(s, alarm::Dir::Low, 11.4f, alarm::OK);
  TEST_ASSERT_EQUAL(alarm::CRIT, l);
  l = alarm::eval(s, alarm::Dir::Low, 11.6f, l);
  TEST_ASSERT_EQUAL(alarm::CRIT, l);
  l = alarm::eval(s, alarm::Dir::Low, 11.8f, l);
  TEST_ASSERT_EQUAL(alarm::WARN, l);
  s.enabled = false;
  TEST_ASSERT_EQUAL(alarm::OK, alarm::eval(s, alarm::Dir::Low, 5.0f, alarm::CRIT));
}

// ---------------------------------------------------------------- AEM serial
static int feedLine(AemLineParser& p, const char* line, AemUnit u, float* out) {
  int hits = 0;
  for (const char* c = line; *c; c++) hits += p.feed(*c, u, 14.7f, out) ? 1 : 0;
  return hits;
}

void test_aem_parser() {
  AemLineParser p;
  float l = 0;
  TEST_ASSERT_EQUAL(1, feedLine(p, "14.7\r\n", AemUnit::Auto, &l));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, l);
  TEST_ASSERT_EQUAL(1, feedLine(p, "29.4\n", AemUnit::Afr, &l));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, l);
  TEST_ASSERT_EQUAL(1, feedLine(p, "1.35\r", AemUnit::Auto, &l));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.35f, l);
  TEST_ASSERT_EQUAL(1, feedLine(p, "AFR: 22.0\r\n", AemUnit::Auto, &l));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 22.0f / 14.7f, l);
  // Linii fără număr sau neplauzibile sunt ignorate
  TEST_ASSERT_EQUAL(0, feedLine(p, "----\r\n", AemUnit::Auto, &l));
  TEST_ASSERT_EQUAL(0, feedLine(p, "0.0\r\n", AemUnit::Auto, &l));
}

void test_raw_log() {
  RawLog<4> r;
  for (uint8_t b = 1; b <= 6; b++) r.push(b);
  TEST_ASSERT_EQUAL(4, r.size());
  TEST_ASSERT_EQUAL(3, r.at(0));
  TEST_ASSERT_EQUAL(6, r.at(3));
}

// ---------------------------------------------------------------- setări
void test_defaults_valid() { TEST_ASSERT_NULL(validate(defaultSettings())); }

void test_validation_rejects() {
  Settings s = defaultSettings();
  s.alarms[AL_EGT].crit = 600;  // sub avertizare (650)
  TEST_ASSERT_NOT_NULL(validate(s));
  s = defaultSettings();
  std::strcpy(s.wifiPass, "short");
  TEST_ASSERT_NOT_NULL(validate(s));
  s = defaultSettings();
  s.rtdRref = NAN;
  TEST_ASSERT_NOT_NULL(validate(s));
}

void test_settings_json_roundtrip() {
  Settings a = defaultSettings();
  a.alarms[AL_EGT].warn = 640;
  a.rtdTrim[1] = -0.7f;
  a.screenMain[2] = CH_VBAT;
  a.boostPsi = false;
  JsonDocument doc;
  toJson(a, doc.to<JsonObject>(), true);
  std::string text;
  serializeJson(doc, text);

  JsonDocument in;
  TEST_ASSERT_FALSE(deserializeJson(in, text));
  Settings b = defaultSettings();
  TEST_ASSERT_TRUE(fromJson(in.as<JsonObjectConst>(), b));
  TEST_ASSERT_EQUAL_FLOAT(640, b.alarms[AL_EGT].warn);
  TEST_ASSERT_EQUAL_FLOAT(-0.7f, b.rtdTrim[1]);
  TEST_ASSERT_EQUAL(CH_VBAT, b.screenMain[2]);
  TEST_ASSERT_FALSE(b.boostPsi);
  TEST_ASSERT_EQUAL_STRING(a.wifiPass, b.wifiPass);
}

void test_settings_partial_and_bad_channel() {
  Settings s = defaultSettings();
  JsonDocument in;
  deserializeJson(in, R"({"alarms":{"ect":{"warn":98}},"log_ms":200})");
  TEST_ASSERT_TRUE(fromJson(in.as<JsonObjectConst>(), s));
  TEST_ASSERT_EQUAL_FLOAT(98, s.alarms[AL_ECT].warn);
  TEST_ASSERT_EQUAL_FLOAT(105, s.alarms[AL_ECT].crit);  // neschimbat
  TEST_ASSERT_EQUAL(200, s.logPeriodMs);
  deserializeJson(in, R"({"screens":[{"main":"rpm","sub":"egt"}]})");
  TEST_ASSERT_FALSE(fromJson(in.as<JsonObjectConst>(), s));
}

// ---------------------------------------------------------------- afișaj / CSV
void test_layout_shows_hidden_critical() {
  Settings s = defaultSettings();  // VBAT nu e pe niciun ecran
  Snapshot snap;
  ScreenPair p[kScreens];
  effectiveLayout(s, snap, p);
  TEST_ASSERT_EQUAL(CH_EOT, p[2].sub);
  snap.level[CH_VBAT] = alarm::CRIT;
  effectiveLayout(s, snap, p);
  TEST_ASSERT_EQUAL(CH_VBAT, p[2].sub);
  snap.level[CH_VBAT] = alarm::OK;
  snap.level[CH_EGT] = alarm::CRIT;  // deja afișat
  effectiveLayout(s, snap, p);
  TEST_ASSERT_EQUAL(CH_EOT, p[2].sub);
}

void test_format_value() {
  char b[16];
  formatValue(CH_BOOST, 1.0f, true, conv::Clip::None, b, sizeof b);
  TEST_ASSERT_EQUAL_STRING("14.5", b);
  formatValue(CH_BOOST, 1.0f, false, conv::Clip::None, b, sizeof b);
  TEST_ASSERT_EQUAL_STRING("1.00", b);
  formatValue(CH_LAMBDA, 1.22f, false, conv::Clip::Above, b, sizeof b);
  TEST_ASSERT_EQUAL_STRING(">1.22", b);
  formatValue(CH_EGT, NAN, false, conv::Clip::None, b, sizeof b);
  TEST_ASSERT_EQUAL_STRING("---", b);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, gaugeFraction(CH_EGT, 2000, false));
}

void test_csv_row() {
  Snapshot s;
  s.ms = 1234;
  s.v[CH_ECT] = 88.04f;
  s.v[CH_BOOST] = 1.5f;
  s.level[CH_EGT] = alarm::WARN;
  char line[200];
  csvRow(s, line, sizeof line);
  TEST_ASSERT_EQUAL_STRING("1234,88.0,,,,1.50,,,0,,,1\n", line);
  char head[200];
  csvHeader(head, sizeof head);
  // același număr de coloane
  int ch = 0, cl = 0;
  for (char* c = head; *c; c++) ch += *c == ',';
  for (char* c = line; *c; c++) cl += *c == ',';
  TEST_ASSERT_EQUAL(ch, cl);
}

void test_log_names() {
  TEST_ASSERT_EQUAL(7, logIndex("log_0007.csv"));
  TEST_ASSERT_EQUAL(7, logIndex("/log_0007.csv"));
  TEST_ASSERT_EQUAL(-1, logIndex("log_0000.csv"));
  TEST_ASSERT_EQUAL(-1, logIndex("log_00a7.csv"));
  TEST_ASSERT_FALSE(isLogName("../log_0001.csv"));
  TEST_ASSERT_FALSE(isLogName("/log_0001.csv"));
  TEST_ASSERT_FALSE(isLogName("log_0001.csv/x"));
  TEST_ASSERT_TRUE(isLogName("log_0001.csv"));
  char b[20];
  logName(42, b, sizeof b);
  TEST_ASSERT_EQUAL_STRING("/log_0042.csv", b);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_rtd_temperature);
  RUN_TEST(test_rtd_resistance);
  RUN_TEST(test_auber_pressure);
  RUN_TEST(test_afr_and_lambda);
  RUN_TEST(test_ema);
  RUN_TEST(test_choose_lambda);
  RUN_TEST(test_alarm_high_hysteresis);
  RUN_TEST(test_alarm_low_and_disabled);
  RUN_TEST(test_aem_parser);
  RUN_TEST(test_raw_log);
  RUN_TEST(test_defaults_valid);
  RUN_TEST(test_validation_rejects);
  RUN_TEST(test_settings_json_roundtrip);
  RUN_TEST(test_settings_partial_and_bad_channel);
  RUN_TEST(test_layout_shows_hidden_critical);
  RUN_TEST(test_format_value);
  RUN_TEST(test_csv_row);
  RUN_TEST(test_log_names);
  return UNITY_END();
}
