# coding=utf-8
#!/usr/bin/env python3
import serial
import time
import sqlite3
import os
import datetime
import requests
import threading
from gtts import gTTS

# --------------------- 配置参数 ---------------------
SERIAL_PORT = "/dev/ttyUSB0"        # 串口设备
BAUDRATE = 115200                   # 波特率

DB_PATH = "sensor_data.db"          # 数据库文件路径

# 云服务器地址及 API 路径（请根据实际情况修改）
BASE_URL = "http://172.20.10.2:8080/flowers_war_exploded"
DATA_POST_PATH = "/DataReceiver"
COMMAND_GET_PATH = "/WaterCommand"

# 设备 ID (树莓派)
DEVICE_ID = "raspi001"

# 时间间隔（秒）
DATA_UPLOAD_INTERVAL = 5           # 每30秒上传数据
COMMAND_CHECK_INTERVAL = 10         # 每10秒检查一次控制命令（可扩展）

# 浇水控制阈值（示例值）
SOIL_MOISTURE_THRESHOLD0 = 20       # 当土壤湿度低于此值时，启动浇水
SOIL_MOISTURE_THRESHOLD1 = 50       # 当土壤湿度低于此值时，启动浇水
SOIL = 1
# DeepSeek API 配置（请替换为你的有效 API 密钥和URL）
DEEPOSEEK_API_URL = "https://api.deepseek.com/v1"
DEEPOSEEK_API_KEY = "sk-59ce491a073e4d0ba525623a6d75a13e"

# --------------------- 全局变量 ---------------------
latest_data = None   # 全局保存最新的传感器数据（字符串）
data_lock = threading.Lock()  # 锁保护

