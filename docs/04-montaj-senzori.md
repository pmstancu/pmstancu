# 4. Montajul senzorilor pe motor

> ⚠ Filetele, adaptoarele și pozițiile exacte de pe 1HD-FTE **nu le-am putut verifica**.
> Ce urmează sunt practici generale. Confirmă dimensiunile pe mașina ta sau cu un atelier
> specializat pe Land Cruiser înainte să comanzi adaptoare.

## EGT pre-turbo (termocuplu tip K)

- **Poziție:** în galeria de evacuare sau în flanșa de intrare în turbină, înainte de
  turbină. Vârful sondei trebuie să ajungă în fluxul de gaze, cam la centrul canalului.
- **Demontează galeria pentru găurire și filetare.** Șpanul care cade în galerie ajunge
  direct în turbină și o poate distruge. Nu găuri cu galeria montată.
- Montajul uzual este cu un fiting de compresie (adesea filet 1/8" NPT). Folosește
  fitingul livrat cu sonda ta.
- O sondă cu **joncțiune izolată** (ungrounded) evită problemele cu masa motorului.
  Cu o sondă cu joncțiune la masă, MAX31856 poate raporta defecte sau citiri instabile.
- Valoarea pre-turbo este semnificativ **mai mare** decât cea post-turbo. Pragurile din
  `config.h` presupun montaj pre-turbo.

## Boost (senzor Auber)

- **Priză de presiune:** după intercooler, pe galeria de admisie. Ideal folosești un
  orificiu dedicat. Un teu pe furtunul unui senzor existent al ECU funcționează, dar
  orice neetanșeitate afectează și citirea ECU-ului.
- ⚠ **Unichip.** Este posibil ca Unichip să intercepteze semnalul senzorului de presiune
  către ECU. Senzorul tău separat va arăta **presiunea reală**, care poate diferi de ce
  „vede” ECU-ul. Este un lucru util de știut, nu o eroare.
- Montează senzorul departe de surse de căldură și cu racordul în jos, ca să nu se adune
  condens sau ulei în el. Poți ține senzorul în habitaclu cu un furtun subțire, dar
  furtunul lung adaugă o mică întârziere.
- Turbina e setată la 22 psi (~1.52 bar). Alarma implicită este la 23 psi (avertizare) și
  25 psi (critic).

## Temperatură apă (Pt100)

- Variantă fără găurire: un **adaptor inline pe furtunul superior** al radiatorului, cu
  port filetat pentru senzor. Alege diametrul după furtunul tău.
- Vârful sondei trebuie să fie în fluxul de lichid, nu într-un buzunar mort.

## Temperatură ulei (Pt100)

- Variantă uzuală: o **placă sandwich** între filtrul de ulei și bloc, cu port pentru senzor.
  ⚠ Plăcile sandwich vin pentru un anumit filet și diametru de garnitură. Verifică filetul
  suportului de filtru de pe 1HD-FTE înainte de comandă.
- Alternativ: un buşon de baie de ulei cu port de senzor. Acesta citește temperatura din
  baie, care e diferită de cea din galeria de ungere.

## Temperatură aer după intercooler (Pt100)

- Pe țeava de după intercooler, cât mai aproape de galeria de admisie.
- Pt100 cu teacă groasă reacționează **lent** la schimbările de temperatură ale aerului.
  Pentru aer, alege o sondă cu vârf subțire sau expus.

## AEM 30-0310 (wideband)

- Montează senzorul LSU 4.9 conform manualului AEM: de obicei după turbină, înclinat cu
  cel puțin ~10° față de orizontală, ca să nu se adune condens. ⚠ Unghiul exact și distanța
  față de turbină sunt în manualul AEM.
- **Diesel:** controllerul AEM afișează AFR pe scara benzinei. Pentru diesel urmărește
  **lambda** (afișată de sistem ca AFR / 14.7). La ralanti și în croazieră lambda poate
  ieși din domeniul controllerului. Valoarea relevantă este cea **în sarcină**, unde
  lambda scade și apare fumul.
- Funcționarea cu încălzitorul senzorului oprit, cu motorul pornit, poate deteriora
  senzorul prin funingine sau condens. Vezi manualul AEM pentru alimentare.

## Cablarea în compartimentul motor

- Folosește conectori auto etanși (ex. Deutsch DT) la peretele despărțitor.
- Ține cablurile senzorilor departe de cablurile injectoarelor, de alternator și de
  demaror. Unde nu se poate, încrucișează-le la 90°.
- Folosește manșon termic lângă galeria de evacuare.
