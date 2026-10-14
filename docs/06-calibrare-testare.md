# 6. Calibrare și testare

Testează fiecare etapă pe banc înainte de montajul în mașină.

## 6.1 Pe banc, fără senzori

1. Încarcă firmware-ul și rulează `pio device monitor`. Trebuie să apară linia
   `{"hello":"esp32-gauges",...}` și cadre JSON la 10 Hz.
2. Fără senzori conectați, toate canalele trebuie să arate `null` cu fault `255`. Pe
   display apare `---`, iar pe dashboard „SENZOR LIPSĂ”. **Un senzor deconectat nu trebuie
   să afișeze niciodată o valoare plauzibilă.** Dacă vezi totuși valori, verifică pull-up-ul
   de pe MISO.
3. Pe CM5: `python -m gaugehub --port ...` și deschide dashboard-ul.

## 6.2 Pt100

- **0 °C:** gheață mărunțită cu apă, amestecate bine, cu sonda la mijloc. Citirea ar trebui
  să fie 0 °C. Abaterea acceptabilă depinde de clasa sondei (A sau B), vezi fișa ei.
- **Fierbere:** apa fierbe sub 100 °C la altitudine, deci folosește-o doar ca verificare
  grosieră.
- O abatere constantă mare înseamnă de obicei că `RTD_RREF` sau jumperii pentru 2/3/4 fire
  sunt greșiți.

## 6.3 EGT

- La temperatura camerei, EGT trebuie să arate aproximativ temperatura camerei.
- Dacă valoarea **scade** când încălzești sonda, polaritatea e inversată.
- Dacă primești fault (`f.egt` ≠ 0), verifică întâi continuitatea sondei și cablul de extensie.

## 6.4 Boost

- Cu motorul oprit, un senzor **relativ** trebuie să arate ~0, iar unul **absolut**
  ~1.0 bar. Dacă vezi ~14.7 psi la motor oprit, ai un senzor absolut: setează
  `BOOST_SENSOR_ABSOLUTE 1`.
- Compară cu un manometru mecanic (sau o pompă de mână cu manometru) la 2–3 presiuni
  cunoscute. Dacă panta diferă, corectează `BOOST_P_MAX_BAR` și `BOOST_V_MIN`/`BOOST_V_MAX`
  din fișa senzorului.

## 6.5 AFR / lambda

- Fă calibrarea în aer liber a senzorului AEM conform manualului AEM.
- Compară valoarea afișată de sistem cu valoarea controllerului AEM, citită din software-ul
  AEM sau pe CAN dacă îl ai. Dacă diferă, corectează `AFR_AT_0V` și `AFR_AT_5V` după
  manual. ⚠ Valorile implicite (8.5 la 0 V și 18.0 la 5 V) sunt **presupuneri**.

## 6.6 Tensiune baterie

- Măsoară cu multimetrul **în același punct** în care e legat divizorul și compară.
  Corectează `DIV_VBAT` cu raportul celor două valori.

## 6.7 Alarme

- Pentru test, scade temporar un prag (de exemplu `ALARM_ECT_WARN 30`). Verifică bipul
  scurt la avertizare, bipurile repetate și pagina forțată la critic, mute-ul cu apăsare
  lungă, plus chenarul și eticheta pe dashboard. Apoi readu pragul la valoarea normală.

## 6.8 Testele automate

```bash
cd firmware/esp32-gauges && pio test -e native   # conversii + alarme cu histerezis
cd cm5 && .venv/bin/python -m pytest tests        # parsare, log CSV, simulator
```
