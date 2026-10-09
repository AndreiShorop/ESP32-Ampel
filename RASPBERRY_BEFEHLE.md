# Raspberry Pi: Befehlsübersicht

Platzhalter ersetzen und die spitzen Klammern `<>` weglassen.

| Platzhalter | Bedeutung           | Beispiel       |
| ----------- | ------------------- | -------------- |
| `IP_PI`     | IP des Raspberry Pi | `192.168.1.97` |
| `IP_ESP`    | IP des ESP32        | `192.168.1.46` |
| Benutzer    | Login auf dem Pi    | `andrey112`    |

Projektordner auf dem Pi: `~/ESP32-Ampel`
Datenbank: `~/ESP32-Ampel/sensordaten.db`

---

## 1. Verbindung (vom Laptop)

```bash
ssh andrey112@IP_PI
```

Trennen: `exit` oder `Ctrl+D`.

IP des Pi herausfinden (auf dem Pi):

```bash
hostname -I
```

---

## 2. Speicherplatz prüfen (auf dem Pi)

```bash
df -h /                      # freier Platz auf der SD-Karte
du -sh ~/ESP32-Ampel         # Größe des Projektordners
ls -lh ~/ESP32-Ampel/*.db    # Größe der Datenbank
du -h --max-depth=1 ~ | sort -h | tail   # größte Ordner im Home
free -h                      # Arbeitsspeicher
```

---

## 3. Logger (systemd-Dienst `ampel-logger`)

```bash
systemctl status ampel-logger        # Status
sudo systemctl start ampel-logger    # starten
sudo systemctl stop ampel-logger     # stoppen
sudo systemctl restart ampel-logger  # neu starten
sudo systemctl enable ampel-logger   # Autostart beim Booten an
sudo systemctl disable ampel-logger  # Autostart aus
journalctl -u ampel-logger -f        # Live-Log (Beenden: Ctrl+C)
journalctl -u ampel-logger -n 50     # letzte 50 Zeilen
```

Läuft der Logger noch irgendwo anders (manuell, tmux)?

```bash
pgrep -af log_sensor_data
pkill -f log_sensor_data.py          # alle manuellen Logger beenden
```

Dienst-Datei bearbeiten (danach `daemon-reload` und `restart`):

```bash
sudo nano /etc/systemd/system/ampel-logger.service
sudo systemctl daemon-reload
sudo systemctl restart ampel-logger
```

Logger manuell im Vordergrund starten (stoppt beim Trennen von SSH):

```bash
cd ~/ESP32-Ampel
python3 tools/log_sensor_data.py --url http://IP_ESP/api/status
```

Mit `tmux` weiterlaufen lassen:

```bash
tmux new -s logger          # neue Session
# Logger starten, dann Ctrl+B, danach D zum Trennen
tmux attach -t logger       # wieder verbinden
tmux ls                     # Sessions anzeigen
```

---

## 4. ESP32 prüfen

```bash
curl http://IP_ESP/api/status        # JSON mit Messwerten
ping -c 3 IP_ESP                     # erreichbar?
```

Website im Browser: `http://IP_ESP/`

Serielle Ausgabe (ESP per USB am Pi):

```bash
ls /dev/ttyUSB* /dev/ttyACM*
screen /dev/ttyUSB0 115200           # Beenden: Ctrl+A, K, Y
```

Geräte im Netz suchen:

```bash
sudo apt install -y nmap
nmap -sn 192.168.1.0/24
```

---

## 5. Datenbank prüfen (auf dem Pi)

```bash
sqlite3 ~/ESP32-Ampel/sensordaten.db ".tables"
sqlite3 ~/ESP32-Ampel/sensordaten.db "SELECT COUNT(*) FROM sensor_readings;"
sqlite3 ~/ESP32-Ampel/sensordaten.db "SELECT MAX(recorded_at) FROM sensor_readings;"
sqlite3 -header -column ~/ESP32-Ampel/sensordaten.db \
  "SELECT * FROM sensor_readings ORDER BY id DESC LIMIT 10;"
```

