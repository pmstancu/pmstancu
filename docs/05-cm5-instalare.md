# 5. Instalare pe CM5

> ⚠ Comenzile de mai jos sunt pentru Raspberry Pi OS (Bookworm, 64-bit). Numele unor pachete
> și mecanismul de autostart s-au schimbat între versiuni, așa că verifică-le pe imaginea ta.

## 5.1 Aplicația gaugehub

```bash
sudo apt install -y python3-venv git
git clone <url-repo> ~/pmstancu
cd ~/pmstancu/cm5
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt

# acces la portul serial fără sudo (necesită logout/login)
sudo usermod -aG dialout $USER

# găsește numele stabil al ESP32 (nu se schimbă dacă mai conectezi un GPS sau un SDR pe USB)
ls -l /dev/serial/by-id/
```

Test manual:

```bash
.venv/bin/python -m gaugehub --port /dev/serial/by-id/usb-Espressif...  # numele tău
# deschide http://127.0.0.1:8080
```

Opțiuni utile:
- `--sim`: date simulate, fără ESP32.
- `--host 0.0.0.0`: dashboard-ul devine accesibil și de pe telefon, în aceeași rețea Wi-Fi.
  Serverul nu are autentificare, deci folosește opțiunea doar într-o rețea a ta.
- `--log-dir`: directorul pentru CSV-uri, câte un fișier pe zi.

## 5.2 Serviciu systemd

```bash
# editează User, căile și REPLACE_ME din fișier
sudo cp systemd/gaugehub.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now gaugehub
journalctl -u gaugehub -f
```

## 5.3 Kiosk pe ecranul de 7"

`systemd/kiosk.desktop` pornește Chromium pe tot ecranul după login-ul desktop:

```bash
mkdir -p ~/.config/autostart
cp systemd/kiosk.desktop ~/.config/autostart/
```

⚠ Pe versiunile recente de Raspberry Pi OS, comanda poate fi `chromium` sau
`chromium-browser`, iar desktopul Wayland (labwc) poate folosi propriul fișier de autostart.
Verifică ce se aplică versiunii tale.

## 5.4 Oprirea curată cu M2-ATX-HV

CM5 trebuie oprit corect înainte să i se taie curentul, altfel riști să corupi sistemul de
fișiere. Principiul:

1. Semnalul de contact (12 V IG) trece printr-un **optocuplor** (sau un divizor cu protecție)
   către un GPIO al CM5. **Nu lega niciodată 12 V direct la un GPIO.**
2. În `/boot/firmware/config.txt` adaugi overlay-ul care oprește sistemul când pinul
   e tras la masă:
   ```
   dtoverlay=gpio-shutdown,gpio_pin=<pinul tău>
   ```
   ⚠ Verifică parametrii overlay-ului (`gpio_pin`, `active_low`, `gpio_pull`) în
   `/boot/firmware/overlays/README` de pe sistemul tău.
3. M2-ATX-HV întârzie tăierea alimentării după oprirea contactului. ⚠ Alege din manualul
   Mini-Box un mod cu o întârziere mai mare decât durata de shutdown a CM5. Nu cunosc
   valorile exacte ale modurilor.

Recomandări:
- Ține log-urile pe NVMe sau USB, nu pe partiția de sistem.
- Pentru protecție maximă poți activa Overlay File System (rădăcină read-only) din
  `raspi-config` și păstra doar partiția de log-uri writable.

## 5.5 Alte aplicații (SDR, GPS)

gaugehub ocupă doar portul ESP32 și portul HTTP 8080. Un GPS pe USB apare ca alt
`/dev/ttyACM*` sau `/dev/ttyUSB*`, de aceea serviciul folosește calea stabilă din
`/dev/serial/by-id/`.
