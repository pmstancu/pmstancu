// Log CSV pe microSD. Un fișier nou la fiecare pornire: /log_0001.csv, ...
// Rulează în loop() (core 1), la fel ca serverul web, deci accesul la SD
// nu are nevoie de mutex.
#pragma once

#include "settings.h"
#include "snapshot.h"

namespace logger {

void begin();
void tick(const Snapshot& snap, const Settings& s);
bool ok();                 // card montat și ultima scriere reușită
const char* currentFile(); // "" dacă nu se scrie

}  // namespace logger
