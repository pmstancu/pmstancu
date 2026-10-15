// Starea partajată între task-uri (senzori pe core 0, restul pe core 1).
// Fiecare acces face o copie sub mutex, ca nimeni să nu citească o
// structură pe jumătate scrisă.
#pragma once

#include <Arduino.h>

#include "aem_parser.h"
#include "settings.h"
#include "snapshot.h"

namespace shared {

void init();

Snapshot snapshot();
void setSnapshot(const Snapshot& s);

Settings settings();
uint32_t settingsVersion();     // crește la fiecare modificare
void setSettings(const Settings& s);

// Ultimii octeți primiți de la AEM, ca text hex pentru pagina web.
void aemRawPush(uint8_t b);
String aemRawHex();

}  // namespace shared
