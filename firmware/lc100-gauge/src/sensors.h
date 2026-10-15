// Achiziția senzorilor: două task-uri pe core 0.
//  - „temps”: 3 × Pt100 (MAX31865) + EGT (MAX31856), ~4 Hz
//  - „fast”:  ADS1115 (AEM analogic, boost, baterie, ref. 5 V) + serial AEM,
//             combină totul, evaluează alarmele și publică Snapshot-ul, ~40 Hz
#pragma once

namespace sensors {
void begin();
}
