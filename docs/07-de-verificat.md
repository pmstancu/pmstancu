# 7. Lucruri de verificat înainte de punerea în funcțiune

Valorile de mai jos **nu le-am putut confirma** din surse primare. Codul folosește presupuneri
rezonabile, dar fiecare trebuie verificată.

| # | Ce | Unde se setează | Sursa de verificare |
|---|---|---|---|
| 1 | Senzorul Auber: domeniul de presiune, tensiunea de ieșire (0.5–4.5 V?), alimentarea (5 V?), absolut sau relativ, culorile firelor | `BOOST_*` în `config.h` | fișa tehnică Auber (pagina produsului nu s-a putut accesa din mediul meu) |
| 2 | AEM 30-0310: scalarea ieșirii analogice (AFR la 0 V și la 5 V), domeniul lambda, culorile firelor | `AFR_AT_0V`, `AFR_AT_5V` | manualul de instalare AEM 30-0310 (am găsit date contradictorii la retaileri) |
| 3 | Rezistența de referință a plăcilor MAX31865 (430 Ω sau 400 Ω) | `RTD_RREF` | marcajul de pe rezistența plăcii |
| 4 | Pragurile EGT pre-turbo (650/720 °C implicit) | `ALARM_EGT_*` | recomandarea GTurbo sau a unui specialist 1HD-FTE; valorile sunt orientative, nu oficiale |
| 5 | Pragurile de apă, ulei și aer | `ALARM_*` | manualul de service Toyota / experiența ta |
| 6 | Filetele pentru adaptoarele de senzori (filtru ulei, furtun radiator, galerie) | — | măsurare pe mașină |
| 7 | Modul de întârziere al M2-ATX-HV | jumperi pe M2-ATX-HV | manualul Mini-Box |
| 8 | Dacă ESP32-S3-DevKitC-1 are protecție la alimentare dublă USB + 5 V | — | schema plăcii Espressif (până atunci, cablu fără VBUS) |
| 9 | Compilarea firmware-ului pentru ESP32 | — | `pio run -e esp32s3` (vezi mai jos) |

## Despre compilare

Mediul în care am scris codul nu a putut descărca platformele PlatformIO, așa că
**firmware-ul nu a fost compilat pentru ESP32**. Ce am verificat:

- logica de conversie și alarme (`conversions.h`) cu teste unitare (9 teste, toate trec);
- sintaxa tuturor fișierelor `.cpp`, compilate cu g++ contra unor stub-uri care reproduc
  semnăturile din sursele oficiale ale bibliotecilor Adafruit MAX31865, Adafruit MAX31856,
  Adafruit ADS1X15 și TFT_eSPI.

La prima compilare reală pot apărea diferențe de versiune la biblioteci. Dacă TFT_eSPI
dă erori sau crash pe ESP32-S3, primul lucru de încercat este să elimini `-DUSE_HSPI_PORT=1`
din `platformio.ini` sau să fixezi o altă versiune a platformei `espressif32`.

După prima compilare reușită, fixează versiunile bibliotecilor în `platformio.ini`
(`lib_deps = ...@x.y.z`), ca build-urile ulterioare să fie reproductibile.
