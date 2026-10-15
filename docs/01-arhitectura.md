# 1. Arhitectură

## De ce un ESP32-S3 și nu Raspberry Pi sau Arduino

| | ESP32-S3 | Raspberry Pi 4B/5 | Arduino Uno/Nano |
|---|---|---|---|
| Pornire | ~1 s | zeci de secunde (Linux) | instant |
| Tăierea contactului | fără risc (nu are sistem de operare pe card) | risc de corupere a cardului SD | fără risc |
| RAM pentru display 240×240 | suficient | suficient | **insuficient** (2 KB) |
| Wi-Fi | da | da | nu |
| Consum | mic | mare (Pi 5 are nevoie de răcire) | mic |

Toate cifrele sunt aproximative. Pentru 3 display-uri mici, o pagină web și un log CSV,
un ESP32-S3 e suficient, iar un Pi ar complica alimentarea și oprirea.

## Schema

```
                 ┌──────────────────────────── ESP32-S3 ─────────────────────────────┐
 Pt100 apă  ─────┤ MAX31865 #1 ┐                                                     │
 Pt100 ulei ─────┤ MAX31865 #2 ├─ SPI software ─► task „temps” (core 0, ~4 Hz)       │
 Pt100 aer  ─────┤ MAX31865 #3 │                       │                             │
 Tip K EGT  ─────┤ MAX31856    ┘                       ▼                             │
 AEM 0-5 V ─div──┤                               task „fast” (core 0, ~40 Hz):       │
 Auber ────div───┤ ADS1115 ── I2C ─────────────► conversii, filtrare, alegere λ,     │
 12 V ─────div───┤                               alarme cu histerezis ─► Snapshot     │
 5 V ref ──div───┤                                     ▲                    │         │
 AEM RS232 ─MAX3232─ UART1 RX ─────────────────────────┘                    │         │
                 │                                                          ▼         │
                 │  loop() pe core 1: 3 × GC9A01 (HSPI) · buton · buzzer             │
                 │                    log CSV pe microSD (FSPI) · server web (Wi-Fi AP)│
                 └───────────────────────────────────────────────────────────────────┘
```

## Decizii

- **Trei magistrale separate.** Senzorii sunt pe un SPI software, display-urile pe HSPI și
  cardul SD pe FSPI. Așa nu apar conflicte de mod sau de viteză SPI, iar un card lent nu
  blochează desenarea.
- **Citirile lente nu blochează afișajul.** O citire MAX31865 durează aproximativ 75 ms
  (biblioteca Adafruit pornește curentul de polarizare, așteaptă, apoi convertește).
  Senzorii rulează pe core 0, iar display-urile și serverul web pe core 1.
- **Alarmele se calculează într-un singur loc** (task-ul „fast”). Display-urile, buzzerul,
  pagina web și log-ul primesc același nivel de alarmă, deci nu se pot contrazice.
- **Trei display-uri pe același SPI.** SCK, MOSI, DC și RST sunt comune, iar fiecare
  display are pinul lui CS, comandat din cod (`TFT_CS=-1` în TFT_eSPI). Fiecare ecran se
  desenează într-un buffer (sprite) și se trimite doar dacă s-a schimbat ceva pe el.
  Un cadru complet 240×240 la 40 MHz durează aproximativ 23 ms (calculat, nemăsurat).
- **λ: serialul are prioritate.** Ieșirea analogică AEM acoperă probabil doar λ ≈ 0.58–1.23
  (după un retailer, de confirmat în manual), iar un diesel merge cu λ mai mare la ralanti
  și în croazieră. De aceea se folosește serialul cât timp vin date (cel mult 1 s vechime).
  Altfel se folosește ieșirea analogică; la capătul scalei ecranul arată „>1.23”.
- **Setările se salvează în NVS** (memoria flash a ESP32), ca text JSON, și se validează
  înainte de salvare. Dacă datele salvate sunt corupte, se folosesc valorile implicite.
- **Wi-Fi-ul este oprit implicit.** Se pornește la cerere și se oprește singur după 10 minute
  fără activitate (configurabil). Așa se reduc consumul și interferențele.

## Ritmuri

| Ce | Frecvență aproximativă |
|---|---|
| ADS1115 (4 canale), serial AEM, alarme | ~40 Hz |
| Fiecare Pt100 + EGT | ~4 Hz |
| Verificare display-uri | 20 Hz (se redesenează doar ce s-a schimbat) |
| Log CSV | 10 Hz implicit (configurabil 50–10000 ms) |

## Filtrare

Medie exponențială: boost și λ analogic α = 0.35, baterie α = 0.15, temperaturi α = 0.5.
O citire invalidă (fir rupt, cip absent) nu e filtrată: valoarea devine imediat „---”.
