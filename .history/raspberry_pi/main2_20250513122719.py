# coding=utf-8
#!/usr/bin/env python3
# coding=utf-8
import socket
import threading
import time
import sqlite3
import os
import datetime
import requests
from gtts import gTTS

# --------------------- 配置参数 ---------------------
SERVER_PORT = 5000                # 树莓派监听的端口
BUFFER_SIZE = 1024                # Socket 接收缓冲区大小

DB_PATH = "sensor_data.db"        # 数据库文件路径

# 云服务器地址及 API 路径
BASE_URL = "http://172.20.10.2:8080/flowers_war_exploded"
DATA_POST_PATH = "/DataReceiver"
COMMAND_GET_PATH = "/WaterCommand"

DEVICE_ID = "raspi001"

# 时间间隔（秒）
DATA_UPLOAD_INTERVAL = 5          # 每 5 秒上传数据（示例中较短）
COMMAND_CHECK_INTERVAL = 10       # 每 10 秒检查控制命令

# 浇水控制阈值（示例）
SOIL_MOISTURE_THRESHOLD0 = 20     # 当土壤湿度低于此值启动浇水
SOIL_MOISTURE_THRESHOLD1 = 50     # 当土壤湿度达到此值停止浇水

# DeepSeek API 配置
DEEPOSEEK_API_URL = "https://api.deepseek.com/v1"
DEEPOSEEK_API_KEY = ""

# --------------------- 全局变量 ---------------------
latest_data = None                # 全局保存最新传感器数据（字符串）
data_lock = threading.Lock()      # 数据保护锁

# 全局 Socket 连接变量及锁（用于与 MCU 通信）
socket_conn = None
socket_conn_lock = threading.Lock()

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
        water REAL,
        light REAL
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

def save_data_1min(temp, humi, soil, water, light):
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute("INSERT INTO raw_data_1min(temp, humi, soil, water, light) VALUES(?,?,?,?,?)",
              (temp, humi, soil, water, light))
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

# --------------------- Socket 通信模块 ---------------------
def socket_receiver_loop():
    """
    创建 TCP Socket 服务器，等待 MCU 通过 ESP01S 建立连接，并不断接收数据。
    数据格式假设为文本，每条数据以换行符结束。
    """
    global latest_data, socket_conn
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(("", SERVER_PORT))
    sock.listen(1)
    print(f"[Socket] Server listening on port {SERVER_PORT}")

    while True:
        try:
            conn, addr = sock.accept()
            with socket_conn_lock:
                socket_conn = conn
            print(f"[Socket] Accepted connection from {addr}")
            # 接收数据循环
            while True:
                data = conn.recv(BUFFER_SIZE)
                if not data:
                    print("[Socket] Connection closed by client")
                    break
                data_line = data.decode("utf-8", errors="ignore").strip()
                if data_line:
                    with data_lock:
                        latest_data = data_line
                    print(f"[Socket] Received: {data_line}")
        except Exception as e:
            print(f"[Socket] Error: {e}")
        with socket_conn_lock:
            socket_conn = None
        time.sleep(1)

def send_command(cmd: str):
    """
    发送命令到 MCU，通过 socket_conn 发送
    """
    global socket_conn
    full_cmd = cmd.strip() + "\n"
    with socket_conn_lock:
        if socket_conn:
            try:
                socket_conn.sendall(full_cmd.encode("utf-8"))
                print(f"[Socket] Sent command: {full_cmd.strip()}")
            except Exception as e:
                print(f"[Socket] Send error: {e}")
        else:
            print("[Socket] No connection available to send command.")

# --------------------- 数据上传模块 ---------------------
def uploader_loop():
    while True:
        time.sleep(DATA_UPLOAD_INTERVAL)
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
                # "light": light   # 如有需要
            }
            try:
                url = BASE_URL + DATA_POST_PATH
                r = requests.post(url, data=payload, timeout=5)
                print("[Uploader] Cloud upload response:", r.text)
            except Exception as e:
                print("[Uploader] Upload error:", e)
            save_data_1min(temp, humi, soil, water, light)
            clean_old_data_1min()

# --------------------- 自动浇水控制模块 ---------------------
def watering_control_loop():
    global SOIL
    # 此处使用全局最新数据，判断土壤湿度并通过 send_command 发送命令给 MCU
    # SOIL 用于判断当前状态，防止重复发送命令
    SOIL = 1  # 初始设定为 1（停止状态）
    while True:
        with data_lock:
            data_line = latest_data
        if data_line:
            try:
                parts = data_line.split(',')
                soil_val = float(parts[2].split('=')[-1])
            except Exception as e:
                print("[Watering] Parse error:", e)
                time.sleep(1)
                continue
            if soil_val < SOIL_MOISTURE_THRESHOLD0 and SOIL == 1:
                send_command("MCU01:p1:")
                SOIL = 0
            elif soil_val >= SOIL_MOISTURE_THRESHOLD1 and SOIL == 0:
                send_command("MCU01:p0:")
                SOIL = 1
                send_command("MCU01:oled:normal")
        time.sleep(1)

# --------------------- AI 对话模块 ---------------------
def ai_conversation_loop():
    while True:
        user_input = input("请输入唤醒命令(输入'ai'启动AI对话)：")
        if user_input.strip().lower() == "ai":
            send_command("MCU01:oled:listen")
            print("[AI] 正在聆听，请输入您的问题：")
            user_question = input("您的问题：")
            with data_lock:
                sensor_info = latest_data or ""
            query_text = f"{sensor_info}\n用户问：{user_question}"
            payload = {
                "model": "deepseek-chat",
                "messages": [
                    {"role": "system", "content": "你是一个智能植物助手。请根据提供的数据回答用户问题。"},
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
                send_command("MCU01:oled:normal")
            print("[AI] 答案：", answer)
            send_command("MCU01:oled:talk")
            tts = gTTS(text=answer, lang='zh')
            audio_file = "response.mp3"
            tts.save(audio_file)
            os.system(f"mpg123 {audio_file}")
            send_command("MCU01:oled:normal")
        time.sleep(1)

# --------------------- 主程序 ---------------------
def main():
    global latest_data
    init_db()

    # 启动 Socket 数据接收线程
    threading.Thread(target=socket_receiver_loop, daemon=True).start()

    # 启动数据上传线程
    threading.Thread(target=uploader_loop, daemon=True).start()

    # 启动自动浇水控制线程
    threading.Thread(target=watering_control_loop, daemon=True).start()

    # 启动 AI 对话线程
    threading.Thread(target=ai_conversation_loop, daemon=True).start()

    # 主循环：定时打印最新传感器数据，便于监控
    while True:
        with data_lock:
            data = latest_data
        if data:
            print("[Main] Latest sensor data:", data)
        time.sleep(5)

if __name__ == "__main__":
    main()
