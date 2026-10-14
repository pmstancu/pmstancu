#pragma once

#include "readings.h"

// Initialises the chips and starts the acquisition tasks on core 0.
void sensorsBegin();

// Thread-safe copy of the latest values, alarm levels and faults.
Snapshot sensorsSnapshot();
