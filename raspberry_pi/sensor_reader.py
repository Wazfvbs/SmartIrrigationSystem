# sensor_reader.py

import serial
import time
from config import SERIAL_PORT, BAUDRATE

def init_serial():
    ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1)
    return ser

def read_sensors(ser):
    """
    Reads a line of sensor data from the microcontroller via serial.
    Example format from MCU: "temp=25.5,hum=60,soil=300,water=80"
    Modify parsing to match your actual protocol.
    Returns a dict like: {"temp": float, "humi": float, "soil": float, "water": float}
    """
    line = ser.readline().decode('utf-8', errors='ignore').strip()
    if not line:
        return None

    print(f"Received sensor data: {line}")  # Output the received data to the console for debugging

    try:
        # Example "temp=25.5,hum=60,soil=300,water=80"
        parts = line.split(',')
        data_dict = {}
        for part in parts:
            kv = part.split('=')
            if len(kv) == 2:
                key = kv[0].strip()
                val = kv[1].strip()
                data_dict[key] = float(val)
        return data_dict
    except Exception as e:
        print(f"Error parsing sensor data: {e}")
        return None
