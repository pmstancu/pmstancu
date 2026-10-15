# 5. Utilizare

## Display-urile

Fiecare dintre cele 3 ecrane arată:

- **sus și pe arc:** canalul principal (arcul se umple după valoare);
- **jos:** canalul secundar, cu text mai mic.

Așezarea implicită (se schimbă din pagina web, secțiunea „Display-uri”):

| Ecran | Arc | Secundar |
|---|---|---|
| 1 | EGT | Aer după intercooler |
| 2 | Boost | λ |
| 3 | Apă | Ulei |

Culori:

- **verde:** normal;
- **portocaliu:** avertizare;
- **roșu:** critic. Fundalul ecranului clipește roșu și pornește buzzerul.

Alte semne:

- **„---” + „SENZOR”:** senzorul nu răspunde, are un fir rupt sau arată o valoare imposibilă.
- **„>1.23”** la λ: ieșirea analogică AEM e la capătul scalei, iar valoarea reală e mai mare.
- **S / A** sub λ: λ vine de pe serial (S) sau de pe ieșirea analogică (A).
- Pe ecranul 1: **WiFi** (Wi-Fi pornit), **SD!** (problemă cu cardul), **MUT** (buzzer oprit).

Dacă un canal care **nu** e afișat pe niciun ecran intră în alarmă critică (de exemplu
bateria), el înlocuiește temporar valoarea secundară de pe ecranul 3.

## Butonul

| Apăsare | Efect |
|---|---|
| scurtă (în timpul unei alarme critice) | oprește buzzerul până când dispar alarmele critice |
| lungă, 3 s | pornește / oprește Wi-Fi-ul |

## Pagina web

1. Ține butonul apăsat 3 s. Pe ecrane apare „WiFi PORNIT” și numele rețelei.
2. De pe telefon, conectează-te la rețeaua **LC100-Gauge** (parola implicită: `landcruiser100`).
3. Deschide **http://192.168.4.1**. Aceasta e adresa implicită a punctului de acces ESP32 în
   biblioteca Arduino. ⚠ Dacă nu răspunde, verifică adresa în setările Wi-Fi ale telefonului
   (gateway / router).

Secțiuni:

- **Live:** toate valorile, actualizate de 2 ori pe secundă, cu sursa λ, tensiunea AEM,
  referința de 5 V și starea cardului.
- **Setări:**
  - praguri de alarmă (activ, avertizare, critic, histerezis);
  - ce apare pe fiecare ecran și unitatea pentru boost (psi sau bar);
  - calibrările senzorilor și ale divizoarelor;
  - Wi-Fi și log.

  La salvare, valorile sunt verificate. Cele invalide sunt refuzate cu un mesaj.
  **Schimbă parola Wi-Fi implicită**: ea apare în acest repository, deci e publică.
- **Log-uri:** lista fișierelor de pe card, cu descărcare și ștergere. Fișierul în curs de
  scriere nu se poate șterge.
- **AEM:** ultimii octeți primiți pe RS232, în hex și ca text. Așa se confirmă formatul serial.

Wi-Fi-ul se oprește singur după 10 minute fără activitate (configurabil; 0 = niciodată),
dar nu cât timp un telefon e conectat.

## Log-ul CSV

- La fiecare pornire se creează un fișier nou: `log_0001.csv`, `log_0002.csv` etc.
  ESP32 nu are ceas, deci fișierele nu au dată și oră; coloana `ms` e timpul de la pornire.
- Datele se scriu pe card o dată pe secundă. La tăierea contactului se pierde cel mult
  ultima secundă. ⚠ Tăierea curentului în timpul unei scrieri poate, rar, corupe fișierul
  sau sistemul de fișiere al cardului.
- Coloane:

| Coloană | Conținut |
|---|---|
| `ms` | milisecunde de la pornire |
| `ect_c`, `eot_c`, `iat_c`, `egt_c` | temperaturi, °C |
| `boost_bar` | boost relativ, bar (în log mereu bar, indiferent de afișaj) |
| `lambda` | λ |
| `vbat_v` | tensiunea bateriei |
| `lambda_src` | 0 = lipsă, 1 = serial, 2 = analogic |
| `aem_v` | tensiunea la ieșirea analogică AEM |
| `ref5v` | alimentarea senzorului Auber |
| `alarm` | cel mai mare nivel de alarmă: 0, 1 sau 2 |

O celulă goală înseamnă că senzorul era indisponibil în acel moment.
