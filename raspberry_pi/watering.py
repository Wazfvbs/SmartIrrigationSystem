# watering.py

def water_on(ser):
    """
    Sends 'p1' to the microcontroller or takes any action that starts watering.
    """
    cmd = "p1\n"
    ser.write(cmd.encode('utf-8'))

def water_off(ser):
    """
    Sends 'p0' to the microcontroller or takes any action that stops watering.
    """
    cmd = "p0\n"
    ser.write(cmd.encode('utf-8'))