# --------------------- 数据库相关函数 ---------------------
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
        water REAL,
        light REAL
    );
    """)
    conn.commit()
    conn.close()

def save_data_1min(temp, humi, soil, water
                   #, light
                   ):
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute("INSERT INTO raw_data_1min(temp, humi, soil, water) VALUES(?,?,?,?)",
              (temp, humi, soil, water))
    conn.commit()
    conn.close()

def save_data_1h(temp, humi, soil, water, light):
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute("INSERT INTO raw_data_1h(temp, humi, soil, water, light) VALUES(?,?,?,?,?)",
              (temp, humi, soil, water, light))
    conn.commit()
    conn.close()

def clean_old_data_1min():
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute("DELETE FROM raw_data_1min WHERE ts < datetime('now', '-15 day')")
    conn.commit()
    conn.close()

# --------------------- 数据上传模块 ---------------------
def uploader_loop():
    while True:
        time.sleep(DATA_UPLOAD_INTERVAL)  # 每 30 秒上传一次
        with data_lock:
            data_line = latest_data
        if data_line:
            try:
                # 假设数据格式为 "T=26,H=41,soil=2,water=0,light=1455"
                parts = data_line.split(',')
                temp = float(parts[0].split('=')[-1])
                humi = float(parts[1].split('=')[-1])
                soil = float(parts[2].split('=')[-1])
                water = float(parts[3].split('=')[-1])
                light = float(parts[4].split('=')[-1])
            except Exception as e:
                print("[Uploader] Parse error:", e)
                continue
            payload = {
                "deviceId": DEVICE_ID,
                "temp": temp,
                "humi": humi,
                "soil": soil,
                "water": water,
                #"light": light
            }
            try:
                url = BASE_URL + DATA_POST_PATH
                r = requests.post(url, data=payload, timeout=5)
                print("[Uploader] Cloud upload response:", r.text)
            except Exception as e:
                print("[Uploader] Upload error:", e)
            save_data_1min(temp, humi, soil, water
                           #, light
                           )
            clean_old_data_1min()

# --------------------- 自动浇水控制模块 ---------------------
def watering_control_loop(ser, ser_write_lock):
    global SOIL
    while True:
        with data_lock:
            data_line = latest_data
        if data_line:
            try:
                parts = data_line.split(',')
                soil = float(parts[2].split('=')[-1])
            except Exception as e:
                print("[Watering] Parse error:", e)
                time.sleep(1)
                continue
            # 根据土壤湿度判断
            if soil < SOIL_MOISTURE_THRESHOLD0 and SOIL == 1:
                command = "MCU01:p1:\n"
                SOIL = 0
                with ser_write_lock:
                    ser.write(command.encode('utf-8'))
                    print("[Watering] Sent command:", command.strip())
            elif soil >= SOIL_MOISTURE_THRESHOLD1 and SOIL == 0:
                command = "MCU01:p0:\n"
                SOIL = 1
                with ser_write_lock:
                    ser.write(command.encode('utf-8'))
                    print("[Watering] Sent command:", command.strip())
                command = "MCU01:oled:normal\n"
                with ser_write_lock:
                    ser.write(command.encode('utf-8'))
                    print("[Watering] Sent command:", command.strip())
        time.sleep(1)

# --------------------- AI 对话模块 ---------------------
def ai_conversation_loop(ser, ser_write_lock):
    while True:
        user_input = input("请输入唤醒命令(输入'ai'启动AI对话)：")
        if user_input.strip().lower() == "ai":
            # 切换 OLED 为聆听状态
            with ser_write_lock:
                ser.write("MCU01:oled:listen\n".encode('utf-8'))
            print("[AI] 正在聆听，请输入您的问题：")
            user_question = input("您的问题：")
            with data_lock:
                sensor_info = latest_data or ""
            query_text = f"{sensor_info}\n用户问：{user_question}"
            payload = {
                "model": "deepseek-chat",
                "messages": [
                    {"role": "system", "content": "你是一个智能植物助手。你需要为用户回答问题，以下是用户所养的植物的相关数据和问题"},
                    {"role": "user", "content": query_text}
                ]
            }
            headers = {
                "Authorization": f"Bearer {DEEPOSEEK_API_KEY}",
                "Content-Type": "application/json"
            }
            try:
                response = requests.post(DEEPOSEEK_API_URL, json=payload, headers=headers, timeout=10)
                data = response.json()
                answer = data["choices"][0]["message"]["content"]
            except Exception as e:
                answer = "对不起，无法回答。"
                print("[AI] DeepSeek API error:", e)
                with ser_write_lock:
                    ser.write("MCU01:oled:normal\n".encode('utf-8'))
            print("[AI] 答案：", answer)
            # 切换 OLED 为回答（talk）模式
            with ser_write_lock:
                ser.write("MCU01:oled:talk\n".encode('utf-8'))
            # 使用 gTTS 播放语音
            tts = gTTS(text=answer, lang='zh')
            audio_file = "response.mp3"
            tts.save(audio_file)
            os.system(f"mpg123 {audio_file}")
            # 最后恢复为 normal 模式
            with ser_write_lock:
                ser.write("MCU01:oled:normal\n".encode('utf-8'))
        time.sleep(1)

# --------------------- 串口数据接收模块 ---------------------
def serial_receiver_loop(ser):
    global latest_data
    while True:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                with data_lock:
                    latest_data = line
                print("[Receiver] Received:", line)
        except Exception as e:
            print("[Receiver] Error:", e)
        time.sleep(1)

# --------------------- 主程序 ---------------------
def main():
    global latest_data
    init_db()
    ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1)
    if ser.isOpen():
        print("[Main] Serial port opened:", SERIAL_PORT)
    else:
        print("[Main] Failed to open serial port.")
        return

    # 串口写入锁，防止多线程写串口冲突
    ser_write_lock = threading.Lock()

    # 启动串口接收线程
    threading.Thread(target=serial_receiver_loop, args=(ser,), daemon=True).start()

    # 启动数据上传线程
    threading.Thread(target=uploader_loop, daemon=True).start()

    # 启动自动浇水控制线程
    threading.Thread(target=watering_control_loop, args=(ser, ser_write_lock), daemon=True).start()

    # 启动 AI 对话线程
    threading.Thread(target=ai_conversation_loop, args=(ser, ser_write_lock), daemon=True).start()

    # 主循环：打印最新数据，便于监控
    while True:
        with data_lock:
            data = latest_data
        if data:
            print("[Main] Latest sensor data:", data)
        time.sleep(5)

if __name__ == "__main__":
    main()
