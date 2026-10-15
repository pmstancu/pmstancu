# 4. Montajul senzorilor pe motor

> ⚠ Filetele, adaptoarele și pozițiile exacte de pe 1HD-FTE **nu le-am putut verifica**.
> Ce urmează sunt practici generale. Confirmă dimensiunile pe mașina ta sau cu un atelier
> specializat pe Land Cruiser înainte să comanzi adaptoare.

## EGT pre-turbo (termocuplu tip K)

- **Poziție:** în galeria de evacuare sau în flanșa de intrare în turbină, înainte de
  turbină. Vârful sondei trebuie să ajungă în fluxul de gaze, aproximativ la centrul canalului.
- **Demontează galeria pentru găurire și filetare.** Șpanul rămas în galerie ajunge
  direct în turbină și o poate distruge.
- Montajul uzual este cu un fiting de compresie, adesea cu filet 1/8" NPT. Folosește
  fitingul livrat cu sonda.
- O sondă cu **joncțiune izolată** (ungrounded) evită problemele cu masa motorului. Cu o
  sondă cu joncțiunea la masă, MAX31856 poate raporta defecte sau citiri instabile.
- Temperatura pre-turbo e semnificativ **mai mare** decât cea post-turbo. Pragurile
  implicite (650/720 °C) presupun montaj pre-turbo și sunt **orientative**.

## Boost (senzor Auber)

- **Priza de presiune:** după intercooler, pe galeria de admisie. Ideal e un orificiu
  dedicat. Un teu pe furtunul unui senzor existent al ECU funcționează, dar orice
  neetanșeitate afectează și citirea ECU-ului.
- ⚠ **Unichip.** E posibil ca Unichip să intercepteze semnalul senzorului de presiune
  către ECU. Senzorul tău separat arată **presiunea reală**, care poate diferi de ce
  „vede” ECU-ul. Diferența e normală și utilă, nu e o eroare.
- Senzorul Auber are precizie de 1 % din scală (aproximativ ±0.04 bar) între 0 și 80 °C,
  dar doar 3 % în afara acestui interval (din fișa tehnică). **Montează-l departe de
  căldură**, cu racordul în jos, ca să nu se adune condens sau ulei. Îl poți ține în
  habitaclu, legat cu un furtun subțire; un furtun lung adaugă o mică întârziere.
- Senzorul e **relativ** (citește 0 la presiune atmosferică) și are domeniul −0.3…3.3 bar,
  deci acoperă cu rezervă cei 22 psi (~1.52 bar) ai turbinei.

## Temperatură apă (Pt100)

- Variantă fără găurire: un **adaptor inline pe furtunul superior** al radiatorului, cu
  port filetat pentru senzor. Alege diametrul după furtunul tău.
- Vârful sondei trebuie să stea în fluxul de lichid, nu într-un buzunar mort.

## Temperatură ulei (Pt100)

- Variantă uzuală: o **placă sandwich** între filtrul de ulei și bloc, cu port pentru senzor.
  ⚠ Plăcile sandwich se fac pentru un anumit filet și diametru de garnitură. Verifică
  filetul suportului de filtru de pe 1HD-FTE înainte de comandă.
- Alternativ: un bușon de baie cu port de senzor. Acesta citește temperatura din baie,
  care diferă de cea din galeria de ungere.

## Temperatură aer după intercooler (Pt100)

- Pe țeava de după intercooler, cât mai aproape de galeria de admisie.
- Un Pt100 cu teacă groasă reacționează **lent** la schimbările de temperatură ale aerului.
  Pentru aer, alege o sondă cu vârf subțire sau expus.

## AEM 30-0310 (wideband)

- Montează senzorul LSU 4.9 conform manualului AEM: de obicei după turbină, înclinat față
  de orizontală, ca să nu se adune condens. ⚠ Unghiul exact și distanța față de turbină
  sunt în manualul AEM.
- **Diesel:** AEM afișează AFR pe scara benzinei, iar sistemul îl transformă în **λ = AFR / 14.7**.
  La ralanti și în croazieră λ e mare și iese din domeniul ieșirii analogice. Valoarea
  importantă e cea **în sarcină**, unde λ scade și poate apărea fum.
- Senzorul nu trebuie lăsat în evacuare cu motorul pornit și controllerul oprit
  (încălzitorul oprit), pentru că se poate deteriora prin funingine sau condens. Vezi
  manualul AEM.

## Cablarea în compartimentul motor

- Folosește conectori auto etanși (de ex. Deutsch DT) la trecerea prin peretele despărțitor.
- Ține cablurile senzorilor departe de cablurile injectoarelor, de alternator și de
  demaror. Unde nu se poate, încrucișează-le la 90°.
- Folosește manșon termic lângă galeria de evacuare.
