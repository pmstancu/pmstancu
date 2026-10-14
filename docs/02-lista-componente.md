# 2. Lista de componente

Prețurile nu sunt incluse, pentru că variază și nu le pot verifica. Pentru piesele marcate
cu ⚠ citește observația.

## Ai deja

| Piesă | Observație |
|---|---|
| CM5 + placă de bază + ecran 7" touch | dashboard, log, SDR/GPS |
| Mini-Box M2-ATX-HV | alimentarea CM5 în mașină, vezi [05](05-cm5-instalare.md) |
| AEM 30-0310 X-Series Inline | ⚠ confirmă scalarea ieșirii analogice în manualul AEM |
| 3× Pt100 RTD | ⚠ prefer sonde cu **3 fire** |
| Termocuplu tip K (EGT) | ⚠ prefer joncțiune **izolată** (ungrounded), vezi [04](04-montaj-senzori.md) |
| Senzor presiune Auber | ⚠ confirmă domeniul, tipul ieșirii și dacă e absolut sau relativ |
| 3× MAX31865 | corect pentru Pt100 |

## De cumpărat

| Cant. | Piesă | Rol |
|---|---|---|
| 1 | **ESP32-S3-DevKitC-1** (sau clonă compatibilă cu același pinout) | controller senzori |
| 1 | **MAX31856** pe placă breakout (ex. placa Adafruit MAX31856 sau echivalent) | **termocuplul tip K**; MAX31865 nu îl poate citi |
| 1 | **ADS1115** pe placă breakout | ADC 16 biți pentru AFR, boost, baterie, referința de 5 V |
| 1 | Display rotund 1.28" **GC9A01** 240×240 SPI | afișaj local |
| 1 | Convertor DC-DC auto 12 V → 5 V, ≥ 2 A, cu protecție la intrare | alimentare ESP32 |
| 1 | Regulator 5 V cu zgomot redus (ex. LM2940-5 sau similar, cu condensatorii din fișa tehnică) | alimentarea senzorului de presiune |
| 1 | Diodă TVS pentru linia de 12 V (ex. seria SMBJ, tensiune de lucru ~24 V) | ⚠ alege după fișa tehnică; protecție la vârfuri de tensiune |
| 1 | Siguranță auto 3 A + port-siguranță | protecție |
| — | Rezistențe 1 %: 3× 10 kΩ, 3× 15 kΩ, 1× 100 kΩ, 1× 22 kΩ | divizoare |
| 4 | Condensatori 100 nF ceramici | filtrare la intrările ADC |
| 4 | Diode Schottky duble BAT54S | protecție intrări ADC la 3.3 V/GND |
| 1 | Rezistență 10 kΩ | pull-up pe MISO (senzori) |
| 1 | Buzzer activ 5 V + tranzistor NPN (ex. 2N2222/BC337) + rezistență 1 kΩ | alarmă sonoră |
| 1 | Buton momentan | schimbare pagină / mute |
| — | **Cablu de extensie tip K** | de la sondă la MAX31856; **nu** cupru obișnuit |
| — | Cablu ecranat 3 fire (pentru Pt100), cablu ecranat 2 fire (semnale analogice) | |
| 1 | Cutie IP54+ pentru electronica din habitaclu, conectori auto (ex. Deutsch DT sau similari) | |
| 1 | Cablu USB **fără VBUS** sau adaptor "USB power blocker" | ⚠ evită ca ESP32 să fie alimentat din două surse |
| 1 | Adaptoare montaj senzori (vezi [04](04-montaj-senzori.md)) | |

## Alternative

- **Placă ESP32-S3 cu GC9A01 integrat.** Există plăci de tip "round display" cu ESP32-S3,
  dar pinii lor diferă de cei din `config.h` și `platformio.ini`. Pinii se pot schimba
  fără probleme.
- **Al doilea ADS1115** (adresa 0x49) dacă vrei măsurători diferențiale, mai imune la
  diferențele de masă, sau mai multe canale analogice, de exemplu presiune de ulei sau de combustibil.
