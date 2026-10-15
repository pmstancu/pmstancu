// Cele trei display-uri GC9A01 pe aceeași magistrală SPI, cu CS separat.
#pragma once

#include "settings.h"
#include "snapshot.h"

namespace display {

struct Status {
  bool wifiOn;
  bool sdOk;
  bool muted;
};

void begin();
// Desenează doar ecranele al căror conținut s-a schimbat.
void update(const Snapshot& snap, const Settings& s, const Status& st);
// Text pe toate ecranele (pornire, mesaje scurte).
void message(const char* line1, const char* line2);

}  // namespace display
