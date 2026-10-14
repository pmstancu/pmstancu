# 1. Arhitectură

## Principiu: două niveluri

```
                       ┌──────────────────────────────── ESP32-S3 ────────────────────────────────┐
 Pt100 apă   ──────────┤ MAX31865 #1 ┐                                                           │
 Pt100 ulei  ──────────┤ MAX31865 #2 ├─ SPI software (pini 11/12/13) ─► task "temps" (core 0)    │
 Pt100 aer   ──────────┤ MAX31865 #3 │                                                           │
 Tip K EGT   ──────────┤ MAX31856    ┘                                         │                 │
 AEM AFR 0-5V ─ divizor┤                                                       ▼                 │
 Auber boost ─ divizor ┤ ADS1115 ── I2C (pini 8/9) ─► task "analog" (core 0) ─► Snapshot +      │
 12V baterie ─ divizor ┤                                                         alarme cu      │
 5V referință ─ divizor┤                                                         histerezis     │
                       │                                                       │                 │
                       │    loop() (core 1): display GC9A01 20 fps, buzzer, buton, JSON 10 Hz    │
                       └───────────────────────────────────────────────────────┬─────────────────┘
                                                                               │ USB (CDC serial)
                       ┌──────────────────────────── CM5 ─────────────────────▼─────────────────┐
                       │  gaugehub: citire serial ─► log CSV zilnic                             │
                       │                         └─► server web ─► Chromium kiosk pe 7"        │
                       │                                        └─► (opțional) telefon pe Wi-Fi │
                       │  + alte aplicații: SDR, GPS etc.                                       │
                       └────────────────────────────────────────────────────────────────────────┘
```

## De ce așa

- **ESP32 este sursa de adevăr pentru alarme.** Pragurile sunt în `config.h`, iar nivelul
  de alarmă (0 = OK, 1 = avertizare, 2 = critic) se trimite către CM5. Dashboard-ul
  nu recalculează alarmele, deci cele două afișaje nu pot ajunge să se contrazică.
- **Pornire instant, fără risc la tăierea curentului.** ESP32 nu are sistem de fișiere de
  corupt. CM5 poate porni mai încet sau poate fi oprit fără ca indicațiile critice să dispară.
- **Senzorii au magistrala lor.** Termocuplul și Pt100 sunt citiți pe un SPI software
  separat de display, deci nu apar conflicte de mod SPI sau de viteză.
- **Citirile lente nu blochează afișajul.** O conversie MAX31865 durează aproximativ
  75 ms (din codul bibliotecii Adafruit). Rulează pe core 0, iar display-ul pe core 1.

## Ritmuri

| Ce | Frecvență aproximativă |
|---|---|
| ADS1115 (boost, AFR, baterie) | ~40 Hz, filtrat EMA |
| Fiecare Pt100 | ~4 Hz |
| EGT (MAX31856 în mod continuu) | ~12 Hz citire; cipul face conversii la aproximativ 10 Hz (de verificat în fișa tehnică) |
| Display GC9A01 | ~20 fps |
| Telemetrie către CM5 | 10 Hz |

## Protocolul serial (ESP32 → CM5)

Câte un obiect JSON pe linie, la 115200 baud (pe USB nativ viteza nu contează):

```json
{"t":123456,"v":{"ect":88.1,"eot":95.0,"iat":41.2,"egt":612.0,"boost":1.48,"afr":19.6,"vbat":14.1},
 "l":{"ect":0,"eot":0,"iat":0,"egt":0,"boost":0,"afr":0,"vbat":0},
 "f":{"ect":0,"eot":0,"iat":0,"egt":0,"boost":0,"afr":0,"vbat":0}}
{"hello":"esp32-gauges","fw":"0.1.0"}
```

- `v`: valori. Temperaturile sunt în °C, `boost` în bar relativ, `afr` pe scara benzinei (λ = AFR / 14.7), `vbat` în V. `null` înseamnă senzor indisponibil.
- `l`: nivel de alarmă.
- `f`: cod de defect. 0 = OK, 255 = cip/senzor absent; orice altă valoare este registrul de defecte al MAX3186x.

## Display-ul rotund (GC9A01)

Butonul scurt trece prin paginile EGT → Boost → Temperaturi → Lambda/Baterie. Un canal
în alarmă critică forțează afișarea paginii lui și pornește buzzerul. Apăsarea lungă
(peste 1.5 s) oprește buzzerul până când alarmele dispar.
