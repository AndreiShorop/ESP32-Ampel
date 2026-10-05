# ESP32-Bewegungsampel

## Zweck

Das Projekt steuert eine dreifarbige Ampel mit einem PIR-Bewegungssensor. Ein AM2302/DHT22 misst zusätzlich Temperatur und relative Luftfeuchtigkeit. Der ESP32 stellt ein eigenes WLAN bereit und zeigt den aktuellen Anlagenstatus auf einer lokalen Webseite an.

Der Zugangspunkt bietet keinen Internetzugang. Der ESP32 stellt Livewerte bereit. Ein optionaler PC-Logger speichert diese regelmäßig in einer SQLite-Datenbank im Projektordner.

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
- Der PC-Logger schreibt einen Datensatz alle 30 Sekunden in `sensordaten.db` im Projektordner.

## WLAN und Webseite

Standardzugangspunkt:

| Einstellung | Wert |
| --- | --- |
| WLAN-Name | `Ampel-Sensor` |
| WLAN-Passwort | `Ampel-Setup-2026` |
| ESP32-Adresse | `192.168.4.1` |
| Webseite | `http://192.168.4.1/` |
| Status-API | `http://192.168.4.1/api/status` |

WLAN-Name und Passwort sind in `include/config.h` definiert. Das Passwort muss mindestens acht Zeichen lang sein. Vor einer Nutzung außerhalb eines geschützten Tests das Standardpasswort ändern. Die Verbindung verwendet kein TLS und ist nur für ein lokales, vertrauenswürdiges Testnetz vorgesehen.

Zum Öffnen der Webseite und zum Speichern der Daten den PC mit dem WLAN `Ampel-Sensor` verbinden. Danach im Browser `http://192.168.4.1/` öffnen. Das WLAN hat absichtlich keinen Internetzugang. Falls Windows fragt, ob es trotzdem verbunden bleiben soll, die Verbindung beibehalten.

Die API liefert JSON mit Temperatur, Luftfeuchtigkeit, Bewegungsstatus, Ampelfarbe, Laufzeit und IP-Adresse. Ungültige Sensorwerte werden als `null` ausgegeben.

## Datenbank auf dem PC

Der ESP32 kann nicht direkt in einen Windows-Ordner schreiben. `tools/log_sensor_data.py` läuft deshalb auf dem PC, fragt den ESP32 alle 30 Sekunden ab und legt `sensordaten.db` im Projekt-Hauptordner an. Die Datenbank ist SQLite; Python 3 wird benötigt, zusätzliche Python-Pakete nicht.

1. PC mit dem WLAN `Ampel-Sensor` verbinden und Verbindung ohne Internetzugang beibehalten.
2. Im Projektordner ein Terminal öffnen.
3. Logger starten:

```powershell
python tools\log_sensor_data.py
```

Der Logger speichert Zeitstempel, Temperatur, Luftfeuchtigkeit, Bewegung, Ampelfarbe und ESP32-Laufzeit in Tabelle `sensor_readings`. Bei ungültigen DHT22-Werten werden Temperatur und Luftfeuchtigkeit als SQL-`NULL` gespeichert. Logger mit `Ctrl+C` beenden. Während er läuft, muss der PC mit dem ESP32-Zugangspunkt verbunden bleiben.

Die Weboberfläche zeigt weiterhin Livewerte, schreibt selbst aber nicht in die Datenbank. Die Datenbankdatei erscheint erst beim ersten Start des Loggers. Sie bleibt im Projekt-Hauptordner erhalten, auch nachdem der Logger beendet wurde.

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
| `src/webserver.cpp` | Access Point, Webseite und Status-API |
| `include/config.h` | GPIOs und WLAN-Zugangsdaten |
| `data/index.html` | Lokale Statusseite |
| `tools/log_sensor_data.py` | PC-Logger für SQLite-Aufzeichnung |
| `sensordaten.db` | Vom Logger erzeugte SQLite-Datenbank im Projekt-Hauptordner |

## Fehlerbehebung

| Symptom | Prüfung |
| --- | --- |
| Webseite nicht erreichbar | Richtiges WLAN verwenden, `http://` statt `https://` öffnen und serielle Ausgabe auf gestarteten Access Point prüfen. |
| Webseite meldet fehlende Dateien | `pio run --target uploadfs` erneut ausführen. |
| `sensordaten.db` wird nicht angelegt | PC mit `Ampel-Sensor` verbinden und `python tools\log_sensor_data.py` starten. |
| Keine neuen Datenbankeinträge | Logger-Terminal auf Verbindungsfehler prüfen; PC muss mit `Ampel-Sensor` verbunden bleiben. |
| Sensorwerte werden als `--.-` angezeigt | DHT22-Versorgung, DATA an GPIO 4 und erforderlichen Pull-up prüfen. |
| Bewegung wird dauerhaft erkannt | PIR-Ausgang, gemeinsame Masse und Sensor-Empfindlichkeit prüfen. |
| Ampel schaltet nicht | GPIO-Zuordnung und gemeinsame Masse prüfen; Ampel nicht direkt aus einem ESP32-GPIO versorgen, wenn sie mehr Strom benötigt. |
| Kein serieller Text | Monitor auf 115200 Baud stellen und den richtigen COM-Port öffnen. |