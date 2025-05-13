# main.py
import time
import threading
from config import DATA_UPLOAD_INTERVAL, COMMAND_CHECK_INTERVAL
from sensor_reader import init_serial, read_sensors
from communicator import post_sensor_data, get_watering_command
from watering import water_on, water_off
from voice_recognition import VoiceRecognition
from ai_interaction import AIInteraction
from text_to_speech import TextToSpeech
from sensor_operations import SensorOperations

class SmartWateringSystem:
    def __init__(self, api_key):
        self.voice_recognition = VoiceRecognition()
        self.ai_interaction = AIInteraction(api_key)
        self.text_to_speech = TextToSpeech()
        self.sensor_operations = SensorOperations()

    def handle_command(self, command):
        sensor_data = self.sensor_operations.get_sensor_data()
        if sensor_data:
            ai_response = self.ai_interaction.get_ai_response(command, sensor_data)
            print(f"AI回复: {ai_response}")
            self.text_to_speech.speak_text(ai_response)  # 播放AI回答
            self.sensor_operations.control_watering(ai_response)  # 根据AI的回答控制浇水

    def listen_for_commands(self):
        while True:
            if self.voice_recognition.listen_for_wake_word():  # 等待唤醒词
                command = self.voice_recognition.listen_for_command()  # 识别命令
                if command:
                    self.handle_command(command)  # 处理命令

def data_loop(ser):
    """
    Periodically reads sensor data and sends it to the remote server.
    """
    while True:
        sensor_data = read_sensors(ser)
        if sensor_data:
            temp = sensor_data.get("T", 0.0)
            humi = sensor_data.get("H", 0.0)
            soil = sensor_data.get("soil", 0.0)
            water = sensor_data.get("water", 0.0)
            result = post_sensor_data(temp, humi, soil, water)
            print(f"[DataLoop] Posted data -> {result}")
        else:
            print("[DataLoop] No sensor data received.")
        time.sleep(DATA_UPLOAD_INTERVAL)

def command_loop(ser):
    """
    Periodically checks the remote server for watering commands
    and sends actions to the microcontroller.
    """
    while True:
        cmd = get_watering_command()
        if cmd:
            print(f"[CommandLoop] Received command: {cmd}")
            if cmd.lower() == "p1":
                water_on(ser)
            elif cmd.lower() == "p0":
                water_off(ser)
        else:
            print("[CommandLoop] No command or request failed.")
        time.sleep(COMMAND_CHECK_INTERVAL)

def main():
    ser = init_serial()
    # Start threads for data upload and command check
    t_data = threading.Thread(target=data_loop, args=(ser,), daemon=True)
    t_cmd = threading.Thread(target=command_loop, args=(ser,), daemon=True)

    t_data.start()
    #t_cmd.start()
    api_key = "sk-59ce491a073e4d0ba525623a6d75a13e"  # 在这里填入你的 DeepSeek API 密钥
    system = SmartWateringSystem(api_key)

    # 启动语音命令监听线程
    t_command = threading.Thread(target=system.listen_for_commands, daemon=True)
    t_command.start()

    # Keep main thread alive
    while True:
        time.sleep(1)

if __name__ == "__main__":
    main()
