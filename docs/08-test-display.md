# 8. Test pentru cele 3 display-uri

Proiectul `firmware/display-test/` verifică partea cea mai nesigură din sistem: cele trei
GC9A01 pe aceeași magistrală SPI, fiecare cu CS separat. Folosește **exact codul de
afișare și pinii** din firmware-ul principal, deci ce cablezi acum rămâne valabil.

## Ce ai nevoie

- ESP32-S3-DevKitC-1 și cablu USB-C;
- 1–3 display-uri GC9A01;
- opțional, un buton între GPIO21 și GND, pentru trecerea la etapa următoare.

Fără senzori, card SD sau alimentare de 12 V: totul merge de pe USB.

## Cablaj

| ESP32-S3 | Display-uri | Observații |
|---|---|---|
| 3V3 | VCC și BL/BLK la toate | ⚠ verifică tensiunea acceptată de modulul tău |
| GND | GND la toate | |
| GPIO4 | SCL/SCK la toate | |
| GPIO5 | SDA/MOSI la toate | |
| GPIO7 | DC la toate | |
| GPIO17 | RST/RES la toate | |
| GPIO6 | CS display 1 | |
| GPIO1 | CS display 2 | |
| GPIO2 | CS display 3 | |

- **Firele cât mai scurte**, ideal sub ~15 cm (o recomandare generală, nu o limită măsurată).
- Merge și cu 1 sau 2 display-uri: codul trimite date și spre CS-urile nefolosite, fără efect.

## Compilare și încărcare

```bash
cd firmware/display-test
pio run -e test40 -t upload     # SPI la 40 MHz (ca firmware-ul principal)
pio device monitor              # rezultatele pe serial
```

Pentru alte frecvențe: `-e test27` (27 MHz) sau `-e test20` (20 MHz).

La pornire, pe serial apare cât buffer s-a alocat pentru ecrane: **16 biți** (normal),
**8 biți** (memorie puțină, culori mai puține) sau **0** (alocarea a eșuat).

## Etapele

Se schimbă singure (8 s, 30 s, 10 s) sau la apăsarea butonului, apoi se reiau.

### 1. Identificare (8 s)

| Ecran | Culoare | Text |
|---|---|---|
| 1 | roșu | ECRAN 1, CS = GPIO6 |
| 2 | verde | ECRAN 2, CS = GPIO1 |
| 3 | albastru | ECRAN 3, CS = GPIO2 |

**Ce verifici:** fiecare ecran are culoarea și numărul lui. Dacă două ecrane arată la fel,
CS-urile lor sunt inversate sau legate împreună. Dacă un ecran e alb sau negru, citește
secțiunea „Probleme” de mai jos.

### 2. Gauge-uri (30 s)

Desenarea reală a firmware-ului principal, cu valori simulate care urcă și coboară:

| Ecran | Arc | Secundar |
|---|---|---|
| 1 | EGT | Aer după intercooler |
| 2 | Boost | λ |
| 3 | Apă | Ulei |

**Ce verifici:**
- arcurile se umplu lin;
- culorile trec din verde în portocaliu și roșu;
- la alarmă critică, fundalul clipește roșu;
- între secunda 20 și 25, uleiul (ecranul 3, jos) apare „---” (defect simulat);
- bateria nu e afișată pe niciun ecran, deci când ajunge la nivel critic apare temporar
  pe ecranul 3, jos.

### 3. Viteză (10 s)

Toate ecranele se redesenează continuu. Pe serial apar, la fiecare 2 s, rânduri ca:

```
3 ecrane: 14.2 cadre/s, 70.4 ms pe cadru complet (max 73.1 ms)
```

Cifrele de mai sus sunt doar un exemplu de format, nu un rezultat măsurat. Din calcul, un
ecran la 40 MHz ar trebui să ia aproximativ 23 ms, deci trei ecrane aproximativ 70 ms.
În funcționarea normală se redesenează doar ecranele care s-au schimbat, deci de obicei
mai puțin.

**Trimite-mi rândurile de pe serial pentru 40, 27 și 20 MHz.** Din ele aleg frecvența finală.

## Probleme

| Simptom | Ce încerci |
|---|---|
| Toate ecranele albe sau negre | verifică VCC, GND, SCK, MOSI, DC, RST; apoi `-e test20` |
| Un singur ecran nu merge | CS-ul lui și firele lui; mută-l pe alt CS ca să vezi dacă e ecranul sau pinul |
| Imagine coruptă, linii, zgomot | frecvență mai mică (`test27`, `test20`), fire mai scurte |
| Culori inversate (negru în loc de alb) | ⚠ unele module cer inversarea culorilor; spune-mi și adaug opțiunea |
| Imagine rotită sau în oglindă | spune-mi; se schimbă rotația din cod |
| Nu compilează / se blochează la pornire | scoate `-DUSE_HSPI_PORT=1` din `platformio.ini`; trimite-mi eroarea |
| Pe serial nu apare nimic | DevKitC-1 are două porturi USB-C; mesajele merg pe portul marcat **USB** (nu „UART”), pentru că firmware-ul folosește USB-ul nativ. Repornește placa cu monitorul deschis |

## După test

Când display-urile merg, urmează prototipul cu senzorii (vezi [06](06-calibrare-testare.md)).
Apoi, după ce prototipul confirmă pinii, frecvența SPI și formatul AEM, urmează PCB-urile
(vezi [07](07-de-verificat.md)).
