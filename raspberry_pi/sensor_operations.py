import time
from sensor_reader import init_serial, read_sensors
from watering import water_on, water_off

class SensorOperations:
    def __init__(self):
        self.ser = init_serial()

    def get_sensor_data(self):
        sensor_data = read_sensors(self.ser)
        return sensor_data

    def control_watering(self, command):
        if command.lower() == "p1":
            water_on(self.ser)
        elif command.lower() == "p0":
            water_off(self.ser)