`sqlite3` installieren, falls es fehlt:

```bash
sudo apt install -y sqlite3
```

---

## 6. Datenbank auf den Laptop kopieren

**Schritt 1: Konsistente Kopie auf dem Pi** (auch während der Logger läuft):

```bash
sqlite3 ~/ESP32-Ampel/sensordaten.db ".backup /tmp/sensordaten_kopie.db"
```

**Schritt 2: Auf dem Laptop herunterladen** (neues Terminal, nicht auf dem Pi):

```bash
mkdir -p ~/Downloads
scp andrey112@IP_PI:/tmp/sensordaten_kopie.db ~/Downloads/sensordaten.db
```

Direkt die Originaldatei laden:

```bash
scp andrey112@IP_PI:~/ESP32-Ampel/sensordaten.db ~/Downloads/
```

**Schritt 3: In DataGrip öffnen**
`File → New → Data Source → SQLite`, Datei auswählen, `Test Connection`.
Tabelle: `main → tables → sensor_readings`. Nach einer neuen Kopie: Rechtsklick und `Refresh`.

---

## 7. Dateien übertragen

Vom Laptop auf den Pi:

```bash
scp datei.txt andrey112@IP_PI:~/
scp -r ordner andrey112@IP_PI:~/
```

Vom Pi auf den Laptop:

```bash
scp andrey112@IP_PI:~/datei.txt ~/Downloads/
```

Projekt abgleichen (Laptop zum Pi):

```bash
rsync -av --exclude '.pio' --exclude '*.db' ./ESP32-Ampel/ andrey112@IP_PI:~/ESP32-Ampel/
```

---

## 8. ESP32 programmieren (PlatformIO auf dem Pi)

Installation:

```bash
sudo apt install -y python3-venv python3-pip
python3 -m venv ~/pio-env
~/pio-env/bin/pip install platformio
echo 'export PATH=$PATH:~/pio-env/bin' >> ~/.bashrc && source ~/.bashrc
sudo usermod -aG dialout $USER       # danach neu anmelden
```

Bauen und hochladen:

```bash
cd ~/ESP32-Ampel
pio run -t upload        # Firmware
pio run -t uploadfs      # Website (data/index.html)
pio device monitor       # Serielle Ausgabe (115200)
```

WLAN-Daten stehen in `include/config.h`. Nach einer Änderung muss neu geflasht werden.

---

## 9. Raspberry Pi verwalten

```bash
sudo apt update && sudo apt upgrade -y   # Updates
sudo reboot                              # Neustart
sudo shutdown -h now                     # Herunterfahren
uptime                                   # Laufzeit
vcgencmd measure_temp                    # CPU-Temperatur
ip -4 addr show                          # Netzwerkadressen
```

---

## 10. Häufige Fehler

| Meldung                                            | Ursache und Lösung                                                  |
| -------------------------------------------------- | ------------------------------------------------------------------- |
| `Datei oder Verzeichnis nicht gefunden` bei `<IP>` | Spitze Klammern weglassen                                           |
| `scp` meldet Datei nicht gefunden                  | Der Befehl lief auf dem Pi statt auf dem Laptop                     |
| `Connection refused` bei SSH                       | `sudo systemctl enable --now ssh` auf dem Pi                        |
| `pio: command not found`                           | PlatformIO installieren, PATH setzen (Abschnitt 8)                  |
| Nur `sqlite_master` in DataGrip                    | Logger lief nie auf dieser Datei, Logger starten, neue Kopie ziehen |
| Logger stoppt beim SSH-Trennen                     | Als Dienst oder in `tmux` starten                                   |
| `Messwert nicht gespeichert`                       | ESP nicht erreichbar, IP und WLAN prüfen, `curl` testen             |
