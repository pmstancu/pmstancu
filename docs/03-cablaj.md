# 3. Schema de cablaj

> Toți pinii se pot schimba în `firmware/esp32-gauges/include/config.h`. Pinii display-ului
> se schimbă în `platformio.ini`. Pinii de mai jos sunt pentru **ESP32-S3-DevKitC-1**.
> Pinii 0, 3, 45 și 46 (strapping) și 19/20 (USB) nu sunt folosiți.

## 3.1 Alimentare

```
 Baterie +12V ──[siguranță 3A]──┬── contact (IG/ACC) ──┬──► Convertor DC-DC 12→5V ──► ESP32 pin 5V
                                │                      │          (≥2A)
                                │                      ├──► TVS la masă (cât mai aproape de intrare)
                                │                      ├──► Regulator 5V low-noise ──► +5V senzor Auber
                                │                      │                          └──► divizor ─► ADS A3
                                │                      └──► divizor 100k/22k ─► ADS A2 (tensiune baterie)
                                │
                                └── M2-ATX-HV (alimentare CM5, intrare separată, conform manualului lui)

 Masă: un singur punct comun de masă pentru electronică, legat la caroserie/bloc cu cablu gros.
```

- **AEM 30-0310** se alimentează cu 12 V comutat și masă, conform manualului AEM.
- **USB între ESP32 și CM5:** ESP32 are alimentarea lui, deci folosește un cablu sau
  adaptor care **întrerupe VBUS (5 V)** și păstrează doar datele și masa. Altfel două surse
  de 5 V ajung legate împreună. ⚠ Nu am verificat dacă DevKitC-1 are diodă de protecție
  pe portul USB nativ, așa că tratează conexiunea ca neprotejată.

## 3.2 ESP32-S3: tabel de pini

| ESP32-S3 | Conectat la | Observații |
|---|---|---|
| 3V3 | VIN pe MAX31865 ×3, MAX31856, ADS1115 | toate plăcile senzorilor la 3.3 V |
| GND | GND comun | |
| GPIO12 | SCK (CLK) pe toate MAX3186x | SPI software, senzori |
| GPIO11 | SDI (MOSI) pe toate MAX3186x | |
| GPIO13 | SDO (MISO) pe toate MAX3186x | + **10 kΩ pull-up la 3V3** |
| GPIO10 | CS MAX31865 #1 (apă) | |
| GPIO14 | CS MAX31865 #2 (ulei) | |
| GPIO15 | CS MAX31865 #3 (aer intercooler) | |
| GPIO16 | CS MAX31856 (EGT) | |
| GPIO8 | SDA ADS1115 | |
| GPIO9 | SCL ADS1115 | ADDR pe ADS1115 legat la GND (adresa 0x48) |
| GPIO4 | SCL/SCK display GC9A01 | |
| GPIO5 | SDA/MOSI display | |
| GPIO6 | CS display | |
| GPIO7 | DC display | |
| GPIO17 | RST display | |
| GPIO18 | BL (backlight) display | |
| GPIO1 | buton → GND | pull-up intern |
| GPIO21 | bază tranzistor buzzer (prin 1 kΩ) | buzzer la 5 V, emitor la GND |
| USB (port marcat "USB", nu "UART") | CM5 | apare ca `/dev/ttyACM0` |

## 3.3 Intrările analogice (ADS1115)

ADS1115 e alimentat la **3.3 V**, deci **nicio intrare nu are voie să depășească 3.3 V**.
Semnalele de 0–5 V trec printr-un divizor:

```
 semnal 0-5V ──[10k]──┬──────────► ADS Ax         5.0 V la semnal → 3.0 V la ADS
                      │
                    [15k]   ═╪═ 100nF            BAT54S între Ax, 3V3 și GND (clamp)
                      │      │
 GND ─────────────────┴──────┘

 +12V comutat ──[100k]──┬──► ADS A2              18.3 V la intrare → 3.3 V la ADS
                        │
                      [22k]  ═╪═ 100nF  + BAT54S clamp
                        │     │
 GND ───────────────────┴─────┘
```

| ADS1115 | Semnal | Divizor | Constantă în `config.h` |
|---|---|---|---|
| A0 | AEM 30-0310 ieșire analogică | 10k/15k | `DIV_5V_SIGNALS` |
| A1 | Senzor presiune Auber (semnal) | 10k/15k | `DIV_5V_SIGNALS` |
| A2 | +12 V comutat | 100k/22k | `DIV_VBAT` |
| A3 | +5 V care alimentează senzorul Auber | 10k/15k | `DIV_5V_SIGNALS` |

- Măsurarea de pe A3 permite **corecția ratiometrică**: dacă cei 5 V ai senzorului variază,
  citirea de presiune se corectează automat.
- Dacă folosești alte rezistențe, actualizează factorii din `config.h`
  (factor = (R_sus + R_jos) / R_jos).
- **Masa AEM:** leagă masa de semnal/analogică a AEM (vezi manualul pentru culoarea firului)
  la masa ADS1115, nu la un punct de caroserie aflat departe. O diferență de masă de câțiva zeci
  de mV se vede direct în citirea AFR.

## 3.4 Senzori de temperatură

**Pt100 → MAX31865, cu 3 fire:**
- Configurează jumperii plăcii pentru 3 fire conform ghidului producătorului plăcii.
  Fiecare clonă poate avea alt layout. Pune `RTD_WIRES` la 3 în `config.h`.
- Verifică rezistența de referință de pe placă: **430 Ω** (marcaj "431") la Adafruit,
  iar unele clone au 400 Ω. Pune valoarea în `RTD_RREF`.
- Folosește cablu ecranat și leagă ecranul la masă doar la capătul dinspre electronică.

**Termocuplu tip K → MAX31856:**
- De la sondă până la bornele plăcii folosește **doar cablu de extensie tip K**. Orice
  îmbinare cu cupru creează un termocuplu parazit.
- Respectă polaritatea. Culorile diferă între standarde (ANSI: galben +, roșu −; IEC:
  verde +, alb −), așa că verifică-le pe cablul tău.
- Placa MAX31856 măsoară temperatura joncțiunii reci. Montează placa în habitaclu, nu
  lângă motor sau evacuare.
