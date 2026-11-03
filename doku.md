# ESP32-Bewegungsampel

Interaktive Systemübersicht: [system-uebersicht.html](system-uebersicht.html). Datei direkt im Browser öffnen; Internet oder Webserver nicht erforderlich.

## Zweck

Das Projekt steuert eine dreifarbige Ampel mit einem PIR-Bewegungssensor. Ein AM2302/DHT22 misst zusätzlich Temperatur und relative Luftfeuchtigkeit. Der ESP32 verbindet sich mit dem Heim-WLAN und zeigt den aktuellen Anlagenstatus auf einer lokalen Webseite an.

Der Zugangspunkt bietet keinen Internetzugang. Der ESP32 stellt Livewerte bereit. Ein optionaler PC-Logger speichert diese regelmäßig in einer SQLite-Datenbank im Ordner `db`.

## Hardware und Anschlüsse

| Gerät | Anschluss | ESP32 |
| --- | --- | --- |
| Ampel | GND | GND |
| Ampel | R | GPIO 25 |
| Ampel | Y | GPIO 26 |
| Ampel | G | GPIO 27 |
| PIR-Bewegungssensor | GND | GND |
| PIR-Bewegungssensor | VCC | 5V, sofern für das Sensormodul vorgesehen |
| PIR-Bewegungssensor | OUT | GPIO 33 |
| AM2302/DHT22 | DATA | GPIO 4 |
| AM2302/DHT22 | VCC | 3V3 |
| AM2302/DHT22 | GND | GND |

Alle Geräte benötigen eine gemeinsame Masse. Der ESP32-Eingang ist nicht 5-V-tolerant. Vor dem Anschluss an GPIO 33 sicherstellen, dass der Ausgang des PIR-Moduls höchstens 3,3 V liefert. Falls das Modul 5 V am Ausgang liefert, Pegelwandler verwenden.

Bei einem nackten DHT22-Sensor ist zwischen DATA und 3V3 üblicherweise ein Pull-up-Widerstand von 4,7 bis 10 kOhm erforderlich. Viele Sensormodule besitzen diesen Widerstand bereits.

## Funktionen

- Startzustand der Ampel ist Grün.
- Bei erkannter Bewegung wechselt die Ampel zwei Sekunden auf Gelb und anschließend auf Rot.
- Rot bleibt aktiv, solange der PIR-Sensor Bewegung meldet.
- Nach Ende der Bewegung folgt eine zweisekündige gelbe Übergangsphase, danach wieder Grün.
- Temperatur und Luftfeuchtigkeit werden etwa alle 2,5 Sekunden ausgelesen.
- Die Ampelsteuerung blockiert den Programmablauf nicht; Webanfragen werden daher auch während eines Ampelwechsels verarbeitet.
- Die Webseite fragt den Status alle 1,5 Sekunden über die lokale JSON-API ab.
- Der PC-Logger schreibt einen Datensatz alle 30 Sekunden in `db\sensordaten.db`.

## WLAN und Webseite

Beim ersten Einrichten `include/config.example.h` als `include/config.h` kopieren und dort `WIFI_SSID` und `WIFI_PASSWORD` durch den Namen und das Passwort des Heim-WLANs ersetzen. `include/config.h` wird von Git ignoriert; die Vorlage enthält keine echten Zugangsdaten. Der ESP32 benötigt ein 2,4-GHz-WLAN. Nach dem Start zeigt der serielle Monitor mit 115200 Baud die vom Router zugewiesene IP-Adresse und die Webseitenadresse an. Ein Gerät im selben Heimnetz kann die Seite unter `http://<ESP32-IP>/` öffnen. Eine DHCP-Reservierung im Router hält die IP-Adresse gleich.

Der Webserver verwendet unverschlüsseltes HTTP und hat keine Anmeldung. Er ist für den Zugriff im vertrauenswürdigen Heimnetz gedacht. Für Zugriff von unterwegs ein VPN zum Heimnetz verwenden; den Webserver nicht direkt per Portweiterleitung ins Internet freigeben.

Die API liefert JSON mit Temperatur, Luftfeuchtigkeit, Bewegungsstatus, Ampelfarbe, Laufzeit und IP-Adresse. Ungültige Sensorwerte werden als `null` ausgegeben.

## Datenbank auf dem PC

Der ESP32 kann nicht direkt in einen Windows-Ordner schreiben. `tools/log_sensor_data.py` läuft deshalb auf dem PC und fragt den ESP32 alle 30 Sekunden ab. Die Datenbank ist SQLite; Python 3 wird benötigt, zusätzliche Python-Pakete nicht.

1. PC mit demselben Heim-WLAN wie den ESP32 verbinden.
2. Die IP-Adresse aus dem seriellen Monitor übernehmen.
3. Im Projektordner ein Terminal öffnen und den Logger mit dieser Adresse starten. Die Datenbank wird im Ordner `db` gespeichert; der Ordner wird bei Bedarf angelegt:

```powershell
python tools\log_sensor_data.py --url http://172.20.10.5/api/status --database "db\sensordaten.db"
```

