#include "display.h"

#include <Arduino.h>
#include <TFT_eSPI.h>

#include <cstring>

#include "display_logic.h"
#include "pins.h"

namespace display {
namespace {

TFT_eSPI tft;
TFT_eSprite spr(&tft);
bool sprOk = false;

const uint8_t kCs[kScreens] = {PIN_CS_DISP1, PIN_CS_DISP2, PIN_CS_DISP3};
uint32_t lastHash[kScreens] = {0, 0, 0};

constexpr int16_t kCx = 120, kCy = 120;
// Unghiuri TFT_eSPI: 0° jos (ora 6), crește în sensul acelor de ceasornic.
constexpr uint16_t kArcStart = 45, kArcEnd = 315;

constexpr uint16_t COL_BG = TFT_BLACK;
constexpr uint16_t COL_TRACK = 0x2104;    // gri foarte închis
constexpr uint16_t COL_OK = 0x07E8;       // verde
constexpr uint16_t COL_WARN = 0xFD20;     // portocaliu
constexpr uint16_t COL_CRIT = TFT_RED;
constexpr uint16_t COL_CRIT_BG = 0x7800;  // roșu închis (fundal la alarmă critică)
constexpr uint16_t COL_TEXT = TFT_WHITE;
constexpr uint16_t COL_DIM = 0x8410;      // gri

void selectAll(bool on) {
  for (uint8_t p : kCs) digitalWrite(p, on ? LOW : HIGH);
}

uint16_t levelColor(alarm::Level l) {
  return l == alarm::CRIT ? COL_CRIT : (l == alarm::WARN ? COL_WARN : COL_OK);
}

// FNV-1a: detectează dacă ce ar trebui desenat s-a schimbat.
uint32_t fnv(uint32_t h, const void* data, size_t n) {
  const uint8_t* p = static_cast<const uint8_t*>(data);
  for (size_t i = 0; i < n; i++) h = (h ^ p[i]) * 16777619u;
  return h;
}

// Fonturile 6/7 de la TFT_eSPI au doar cifre, „.” și „-”. Prefixul „>”/„<”
// al lui λ se desenează separat, cu fontul 4.
void drawBigNumber(const char* text, int16_t y, uint16_t color, uint16_t bg) {
  const char* num = text;
  char prefix = 0;
  if (*num == '>' || *num == '<') prefix = *num++;
  spr.setTextColor(color, bg);
  spr.setTextDatum(MC_DATUM);
  uint8_t font = std::strlen(num) > 4 ? 4 : 7;
  if (font == 4) spr.setTextSize(2);
  int16_t w = spr.drawString(num, kCx, y, font);
  spr.setTextSize(1);
  if (prefix) {
    char p[2] = {prefix, 0};
    spr.drawString(p, kCx - w / 2 - 12, y, 4);
  }
}

void render(uint8_t idx, const ScreenPair& pair, const Snapshot& snap, const Settings& s,
            const Status& st, bool blink) {
  const uint8_t m = pair.main, sub = pair.sub;
  const bool critBg = (snap.level[m] == alarm::CRIT || snap.level[sub] == alarm::CRIT) && blink;
  const uint16_t bg = critBg ? COL_CRIT_BG : COL_BG;
  spr.fillSprite(bg);

  // Arcul canalului principal
  spr.drawSmoothArc(kCx, kCy, 119, 105, kArcStart, kArcEnd, COL_TRACK, bg, true);
  float frac = gaugeFraction(m, snap.v[m], s.boostPsi);
  if (!std::isnan(snap.v[m]) && frac > 0.005f) {
    uint16_t end = kArcStart + static_cast<uint16_t>(frac * (kArcEnd - kArcStart));
    if (end > kArcStart) spr.drawSmoothArc(kCx, kCy, 119, 105, kArcStart, end, levelColor(snap.level[m]), bg, true);
  }

  char buf[16];
  // Eticheta și unitatea canalului principal
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(COL_DIM, bg);
  spr.drawString(chLabel(m), kCx, 52, 4);
  formatValue(m, snap.v[m], s.boostPsi, m == CH_LAMBDA ? snap.lambdaClip : conv::Clip::None, buf, sizeof buf);
  uint16_t valCol = snap.level[m] == alarm::OK ? COL_TEXT : levelColor(snap.level[m]);
  drawBigNumber(buf, 105, valCol, bg);
  spr.setTextColor(COL_DIM, bg);
  if (std::isnan(snap.v[m]) && snap.fault[m] != FAULT_OK) spr.drawString("SENZOR", kCx, 145, 2);
  else spr.drawString(displayUnit(m, s.boostPsi), kCx, 145, 2);

  // Separator + valoarea secundară
  spr.drawFastHLine(60, 160, 120, COL_TRACK);
  spr.setTextColor(COL_DIM, bg);
  spr.drawString(chLabel(sub), kCx, 174, 2);
  formatValue(sub, snap.v[sub], s.boostPsi, sub == CH_LAMBDA ? snap.lambdaClip : conv::Clip::None, buf, sizeof buf);
  char line[24];
  const char* unit = displayUnit(sub, s.boostPsi);
  std::snprintf(line, sizeof line, "%s%s%s", buf, *unit ? " " : "", unit);
  spr.setTextColor(snap.level[sub] == alarm::OK ? COL_TEXT : levelColor(snap.level[sub]), bg);
  spr.drawString(line, kCx, 198, 4);

  // Indicatori mici, doar pe primul ecran
  if (idx == 0) {
    spr.setTextColor(COL_DIM, bg);
    if (st.wifiOn) spr.drawString("WiFi", kCx, 22, 2);
    if (!st.sdOk) {
      spr.setTextColor(COL_WARN, bg);
      spr.drawString("SD!", kCx + 40, 30, 2);
    }
    if (st.muted) {
      spr.setTextColor(COL_WARN, bg);
      spr.drawString("MUT", kCx - 40, 30, 2);
    }
  }
  // Sursa lui λ: S = serial, A = analogic
  if (m == CH_LAMBDA || sub == CH_LAMBDA) {
    const char* src = snap.lambdaSrc == conv::LambdaSrc::Serial ? "S" : (snap.lambdaSrc == conv::LambdaSrc::Analog ? "A" : "");
    spr.setTextColor(COL_DIM, bg);
    spr.drawString(src, kCx, 222, 2);
  }
}

uint32_t screenHash(uint8_t idx, const ScreenPair& p, const Snapshot& snap, const Settings& s,
                    const Status& st, bool blink) {
  uint32_t h = 2166136261u;
  char buf[16];
  for (uint8_t ch : {p.main, p.sub}) {
    formatValue(ch, snap.v[ch], s.boostPsi, snap.lambdaClip, buf, sizeof buf);
    h = fnv(h, buf, std::strlen(buf));
    h = fnv(h, &ch, 1);
    h = fnv(h, &snap.level[ch], 1);
    uint8_t faulty = std::isnan(snap.v[ch]) && snap.fault[ch] != FAULT_OK;
    h = fnv(h, &faulty, 1);
    // Arcul se mișcă și când textul nu se schimbă
    uint16_t arc = static_cast<uint16_t>(gaugeFraction(ch, snap.v[ch], s.boostPsi) * 270);
    h = fnv(h, &arc, sizeof arc);
  }
  // Clipirea contează doar când ecranul are o alarmă critică.
  const bool crit = snap.level[p.main] == alarm::CRIT || snap.level[p.sub] == alarm::CRIT;
  uint8_t flags = (crit && blink ? 1 : 0) | (st.wifiOn ? 2 : 0) | (st.sdOk ? 4 : 0) | (st.muted ? 8 : 0) |
                  (s.boostPsi ? 16 : 0) | (static_cast<uint8_t>(snap.lambdaSrc) << 5);
  h = fnv(h, &flags, 1);
  return h ^ idx;
}

}  // namespace

void begin() {
  for (uint8_t p : kCs) pinMode(p, OUTPUT);
  // Inițializare simultană: toate CS-urile active, apoi toate inactive.
  selectAll(true);
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(COL_BG);
  selectAll(false);

  spr.setColorDepth(16);
  sprOk = spr.createSprite(240, 240) != nullptr;
  if (!sprOk) {
    // Memorie insuficientă (fără PSRAM, heap fragmentat): 8 biți = jumătate.
    spr.setColorDepth(8);
    sprOk = spr.createSprite(240, 240) != nullptr;
  }
}

void message(const char* line1, const char* line2) {
  selectAll(true);
  tft.fillScreen(COL_BG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString(line1, kCx, 105, 4);
  tft.setTextColor(COL_DIM, COL_BG);
  if (line2) tft.drawString(line2, kCx, 140, 2);
  selectAll(false);
  for (uint32_t& h : lastHash) h = 0;
}

void update(const Snapshot& snap, const Settings& s, const Status& st) {
  if (!sprOk) return;
  const bool blink = (millis() / 400) % 2 == 0;
  ScreenPair layout[kScreens];
  effectiveLayout(s, snap, layout);

  for (uint8_t i = 0; i < kScreens; i++) {
    uint32_t h = screenHash(i, layout[i], snap, s, st, blink);
    if (h == lastHash[i]) continue;
    lastHash[i] = h;
    render(i, layout[i], snap, s, st, blink);
    digitalWrite(kCs[i], LOW);
    spr.pushSprite(0, 0);
    digitalWrite(kCs[i], HIGH);
  }
}

}  // namespace display
