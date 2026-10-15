# 2. Lista de componente

Prețurile nu sunt incluse, pentru că variază și nu le pot verifica. Pentru piesele marcate
cu ⚠ citește observația.

## Ai deja

| Piesă | Observație |
|---|---|
| AEM 30-0310 X-Series Inline | ⚠ confirmă în manual scalarea ieșirii analogice, formatul RS232 și culorile firelor |
| 3 × Pt100 RTD | ⚠ sondele cu **3 fire** sunt mai bune (compensează rezistența cablului) |
| Termocuplu tip K (EGT) | ⚠ joncțiunea **izolată** (ungrounded) e preferabilă, vezi [04](04-montaj-senzori.md) |
| Senzor presiune Auber | −0.3…3.3 bar relativ, 5 V, ieșire 0.1–4.85 V ratiometrică (din fișa tehnică) |
| MAX31865 (câte unul pentru fiecare Pt100, deci 3) | corect pentru Pt100 |
| 3 × display GC9A01 1.28" | ⚠ verifică dacă modulul tău acceptă 3.3 V pe VCC și unde e pinul BL/BLK |

## De cumpărat

| Cant. | Piesă | Rol |
|---|---|---|
| 1 | **ESP32-S3-DevKitC-1** (modul WROOM-1, de ex. N8 sau N16R8) | controllerul |
| 1 | **MAX31856** pe placă breakout | ⚠ **pentru termocuplul tip K**; MAX31865 nu îl poate citi |
| 1 | **ADS1115** pe placă breakout | ADC 16 biți: AEM analogic, boost, baterie, referința de 5 V |
| 1 | **Modul MAX3232** (RS232 ↔ TTL), care merge la 3.3 V | ⚠ ieșirea RS232 a AEM are niveluri de ±V; ESP32 ar fi distrus fără convertor |
| 1 | **Modul microSD SPI** + card **≤ 32 GB, FAT32** | log. ⚠ multe module ieftine au regulator pentru 5 V; alimentează-l conform plăcii |
| 1 | Convertor DC-DC auto 12 V → 5 V, ≥ 2 A, cu protecție la intrare | alimentarea ESP32 și a display-urilor |
| 1 | Regulator 5 V liniar (de ex. LM2940-5, cu condensatorii din fișa tehnică) | alimentare curată pentru senzorul Auber |
| 1 | Diodă TVS pentru 12 V (de ex. seria SMBJ, tensiune de lucru ~24 V) | ⚠ alege după fișa tehnică; protecție la vârfuri de tensiune |
| 1 | Siguranță auto 3 A + port-siguranță | protecție |
| — | Rezistențe 1 %: 3 × 10 kΩ, 3 × 15 kΩ, 1 × 100 kΩ, 1 × 22 kΩ | divizoare |
| 4 | Condensatori ceramici 100 nF | filtrare la intrările ADC |
| 4 | Diode Schottky duble BAT54S | protecția intrărilor ADC (limitare la 3.3 V / GND) |
| 1 | Rezistență 10 kΩ | pull-up pe MISO-ul senzorilor |
| 1 | Buzzer activ 5 V + tranzistor NPN (de ex. BC337/2N2222) + rezistență 1 kΩ | alarmă sonoră |
| 1 | Buton momentan | oprire buzzer / Wi-Fi |
| — | **Cablu de extensie tip K** | de la sondă la MAX31856; **nu** cupru obișnuit |
| — | Cablu ecranat cu 3 fire (Pt100), cablu ecranat cu 2 fire (semnale analogice) | |
| — | Cablu scurt pentru display-uri (cât mai scurt, vezi [03](03-cablaj.md)) | |
| 1 | Cutie IP54+ pentru electronică, conectori auto (de ex. Deutsch DT sau similari) | |
| — | Adaptoare pentru montajul senzorilor (vezi [04](04-montaj-senzori.md)) | |

## Consum estimat

Valori aproximative, de verificat cu un ampermetru:

- ESP32-S3: sub 0.1 A fără Wi-Fi, vârfuri de câteva sute de mA cu Wi-Fi pornit;
- display-urile (lumina de fundal): câteva zeci de mA fiecare;
- senzorii și plăcile lor: sub 0.1 A.

Un convertor de 2 A are rezervă suficientă.
