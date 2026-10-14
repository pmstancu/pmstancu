# LC100 Gauges: vizualizare parametri motor pentru Toyota Land Cruiser 100 (1HD-FTE)

Sistem de afișare și înregistrare pentru temperaturi, presiunea turbo, AFR/lambda și
tensiunea bateriei pe un Land Cruiser 100 (04/2002, 1HD-FTE) cu turbo GTurbo HD-G30,
injectoare +30 %, intercooler frontal PDI și Unichip.

```
 senzori ──► ESP32-S3 (achiziție + alarme + display rotund GC9A01) ──USB──► CM5 (dashboard 7", log CSV, SDR/GPS)
```

ESP32 funcționează singur: pornește instant, afișează valorile și declanșează alarmele
chiar dacă CM5 este oprit sau încă bootează. CM5 adaugă ecranul mare, graficele și
înregistrarea datelor.

## Ce măsoară

| Canal | Senzor | Interfață |
|---|---|---|
| Temperatură apă (lichid răcire) | Pt100 | MAX31865 #1 |
| Temperatură ulei | Pt100 | MAX31865 #2 |
| Temperatură aer după intercooler | Pt100 | MAX31865 #3 |
| EGT pre-turbo | Termocuplu tip K | **MAX31856** (nu MAX31865) |
| Boost | Auber (0–5 V, de confirmat) | ADS1115 A1 |
| AFR / lambda | AEM 30-0310 (ieșire analogică) | ADS1115 A0 |
| Tensiune baterie | divizor rezistiv | ADS1115 A2 |
| Referință 5 V senzor presiune | divizor rezistiv | ADS1115 A3 |

## Structura proiectului

```
firmware/esp32-gauges/   Firmware PlatformIO (Arduino) pentru ESP32-S3
  include/config.h       ← toate setările: pini, scalări senzori, praguri alarmă
cm5/                     Aplicația Python pentru CM5 (cititor serial, log CSV, dashboard web)
  systemd/               Serviciu systemd + autostart kiosk
docs/                    Documentație (arhitectură, componente, cablaj, montaj, instalare, calibrare)
```

## Documentație

1. [Arhitectură](docs/01-arhitectura.md)
2. [Lista de componente](docs/02-lista-componente.md)
3. [Schema de cablaj](docs/03-cablaj.md)
4. [Montajul senzorilor pe motor](docs/04-montaj-senzori.md)
5. [Instalare pe CM5](docs/05-cm5-instalare.md)
6. [Calibrare și testare](docs/06-calibrare-testare.md)
7. **[Lucruri de verificat înainte de punerea în funcțiune](docs/07-de-verificat.md)**

## Pornire rapidă

Firmware (necesită [PlatformIO](https://platformio.org/)):

```bash
cd firmware/esp32-gauges
pio test -e native          # teste conversii, fără hardware
pio run -e esp32s3 -t upload
pio device monitor          # vezi liniile JSON
```

CM5 / PC, fără hardware (date simulate):

```bash
cd cm5
python3 -m venv .venv && .venv/bin/pip install -r requirements.txt
.venv/bin/python -m gaugehub --sim --no-log
# deschide http://127.0.0.1:8080
.venv/bin/pip install pytest && .venv/bin/python -m pytest tests
```

## Stare

Codul este o primă versiune și **nu a fost încă testat pe hardware real**. Vezi
[docs/07-de-verificat.md](docs/07-de-verificat.md) pentru valorile care trebuie confirmate
din fișele tehnice (senzorul Auber, scalarea AEM, pragurile de alarmă).
