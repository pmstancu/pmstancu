# 7. Lucruri de verificat înainte de punerea în funcțiune

Valorile de mai jos **nu le-am putut confirma** din surse primare. Codul folosește presupuneri
rezonabile, dar fiecare trebuie verificată.

| # | Ce | Unde se setează | Sursa de verificare |
|---|---|---|---|
| 1 | AEM 30-0310: scalarea ieșirii analogice. Implicit: AFR 8.5 la 0 V, 18.0 la 5 V (după un retailer) | pagina web „AFR la 0 V / 5 V” | manualul de instalare AEM 30-0310 |
| 2 | AEM 30-0310: formatul RS232. Implicit: 9600 baud, 8N1, câte o valoare text pe linie (din forumuri, nu din manual) | „Baud serial”, „Valoarea serială este” | manualul AEM + tab-ul **AEM** din pagină |
| 3 | AEM: culorile firelor (analog, masă de semnal, RS232) | — | manualul AEM |
| 4 | Senzorul Auber: dacă ieșirea e liniară între 0.1 V și 4.85 V | „Senzor presiune” | test cu pompă și manometru ([06](06-calibrare-testare.md)) |
| 5 | Rezistența de referință a plăcilor MAX31865 (430 Ω sau 400 Ω) | „RREF” | marcajul de pe rezistența plăcii |
| 6 | Jumperii pentru 3 fire pe plăcile MAX31865 | — | documentația plăcii |
| 7 | Pragurile EGT (650/720 °C pre-turbo) | „Praguri alarmă” | recomandarea GTurbo sau a unui specialist 1HD-FTE |
| 8 | Pragurile pentru apă, ulei, aer, boost și λ (λ e dezactivată implicit) | „Praguri alarmă” | manualul de service Toyota, experiența ta, log-urile |
| 9 | Filetele adaptoarelor pentru senzori | — | măsurare pe mașină |
| 10 | Pinii plăcii ESP32-S3 (mai ales pe ce pin e LED-ul RGB, 38 sau 48, și dacă placa are PSRAM octal care ocupă GPIO 35–37) | `pins.h`, `platformio.ini` | marcajul și schema plăcii tale |
| 11 | Modulele GC9A01: tensiunea pe VCC și pinul de lumină de fundal | — | vânzătorul / documentația modulului |
| 12 | Modulul microSD: dacă merge la 3.3 V sau are nevoie de 5 V | — | documentația modulului |
| 13 | Adresa paginii web (implicit 192.168.4.1) | — | setările Wi-Fi ale telefonului, după conectare |
| 14 | **Compilarea firmware-ului pentru ESP32** | — | `pio run -e esp32s3` (vezi mai jos) |

## Despre compilare

Mediul în care am scris codul **nu a avut acces** la registrul PlatformIO și nici la serverele
Espressif sau Arduino, așa că **firmware-ul nu a fost compilat pentru ESP32**. Ce am verificat:

- logica pură (`include/*.h`), cu 18 teste unitare, toate trec;
- toate fișierele `src/*.cpp` cu g++ (`-Wall -Wextra`), contra:
  - surselor reale Adafruit MAX31865 1.6.2, Adafruit MAX31856 1.2.8, Adafruit ADS1X15 2.6.2,
    Adafruit BusIO 1.17.4 și ArduinoJson 7.4.2, descărcate de pe GitHub;
  - unor stub-uri pentru Arduino/ESP32 și TFT_eSPI, cu semnăturile copiate din sursele
    arduino-esp32 (3.3.x) și TFT_eSPI (2.5.x);
- pagina web, în Chromium, cu un server simulat (fără erori JS, încape pe ecran de telefon).

La prima compilare reală pot apărea diferențe de versiune. Ce merită știut:

- **Platforma.** `platform = espressif32` aduce versiunea pe care o are registrul
  PlatformIO. ⚠ N-am putut verifica ce versiune de Arduino-ESP32 (2.x sau 3.x) aduce acum.
  Codul folosește doar API-uri prezente în ambele, din câte am verificat, dar nu e testat
  pe niciuna.
- **TFT_eSPI pe ESP32-S3.** Dacă dă erori la compilare sau se blochează la pornire, încearcă
  întâi să elimini `-DUSE_HSPI_PORT=1`, apoi să schimbi versiunea platformei.
- **Trei display-uri.** Inițializarea comună (toate CS-urile active în timpul `tft.init()`)
  e o tehnică folosită des, dar n-am testat-o pe hardware. Dacă un display rămâne negru,
  verifică întâi cablajul CS, apoi scade `SPI_FREQUENCY`.

După prima compilare reușită, fixează versiunile exacte în `platformio.ini`
(`platform = espressif32@x.y.z`, `lib_deps = ...@x.y.z`), ca build-urile ulterioare să fie
reproductibile.