Dabei `172.20.10.5` durch die tatsächliche ESP32-IP ersetzen. Logger mit `Ctrl+C` beenden. Während er läuft, muss der PC mit demselben WLAN wie der ESP32 verbunden bleiben.

Der Logger speichert Zeitstempel, Temperatur, Luftfeuchtigkeit, Bewegung, Ampelfarbe und ESP32-Laufzeit in Tabelle `sensor_readings`. Bei ungültigen DHT22-Werten werden Temperatur und Luftfeuchtigkeit als SQL-`NULL` gespeichert. Logger mit `Ctrl+C` beenden. Während er läuft, muss der PC mit dem ESP32-Zugangspunkt verbunden bleiben.

Die Weboberfläche zeigt weiterhin Livewerte, schreibt selbst aber nicht in die Datenbank. Die Datenbankdatei `db\sensordaten.db` erscheint beim ersten Start des Loggers und bleibt nach dessen Ende erhalten.

## Datenbank-Backup

`BackupService\backup.ps1` kopiert `db\sensordaten.db` sofort beim Start und danach alle 60 Sekunden. Die Sicherung hat abweichenden Namen `sensordaten_backup.db` und liegt unter `C:\vscode\ESP32-Ampel\BackupService\Backup\sensordaten_backup.db`. Das Skript legt benötigte Ordner automatisch an. Erfolgs- und Fehlermeldungen stehen in `BackupService\Logs\Success\success.log` und `BackupService\Logs\Error\error.log`.

Logger zuerst starten und mindestens einen Datensatz speichern lassen. Dann im Projektordner ein zweites PowerShell-Terminal öffnen und das Backup starten:

```powershell
.\BackupService\backup.ps1
```

Falls PowerShell die Skriptausführung blockiert, gilt die Freigabe nur für dieses Terminal:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\BackupService\backup.ps1
```

Mit `Ctrl+C` beendest du das Backup-Skript. Das laufende Logger-Terminal bleibt davon unberührt.

Die letzten 20 Datensätze lassen sich beispielsweise mit DB Browser for SQLite oder folgender SQL-Abfrage anzeigen:

```sql
SELECT recorded_at, temperature_c, humidity_percent, motion_detected, traffic_light
FROM sensor_readings
ORDER BY id DESC
LIMIT 20;
```

## Projekt bauen und übertragen

PlatformIO-Projekt in VS Code öffnen. Firmware und Webseite werden getrennt übertragen:

```sh
pio run
pio run --target uploadfs
pio run --target upload
```

Alternativ in PlatformIO nacheinander **Build Filesystem Image**, **Upload Filesystem Image** und **Upload** ausführen. Die HTML-Datei liegt in `data/index.html`; PlatformIO packt diesen Ordner in ein LittleFS-Abbild. Bei Änderungen an der Webseite das Dateisystem-Abbild erneut hochladen.

Der serielle Monitor verwendet 115200 Baud. Nach dem Start meldet er WLAN-Name und Webadresse. Firmware-Upload und serieller Monitor können denselben COM-Port nicht gleichzeitig verwenden.

## Projektstruktur

| Pfad | Inhalt |
| --- | --- |
| `src/main.cpp` | Initialisierung und Hauptschleife |
| `src/sensors.cpp` | PIR- und DHT22-Zugriff |
| `src/traffic_light.cpp` | Ampelausgänge und zeitgesteuerte Zustandslogik |
| `src/webserver.cpp` | WLAN-Verbindung, Webseite und Status-API |
| `include/config.h` | GPIOs und WLAN-Zugangsdaten |
| `data/index.html` | Lokale Statusseite |
| `tools/log_sensor_data.py` | PC-Logger für SQLite-Aufzeichnung |
| `db\sensordaten.db` | Vom Logger erzeugte SQLite-Datenbank |
| `BackupService\Backup\sensordaten_backup.db` | Letzte Sicherung der Datenbank |

## Fehlerbehebung

| Symptom | Prüfung |
| --- | --- |
| Webseite nicht erreichbar | Richtiges WLAN verwenden, `http://` statt `https://` öffnen und serielle Ausgabe auf gestarteten Access Point prüfen. |
| Webseite meldet fehlende Dateien | `pio run --target uploadfs` erneut ausführen. |
| `db\sensordaten.db` wird nicht angelegt | Logger mit gültiger API-Adresse und `--database "db\sensordaten.db"` starten. |
| Keine neuen Datenbankeinträge | Logger-Terminal auf Verbindungsfehler prüfen; PC muss mit demselben WLAN wie der ESP32 verbunden bleiben. |
| Sensorwerte werden als `--.-` angezeigt | DHT22-Versorgung, DATA an GPIO 4 und erforderlichen Pull-up prüfen. |
| Bewegung wird dauerhaft erkannt | PIR-Ausgang, gemeinsame Masse und Sensor-Empfindlichkeit prüfen. |
| Ampel schaltet nicht | GPIO-Zuordnung und gemeinsame Masse prüfen; Ampel nicht direkt aus einem ESP32-GPIO versorgen, wenn sie mehr Strom benötigt. |
| Kein serieller Text | Monitor auf 115200 Baud stellen und den richtigen COM-Port öffnen. |


