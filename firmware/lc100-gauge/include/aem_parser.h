// Parser pentru ieșirea RS232 a AEM 30-0310. Pur, testabil pe PC.
//
// Formatul exact NU e confirmat din manualul AEM. Surse secundare (forumuri)
// spun 9600 8N1, cam 10 cadre/s, câte o valoare text de forma „xx.x” pe linie.
// Parserul e deliberat tolerant: ia primul număr de pe fiecare linie. Ultimii
// octeți primiți se păstrează (RawLog) ca să poată fi văzuți în pagina web
// și formatul să fie confirmat pe mașină.
#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "settings.h"

class AemLineParser {
 public:
  // Întoarce true când o linie completă conține un număr valid; λ în *lambdaOut.
  bool feed(char c, AemUnit unit, float stoich, float* lambdaOut) {
    if (c == '\r' || c == '\n') {
      bool ok = len_ > 0 && parseLine(unit, stoich, lambdaOut);
      len_ = 0;
      return ok;
    }
    if (len_ < sizeof(buf_) - 1) buf_[len_++] = c;
    else len_ = 0;  // linie prea lungă: nu e formatul așteptat, o ignorăm
    return false;
  }

  // Interpretează un număr ca λ sau AFR. În modul Auto, sub 5 e λ (AFR-ul
  // nu poate fi atât de mic), altfel AFR pe scara benzinei.
  static float toLambda(float v, AemUnit unit, float stoich) {
    bool isLambda = unit == AemUnit::Lambda || (unit == AemUnit::Auto && v < 5.0f);
    float l = isLambda ? v : v / stoich;
    return (l >= 0.5f && l <= 15.0f) ? l : NAN;
  }

 private:
  bool parseLine(AemUnit unit, float stoich, float* out) {
    buf_[len_] = 0;
    for (size_t i = 0; i < len_; i++) {
      char c = buf_[i];
      if ((c >= '0' && c <= '9') || (c == '.' && i + 1 < len_ && buf_[i + 1] >= '0' && buf_[i + 1] <= '9')) {
        char* end = nullptr;
        float v = std::strtof(buf_ + i, &end);
        if (end == buf_ + i) return false;
        float l = toLambda(v, unit, stoich);
        if (std::isnan(l)) return false;
        *out = l;
        return true;
      }
    }
    return false;
  }

  char buf_[32] = {};
  size_t len_ = 0;
};

// Ultimii N octeți primiți, pentru diagnostic.
template <size_t N>
class RawLog {
 public:
  void push(uint8_t b) {
    data_[head_] = b;
    head_ = (head_ + 1) % N;
    if (count_ < N) count_++;
  }
  size_t size() const { return count_; }
  // i = 0 e cel mai vechi octet păstrat.
  uint8_t at(size_t i) const { return data_[(head_ + N - count_ + i) % N]; }

 private:
  uint8_t data_[N] = {};
  size_t head_ = 0;
  size_t count_ = 0;
};
