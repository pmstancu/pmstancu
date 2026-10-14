#pragma once

#include "readings.h"

void displayBegin();

// Advances to the next page (button short press).
void displayNextPage();

// Redraws the screen. If a channel is in critical alarm, its page is forced.
void displayRender(const Snapshot& s, uint32_t nowMs);
