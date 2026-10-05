#!/usr/bin/env python3
"""Poll the ESP32 status endpoint and save samples to a local SQLite database."""

import argparse
import json
import sqlite3
import time
from datetime import datetime
from pathlib import Path
from urllib.error import URLError
from urllib.request import urlopen


PROJECT_DIRECTORY = Path(__file__).resolve().parent.parent
DEFAULT_DATABASE = PROJECT_DIRECTORY / "sensordaten.db"
DEFAULT_STATUS_URL = "http://192.168.4.1/api/status"


def initialize_database(connection):
    connection.execute(
        """
        CREATE TABLE IF NOT EXISTS sensor_readings (
            id INTEGER PRIMARY KEY,
            recorded_at TEXT NOT NULL,
            temperature_c REAL,
            humidity_percent REAL,
            motion_detected INTEGER NOT NULL CHECK (motion_detected IN (0, 1)),
            traffic_light TEXT NOT NULL,
            uptime_seconds INTEGER NOT NULL
        )
        """
    )
    connection.execute(
        "CREATE INDEX IF NOT EXISTS idx_sensor_readings_recorded_at "
        "ON sensor_readings (recorded_at)"
    )
    connection.commit()


def fetch_status(status_url):
    with urlopen(status_url, timeout=8) as response:
        status = json.loads(response.read().decode("utf-8"))

    if not isinstance(status, dict):
        raise ValueError("Status-API lieferte kein JSON-Objekt.")

    required_fields = (
        "readingsValid",
        "motionDetected",
        "trafficLight",
        "uptimeSeconds",
    )
    if any(field not in status for field in required_fields):
        raise ValueError("Status-API-Antwort enthält nicht alle erforderlichen Felder.")

    return status


def store_status(connection, status):
    readings_valid = status["readingsValid"] is True
    temperature = status.get("temperatureC") if readings_valid else None
    humidity = status.get("humidityPercent") if readings_valid else None
    motion = status["motionDetected"]
    traffic_light = status["trafficLight"]
    uptime = status["uptimeSeconds"]

    if not isinstance(motion, bool):
        raise ValueError("Bewegungsstatus in API-Antwort ist ungültig.")
    if traffic_light not in ("green", "yellow", "red"):
        raise ValueError("Ampelstatus in API-Antwort ist ungültig.")
    if not isinstance(uptime, int):
        raise ValueError("Laufzeit in API-Antwort ist ungültig.")

    recorded_at = datetime.now().astimezone().isoformat(timespec="seconds")
    connection.execute(
        """
        INSERT INTO sensor_readings (
            recorded_at,
            temperature_c,
            humidity_percent,
            motion_detected,
            traffic_light,
            uptime_seconds
        ) VALUES (?, ?, ?, ?, ?, ?)
        """,
        (recorded_at, temperature, humidity, int(motion), traffic_light, uptime),
    )
    connection.commit()


def main():
    parser = argparse.ArgumentParser(description="ESP32-Messwerte alle 30 Sekunden lokal speichern.")
    parser.add_argument("--url", default=DEFAULT_STATUS_URL, help="URL der ESP32-Status-API")
    parser.add_argument("--interval", type=float, default=30, help="Abfrageintervall in Sekunden")
    parser.add_argument(
        "--database",
        type=Path,
        default=DEFAULT_DATABASE,
        help="SQLite-Datei; Standard: sensordaten.db im Projektordner",
    )
    arguments = parser.parse_args()
    if arguments.interval <= 0:
        parser.error("--interval muss größer als 0 sein.")

    database_path = arguments.database.resolve()
    connection = sqlite3.connect(database_path)
    initialize_database(connection)
    print("Speichere Daten in:", database_path)
    print("Abfrageintervall:", arguments.interval, "Sekunden. Beenden mit Ctrl+C.")

    next_sample = time.monotonic()
    try:
        while True:
            wait_seconds = next_sample - time.monotonic()
            if wait_seconds > 0:
                time.sleep(wait_seconds)

            try:
                status = fetch_status(arguments.url)
                store_status(connection, status)
                print(
                    datetime.now().astimezone().isoformat(timespec="seconds"),
                    "gespeichert:",
                    status["trafficLight"],
                    "Bewegung:",
                    status["motionDetected"],
                )
            except (URLError, TimeoutError, ValueError, sqlite3.Error) as error:
                print("Messwert nicht gespeichert:", error)

            next_sample += arguments.interval
            if next_sample <= time.monotonic():
                next_sample = time.monotonic() + arguments.interval
    except KeyboardInterrupt:
        print("Logger beendet.")
    finally:
        connection.close()


if __name__ == "__main__":
    main()