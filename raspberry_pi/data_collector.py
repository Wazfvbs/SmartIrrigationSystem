#!/usr/bin/env python3
import serial
import time
import sqlite3
import os
import datetime
import requests

SERIAL_PORT = "/dev/ttyUSB0"
BAUDRATE = 115200

DB_PATH = "sensor_data.db"


SOIL_MOISTURE_THRESHOLD = 400 

def init_db():
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute("""
    CREATE TABLE IF NOT EXISTS raw_data_1min (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        ts DATETIME DEFAULT CURRENT_TIMESTAMP,
        temp REAL,
        humi REAL,
        soil REAL,
        water REAL
    );
    """)
    c.execute("""
    CREATE TABLE IF NOT EXISTS raw_data_1h (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        ts DATETIME DEFAULT CURRENT_TIMESTAMP,
        temp REAL,
        humi REAL,
        soil REAL,
        water REAL
    );
    """)
    conn.commit()
    conn.close()

def save_data_1min(temp, humi, soil, water):
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute("""
        INSERT INTO raw_data_1min(temp, humi, soil, water)
        VALUES(?,?,?,?)
    """, (temp, humi, soil, water))
    conn.commit()
    conn.close()

def save_data_1h(temp, humi, soil, water):
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute("""
        INSERT INTO raw_data_1h(temp, humi, soil, water)
        VALUES(?,?,?,?)
    """, (temp, humi, soil, water))
    conn.commit()
    conn.close()

def clean_old_data_1min():
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute("""
        DELETE FROM raw_data_1min
        WHERE ts < datetime('now', '-1 day')
    """)
    conn.commit()
    conn.close()
def Post(payload):
    r = requests.post("http://192.168.0.147:8080/flowers_war_exploded/DataReceiver", data=payload)
    print(r.text)
    
def main():
    init_db()
    ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1)
    if ser.isOpen():
        print("Serial open success:", SERIAL_PORT)
    else:
        print("Failed to open serial port.")
        return

    last_save_hour = datetime.datetime.now().hour
    while True:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if not line:
            continue

        try:
            data_parts = line.split(',')
            temp_str = data_parts[0].split('=')[-1]
            humi_str = data_parts[1].split('=')[-1]
            soil_str = data_parts[2].split('=')[-1]
            water_str = data_parts[3].split('=')[-1]

            temp = float(temp_str)
            humi = float(humi_str)
            soil = float(soil_str)
            water = float(water_str)
            save_data_1min(temp, humi, soil, water)
            
            clean_old_data_1min()

            current_hour = datetime.datetime.now().hour
            if current_hour != last_save_hour:
                save_data_1h(temp, humi, soil, water)
                last_save_hour = current_hour

            if soil < SOIL_MOISTURE_THRESHOLD:
                ser.write("p1\n".encode('utf-8'))
                #print("Soil too dry, start watering!")
            else:
                ser.write("p0\n".encode('utf-8'))

            print(f"Received: T={temp}, H={humi}, soil={soil}, water={water}")
            payload = {
              "deviceId": "raspi001",
              "temp": temp,
              "humi": humi,
              "soil": soil,
              "water": water
            }
            print(payload)
            #Post(payload)
        except Exception as e:
            print("Parse error:", e)
            pass

        time.sleep(1)

if __name__ == "__main__":
    main()
