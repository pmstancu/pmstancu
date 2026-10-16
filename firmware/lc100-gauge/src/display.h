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

// Pentru testul de display-uri (firmware/display-test):
// umple un singur ecran cu o culoare și un text mare.
void identify(uint8_t screen, uint16_t color, const char* line1, const char* line2);
// Următorul update() redesenează toate ecranele, chiar dacă nu s-a schimbat nimic.
void forceRedraw();
// Adâncimea de culoare a buffer-ului: 16, 8 sau 0 (alocarea a eșuat).
uint8_t bufferDepth();

}  // namespace display
