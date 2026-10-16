# LC100 Gauge: afișaj parametri motor pentru Toyota Land Cruiser 100 (1HD-FTE)

Dispozitiv care afișează și înregistrează parametrii motorului pe un Land Cruiser 100
(04/2002, 1HD-FTE). Motorul are turbo GTurbo HD-G30 (22 psi), injectoare +30 %,
intercooler frontal PDI și Unichip.

Totul rulează pe **un singur ESP32-S3**, fără Raspberry Pi:

```
 3 × Pt100 ─► 3 × MAX31865 ─┐
 EGT tip K ─► MAX31856  ────┤
 Auber, AEM analogic, 12 V ─┼─► ESP32-S3 ─┬─► 3 × display rotund GC9A01 (câte 2 valori pe fiecare)
 AEM RS232 ─► MAX3232 ──────┘             ├─► buzzer + buton
                                          ├─► log CSV pe microSD
                                          └─► Wi-Fi (la cerere): pagină web pe telefon,
                                              cu valori live, setări și descărcare log-uri
```

## Ce măsoară

| Canal | Senzor | Interfață |
|---|---|---|
| Temperatură apă | Pt100 | MAX31865 #1 |
| Temperatură ulei | Pt100 | MAX31865 #2 |
| Temperatură aer după intercooler | Pt100 | MAX31865 #3 |
| EGT pre-turbo | termocuplu tip K | **MAX31856** sau MAX31855K (MAX31865 nu poate citi termocupluri) |
| Boost | Auber, −0.3…3.3 bar relativ, 0.1–4.85 V ratiometric | ADS1115 A1 (+ ref. 5 V pe A3) |
| λ | AEM 30-0310: RS232 (prioritar) și ieșirea analogică | MAX3232 → UART și ADS1115 A0 |
| Tensiune baterie | divizor rezistiv | ADS1115 A2 |

## Structura proiectului

```
firmware/lc100-gauge/
  platformio.ini          pinii display-urilor, bibliotecile
  include/pins.h          ceilalți pini
  include/settings.h      valori implicite (praguri, calibrări), validare
  include/*.h             logică pură: conversii, alarme, parser AEM, CSV, afișare
  src/                    senzori, display-uri, log SD, server web, main
  test/test_native/       teste pe PC pentru logica pură
firmware/display-test/    test separat pentru cele 3 display-uri (refolosește codul de afișare)
docs/                     documentație (arhitectură, piese, cablaj, montaj, Wi-Fi, calibrare)
```

## Documentație

1. [Arhitectură](docs/01-arhitectura.md)
2. [Lista de componente](docs/02-lista-componente.md)
3. [Schema de cablaj](docs/03-cablaj.md)
4. [Montajul senzorilor pe motor](docs/04-montaj-senzori.md)
5. [Utilizare: display-uri, buton, Wi-Fi, log](docs/05-utilizare.md)
6. [Calibrare și testare](docs/06-calibrare-testare.md)
7. **[Lucruri de verificat înainte de punerea în funcțiune](docs/07-de-verificat.md)**
8. [Test pentru cele 3 display-uri](docs/08-test-display.md)

## Pornire rapidă

Necesită [PlatformIO](https://platformio.org/).

```bash
cd firmware/lc100-gauge
pio test -e native            # testele logicii, fără hardware
pio run -e esp32s3 -t upload  # compilare și încărcare pe ESP32-S3
```

Apoi ține butonul apăsat 3 s ca să pornești Wi-Fi-ul. Conectează-te de pe telefon la
rețeaua `LC100-Gauge` (parola implicită `landcruiser100`, **schimb-o** din pagină) și
deschide `http://192.168.4.1`.

## Stare

Codul **nu a fost încă testat pe hardware real și nu a fost compilat pentru ESP32**:
mediul în care a fost scris nu a avut acces la registrul PlatformIO. Ce s-a verificat:

- 18 teste pentru logica pură, toate trec;
- toate fișierele `.cpp` trec de compilator (g++ `-Wall -Wextra`), contra headerelor
  reale Adafruit MAX31865/MAX31856/MAX31855/ADS1X15 și ArduinoJson și a unor stub-uri care
  reproduc semnăturile din sursele TFT_eSPI și arduino-esp32;
- pagina web, într-un browser Chromium la lățime de telefon, cu un server simulat.

Vezi [docs/07-de-verificat.md](docs/07-de-verificat.md) pentru valorile care trebuie confirmate.
