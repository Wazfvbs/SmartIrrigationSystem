# config.py

BASE_URL = "http://192.168.0.147:8080/flowers_war_exploded"  # Replace with your actual server endpoint
DEVICE_ID = "raspi001"

# Serial port configuration (if reading data from a microcontroller over USB-TTL)
SERIAL_PORT = "/dev/ttyUSB0"
BAUDRATE = 115200

# Time intervals (seconds)
DATA_UPLOAD_INTERVAL = 60    # How often to post sensor data
COMMAND_CHECK_INTERVAL = 10  # How often to check for watering commands

# If you need an API path for posting data
DATA_POST_PATH = "/DataReceiver"    # e.g. /DataReceiver
# If you need an API path for getting commands
COMMAND_GET_PATH = "/WaterCommand"  # e.g. /CheckCommand?deviceId=xxx
