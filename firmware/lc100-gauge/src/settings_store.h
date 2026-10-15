// Salvarea setărilor în NVS (memoria flash a ESP32), ca text JSON.
#pragma once

#include "settings.h"

namespace settings_store {

// Încarcă setările salvate peste valorile implicite. Dacă nu există sau sunt
// invalide, întoarce valorile implicite.
Settings load();
bool save(const Settings& s);
void erase();

}  // namespace settings_store
