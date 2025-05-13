# communicator.py

import requests
from config import BASE_URL, DEVICE_ID, DATA_POST_PATH, COMMAND_GET_PATH

def post_sensor_data(temp, humi, soil, water):
    """
    Sends sensor data to the remote server using an HTTP POST.
    Adjust parameters or JSON/form structure to match your JavaWeb endpoint.
    """
    url = BASE_URL + DATA_POST_PATH
    payload = {
        "deviceId": DEVICE_ID,
        "temp": str(temp),
        "humi": str(humi),
        "soil": str(soil),
        "water": str(water)
    }

    try:
        print(f"Sending data: {payload}")  # Output the data being sent for debugging
        resp = requests.post(url, data=payload, timeout=5)
        return resp.text
    except Exception as e:
        return f"Error posting data: {e}"

def get_watering_command():
    """
    Retrieves the latest watering command from the server.
    For example: p1 (start watering) or p0 (stop watering).
    Adjust parameters or query string as needed.
    """
    url = f"{BASE_URL}{COMMAND_GET_PATH}?deviceId={DEVICE_ID}"
    try:
        print(f"Sending GET request to: {url}")  # Log the URL being requested
        resp = requests.get(url, timeout=5)
        # Suppose the server returns a simple string like "p1" or "p0".
        return resp.text.strip()
    except Exception as e:
        return f"Error getting watering command: {e}"
