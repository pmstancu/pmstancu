#include "display.h"

#include <Arduino.h>
#include <TFT_eSPI.h>

#include <cmath>

#include "config.h"

using conv::Level;

namespace {

TFT_eSPI tft;
TFT_eSprite spr(&tft);
bool gSpriteOk = false;

enum Page : uint8_t { PAGE_EGT, PAGE_BOOST, PAGE_TEMPS, PAGE_AFR, PAGE_COUNT };
uint8_t gPage = PAGE_EGT;

constexpr int kCx = 120, kCy = 120;
constexpr uint32_t kArcStart = 45, kArcEnd = 315;  // TFT_eSPI: 0° = 6 o'clock, clockwise

TFT_eSPI& canvas() { return gSpriteOk ? static_cast<TFT_eSPI&>(spr) : tft; }

uint16_t levelColor(Level l, uint32_t nowMs) {
  switch (l) {
    case Level::Crit: return (nowMs / 300) % 2 ? TFT_RED : TFT_WHITE;  // flashing
    case Level::Warn: return TFT_ORANGE;
    default: return TFT_GREEN;
  }
}

Page pageFor(Channel ch) {
  switch (ch) {
    case CH_EGT: return PAGE_EGT;
    case CH_BOOST: return PAGE_BOOST;
    case CH_VBAT:
    case CH_AFR: return PAGE_AFR;
    default: return PAGE_TEMPS;
  }
}

float boostForDisplay(float bar) { return BOOST_DISPLAY_PSI ? conv::barToPsi(bar) : bar; }
const char* boostUnit() { return BOOST_DISPLAY_PSI ? "psi" : "bar"; }

String fmt(float v, int decimals) { return std::isnan(v) ? String("---") : String(v, decimals); }

void drawGauge(TFT_eSPI& c, float value, float vMin, float vMax, uint16_t color) {
  c.drawSmoothArc(kCx, kCy, 118, 104, kArcStart, kArcEnd, TFT_DARKGREY, TFT_BLACK, false);
  if (std::isnan(value)) return;
  float frac = (value - vMin) / (vMax - vMin);
  frac = frac < 0 ? 0 : (frac > 1 ? 1 : frac);
  const uint32_t end = kArcStart + static_cast<uint32_t>(frac * (kArcEnd - kArcStart));
  if (end > kArcStart + 1) c.drawSmoothArc(kCx, kCy, 118, 104, kArcStart, end, color, TFT_BLACK, true);
}

void drawBigValue(TFT_eSPI& c, const String& value, const char* label, const String& footer, uint16_t color) {
  c.setTextDatum(MC_DATUM);
  c.setTextColor(color, TFT_BLACK);
  c.drawString(value, kCx, kCy, 7);  // 7-segment font: digits, '.', '-'
  c.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  c.drawString(label, kCx, kCy - 50, 4);
  c.drawString(footer, kCx, kCy + 58, 2);
}

void pageEgt(TFT_eSPI& c, const Snapshot& s, uint32_t now) {
  const uint16_t col = levelColor(s.level[CH_EGT], now);
  drawGauge(c, s.value[CH_EGT], 0, 900, col);
  drawBigValue(c, fmt(s.value[CH_EGT], 0), "EGT C",
               "BOOST " + fmt(boostForDisplay(s.value[CH_BOOST]), 1) + " " + boostUnit(), col);
}

void pageBoost(TFT_eSPI& c, const Snapshot& s, uint32_t now) {
  const uint16_t col = levelColor(s.level[CH_BOOST], now);
  const float maxScale = BOOST_DISPLAY_PSI ? 30.0f : 2.0f;
  drawGauge(c, boostForDisplay(s.value[CH_BOOST]), 0, maxScale, col);
  drawBigValue(c, fmt(boostForDisplay(s.value[CH_BOOST]), 1), boostUnit(), "EGT " + fmt(s.value[CH_EGT], 0) + " C",
               col);
}

void pageTemps(TFT_eSPI& c, const Snapshot& s, uint32_t now) {
  struct Row {
    const char* label;
    Channel ch;
    int y;
  } rows[] = {{"APA", CH_ECT, 65}, {"ULEI", CH_EOT, 120}, {"AER", CH_IAT, 175}};
  for (const auto& r : rows) {
    c.setTextDatum(MR_DATUM);
    c.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    c.drawString(r.label, 105, r.y, 4);
    c.setTextDatum(ML_DATUM);
    c.setTextColor(levelColor(s.level[r.ch], now), TFT_BLACK);
    c.drawString(fmt(s.value[r.ch], 0), 120, r.y, 6);
  }
}

void pageAfr(TFT_eSPI& c, const Snapshot& s, uint32_t now) {
  const float afr = s.value[CH_AFR];
  const float lambda = std::isnan(afr) ? NAN : conv::lambdaFromAfr(afr, AFR_STOICH_GAS);
  drawGauge(c, lambda, 0.6f, 1.6f, TFT_CYAN);
  c.setTextDatum(MC_DATUM);
  c.setTextColor(TFT_CYAN, TFT_BLACK);
  c.drawString(fmt(lambda, 2), kCx, kCy - 10, 7);
  c.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  c.drawString("LAMBDA  AFR " + fmt(afr, 1), kCx, kCy - 60, 2);
  c.setTextColor(levelColor(s.level[CH_VBAT], now), TFT_BLACK);
  c.drawString(fmt(s.value[CH_VBAT], 1) + " V", kCx, kCy + 50, 4);
}

}  // namespace

void displayBegin() {
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
#endif
  // Full-screen sprite for flicker-free redraws; fall back to 8-bit, then direct drawing.
  gSpriteOk = spr.createSprite(240, 240) != nullptr;
  if (!gSpriteOk) {
    spr.setColorDepth(8);
    gSpriteOk = spr.createSprite(240, 240) != nullptr;
  }
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("1HD-FTE", kCx, kCy - 15, 4);
  tft.drawString("fw " FW_VERSION, kCx, kCy + 15, 2);
}

void displayNextPage() { gPage = (gPage + 1) % PAGE_COUNT; }

void displayRender(const Snapshot& s, uint32_t nowMs) {
  uint8_t page = gPage;
  for (int ch = 0; ch < CH_COUNT; ++ch) {
    if (s.level[ch] == Level::Crit) {
      page = pageFor(static_cast<Channel>(ch));
      break;
    }
  }

  TFT_eSPI& c = canvas();
  if (gSpriteOk) spr.fillSprite(TFT_BLACK);
  else tft.fillScreen(TFT_BLACK);  // flickers, but only used if RAM is short

  switch (page) {
    case PAGE_EGT: pageEgt(c, s, nowMs); break;
    case PAGE_BOOST: pageBoost(c, s, nowMs); break;
    case PAGE_TEMPS: pageTemps(c, s, nowMs); break;
    default: pageAfr(c, s, nowMs); break;
  }

  if (gSpriteOk) spr.pushSprite(0, 0);
}
