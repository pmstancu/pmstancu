# 3. Schema de cablaj

> Pinii se schimbă în `firmware/lc100-gauge/include/pins.h`. Pinii display-urilor (SCLK, MOSI,
> DC, RST) se schimbă în `platformio.ini`. Pinii de mai jos sunt pentru **ESP32-S3-DevKitC-1**.
> Nu sunt folosiți: 0, 3, 45, 46 (strapping), 19/20 (USB), 26–37 (flash/PSRAM), 43/44 (UART0),
> 38/48 (LED-ul RGB de pe placă, poziția depinde de revizie).

## 3.1 Alimentare

```
 Baterie +12V ──[siguranță 3A]── contact (IG/ACC) ──┬──► TVS la masă (cât mai aproape de intrare)
                                                    ├──► DC-DC 12→5V (≥2A) ──► ESP32 pin 5V
                                                    │                     └──► buzzer, modul SD (după caz)
                                                    ├──► LM2940-5 ──► +5V senzor Auber
                                                    │             └──► divizor ─► ADS A3
                                                    └──► divizor 100k/22k ─► ADS A2 (baterie)

 Masă: un singur punct comun pentru electronică, legat la caroserie/bloc cu cablu gros.
```

- **AEM 30-0310** se alimentează separat, cu 12 V comutat și masă, conform manualului AEM.
- Când programezi ESP32 pe USB cu mașina pornită, ESP32 primește 5 V din două surse.
  ⚠ Nu am verificat dacă DevKitC-1 are protecție pentru asta. Programează cu contactul oprit
  sau folosește un cablu USB fără VBUS.

## 3.2 Tabel de pini

| ESP32-S3 | Conectat la | Observații |
|---|---|---|
| 3V3 | VIN la MAX31865 ×3, MAX31856, ADS1115, MAX3232; VCC și BL la cele 3 GC9A01 | ⚠ verifică tensiunea acceptată de modulele tale |
| GND | masă comună | |
| GPIO12 | SCK (CLK) la toate MAX3186x | SPI software, senzori |
| GPIO11 | SDI (MOSI) la toate MAX3186x | |
| GPIO13 | SDO (MISO) la toate MAX3186x | + **pull-up 10 kΩ la 3V3** |
| GPIO10 | CS MAX31865 #1 (apă) | |
| GPIO14 | CS MAX31865 #2 (ulei) | |
| GPIO15 | CS MAX31865 #3 (aer după intercooler) | |
| GPIO16 | CS MAX31856 (EGT) | |
| GPIO8 | SDA ADS1115 | |
| GPIO9 | SCL ADS1115 | ADDR la GND → adresa 0x48 |
| GPIO4 | SCL/SCK la **toate 3** display-urile | |
| GPIO5 | SDA/MOSI la **toate 3** display-urile | |
| GPIO7 | DC la **toate 3** display-urile | |
| GPIO17 | RST/RES la **toate 3** display-urile | |
| GPIO6 | CS display 1 | |
| GPIO1 | CS display 2 | |
| GPIO2 | CS display 3 | |
| GPIO39 | SCK modul microSD | |
| GPIO40 | MOSI (DI) modul microSD | |
| GPIO41 | MISO (DO) modul microSD | |
| GPIO42 | CS modul microSD | |
| GPIO47 | R1OUT (RX, nivel TTL) la MAX3232 | ← R1IN primește TX RS232 de la AEM |
| GPIO21 | buton → GND | pull-up intern |
| GPIO18 | bază tranzistor buzzer (prin 1 kΩ) | buzzer la 5 V, emitorul la GND |

### Display-urile

- Pinul **BL/BLK** al fiecărui display merge la 3.3 V (lumina de fundal rămâne mereu aprinsă).
- **Firele cât mai scurte.** La 40 MHz, firele lungi spre trei display-uri pot produce
  imagini corupte. Dacă apar, scade `SPI_FREQUENCY` în `platformio.ini` la `20000000`
  sau `27000000`.
- Ordinea ecranelor (1, 2, 3) e cea din pagina de setări. Valorile de pe fiecare ecran se
  aleg de acolo.

## 3.3 Intrările analogice (ADS1115)

Totul de mai jos e valabil identic și pentru **ADS1015**, cu aceiași pini și aceeași adresă.
ADS1115 e alimentat la **3.3 V**, deci **nicio intrare nu are voie să depășească 3.3 V**.
Semnalele de 0–5 V trec printr-un divizor:

```
 semnal 0-5V ──[10k]──┬──────────► ADS Ax         5.0 V la semnal → 3.0 V la ADS
                      │
                    [15k]   ═╪═ 100nF            BAT54S între Ax, 3V3 și GND (limitare)
                      │      │
 GND ─────────────────┴──────┘

 +12V comutat ──[100k]──┬──► ADS A2              18.3 V la intrare → 3.3 V la ADS
                        │
                      [22k]  ═╪═ 100nF  + BAT54S
                        │     │
 GND ───────────────────┴─────┘
```

| ADS1115 | Semnal | Divizor | Setare în pagina web |
|---|---|---|---|
| A0 | AEM 30-0310, ieșire analogică | 10k/15k | „Factor semnale 5 V” |
| A1 | senzor Auber (semnal) | 10k/15k | „Factor semnale 5 V” |
| A2 | +12 V comutat | 100k/22k | „Factor baterie” |
| A3 | +5 V care alimentează senzorul Auber | 10k/15k | „Factor semnale 5 V” |

- Citirea de pe A3 permite **corecția ratiometrică**: dacă tensiunea de 5 V a senzorului
  variază, presiunea se corectează automat.
- Dacă folosești alte rezistențe, schimbă factorii din pagină:
  factor = (R_sus + R_jos) / R_jos.
- **Masa AEM:** leagă masa de semnal a AEM (culoarea firului e în manual) la masa ADS1115,
  nu la un punct îndepărtat de pe caroserie. Câțiva zeci de mV diferență de masă se văd
  direct în citirea λ.

## 3.4 AEM RS232 → MAX3232

```
 AEM fir RS232 TX ──► MAX3232 R1IN    MAX3232 R1OUT ──► GPIO47
 AEM masă         ──► MAX3232 GND     MAX3232 VCC   ──► 3V3
```

- Firul RS232 al AEM: ⚠ verifică culoarea în manual. Unele kituri au mufă DB9.
- Pe GPIO47 nu se leagă **niciodată** direct RS232. Nivelurile de ±V ar distruge pinul.

## 3.5 Senzorii de temperatură

**Pt100 → MAX31865, cu 3 fire:**
- Configurează jumperii plăcii pentru 3 fire conform ghidului producătorului. Clonele pot
  avea alt layout.
- Verifică rezistența de referință de pe placă: **430 Ω** (marcaj „431”) la Adafruit; unele
  clone au 400 Ω. Valoarea se pune în pagină, la „RREF”.
- Folosește cablu ecranat; leagă ecranul la masă doar la capătul dinspre electronică.

**Termocuplu tip K → MAX31856:**
- De la sondă până la bornele plăcii folosește **doar cablu de extensie tip K**. Orice
  îmbinare cu cupru creează un termocuplu parazit.
- Respectă polaritatea. Culorile diferă între standarde (ANSI: galben +, roșu −; IEC:
  verde +, alb −), deci verifică-le pe cablul tău.
- Placa MAX31856 măsoară temperatura joncțiunii reci. Montează placa în habitaclu, nu
  lângă motor.
