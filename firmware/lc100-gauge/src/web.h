// Punct de acces Wi-Fi + pagină web pentru valori live, setări și log-uri.
// Oprit implicit; pornește/oprește la apăsare lungă pe buton și se oprește
// singur după `wifiIdleMin` minute fără cereri.
#pragma once

#include <cstdint>

namespace web {

void start();
void stop();
bool running();
void tick();  // de apelat din loop()

}  // namespace web
