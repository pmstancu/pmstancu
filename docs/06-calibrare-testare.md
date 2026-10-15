# 6. Calibrare și testare

Toate corecțiile de mai jos se fac din pagina web (secțiunea **Setări**), fără recompilare.

## 6.1 Pe PC, fără hardware

```bash
cd firmware/lc100-gauge
pio test -e native
```

Testele verifică:

- conversiile (Pt100, Auber, AEM);
- alarmele cu histerezis;
- parserul serial AEM;
- validarea și salvarea setărilor;
- formatul CSV;
- așezarea pe ecrane;
- numele fișierelor de log.

## 6.2 Pe bancă, înainte de mașină

Alimentează sistemul de la o sursă de laborator de 12–13.8 V.

1. **Pornire fără senzori.** Toate canalele trebuie să arate „---” / „SENZOR”, fără blocaje.
   Pagina web arată defectul pentru fiecare canal.
2. **Pt100:** în locul sondei pune o rezistență de precizie:
   - 100.0 Ω trebuie să dea aproximativ **0 °C**;
   - 138.5 Ω trebuie să dea aproximativ **100 °C** (tabel IEC 60751).

   Dacă ambele valori sunt deplasate cu aceeași cantitate, corectează cu „Corecție … (°C)”.
   Dacă eroarea crește odată cu temperatura, verifică RREF (430 Ω sau 400 Ω).
3. **EGT:** cu sonda la temperatura camerei, citirea trebuie să fie apropiată de temperatura
   camerei. Încălzește vârful (de exemplu cu o brichetă) și verifică că valoarea crește.
   Dacă scade, polaritatea e inversată.
4. **Tensiunea bateriei:** compară cu un multimetru. Dacă diferă, noul factor este
   `factor_nou = factor_vechi × V_multimetru / V_afișat`.
5. **Referința de 5 V:** compară `ref. 5 V` din pagină cu multimetrul pe ieșirea LM2940.
   Corectează „Factor semnale 5 V” cu aceeași formulă. Acest factor afectează și boost-ul,
   și λ analogic.
6. **Boost:**
   - **la presiune atmosferică,** senzorul dă aproximativ 0.50 V (0.1 + 0.3/3.6 × 4.75),
     iar afișajul trebuie să arate aproximativ 0. Corectează mica diferență cu
     „Corecție zero (bar)”;
   - **liniaritatea:** cu o pompă de mână și un manometru de referință, verifică mai multe
     puncte, de exemplu 0.5, 1.0 și 1.5 bar. Fișa nu spune explicit că ieșirea e liniară,
     deci acest test e important.
7. **AEM:**
   - în tab-ul **AEM** din pagină trebuie să apară octeți. Dacă nu apare nimic, verifică
     cablajul MAX3232, viteza (baud) și dacă AEM are ieșirea serială activă;
   - textul primit (de ex. `14.7\r\n`) arată dacă AEM trimite AFR sau λ. Alege tipul în
     „Valoarea serială este”;
   - compară valoarea serială cu cea analogică într-o zonă unde amândouă sunt valide
     (λ < 1.2).
8. **Buzzer și buton:** coboară temporar un prag sub valoarea curentă (de ex. pragul critic
   la apă la 20 °C), verifică buzzerul și oprirea lui cu o apăsare scurtă, apoi revino la
   valorile inițiale.
9. **Card SD:** scoate cardul în timpul funcționării. Pe ecran trebuie să apară „SD!”, iar
   după ce îl pui la loc se creează un fișier nou în maximum 10 s.

## 6.3 În mașină

- Prima pornire cu motorul la ralanti: temperaturile trebuie să crească treptat, iar
  boost-ul trebuie să fie aproximativ 0.
- Compară temperatura apei cu indicatorul de bord. Indicatorul de bord e de obicei
  „amortizat”, deci compară doar tendința.
- Fă un drum de test cu log-ul pornit și verifică fișierul CSV pe PC.
- Pragurile de alarmă implicite sunt doar un punct de plecare. Ajustează-le după log-urile
  tale și după recomandările unui specialist 1HD-FTE.
