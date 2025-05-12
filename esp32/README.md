
# 🌱 ESP32 智慧浇水系统

本项目基于 ESP32 实现智慧植物灌溉系统，配合 STM32、树莓派和服务器组成完整生态，实现自动感知环境、智能控制浇水、LED 状态反馈、语音对话等功能。

---

## 📌 项目简介

通过传感器采集温湿度、土壤湿度、水位、光照等环境数据，ESP32 负责数据处理与显示，并实现：
- 显示屏幕切换控制
- 按钮操作与模式切换
- 自动/手动浇水控制
- AI语音交互与命令识别
- LED状态反馈
- 与服务器、树莓派通信上传数据

---

## 🧠 系统架构图

```plaintext
+------------+       UART/I2C        +------------+       WiFi/MQTT        +------------+
|   STM32    |---------------------->|   ESP32    |----------------------->|  Server /  |
|  采集传感器数据  |                    |  数据处理 & 控制  |                    | Raspberry Pi |
+------------+                       +------------+                        +------------+
                                              |
                     +------------------------+------------------------+
                     |                        |                        |
               +------------+         +---------------+        +--------------+
               | OLED 显示屏 |         | 语音识别模块 |        | LED 灯带反馈 |
               +------------+         +---------------+        +--------------+
```

---

## 🔧 功能模块详解

### 📥 数据接收与解析
- 从 STM32 接收 JSON 格式环境数据
- 使用 `cJSON` 解析：温度、湿度、土壤湿度、水位、光照强度

### 📺 显示控制（基于 LVGL + ST7796）
- **日常模式**：显示植物信息与环境数据
- **浇水模式**：显示“浇水中”状态
- **AI 对话模式**：显示 AI 监听与回复信息

### 🎛️ 按钮控制
- 单击：开/关显示屏
- 双击：进入 AI 对话模式
- 长按：进入手动浇水模式

### 💧 浇水控制
- **自动模式**：土壤湿度低于阈值，自动浇水
- **手动模式**：长按按钮或通过语音命令启动浇水

### 🗣️ AI 对话功能
- 支持语音识别与平台交互（如 ChatGPT 接入）
- 用户可通过语音控制浇水、查询状态等

### 🌐 数据上传与通信
- 通过 HTTP / MQTT 协议将数据上传至服务器和树莓派
- 支持远程配置与数据监控

### 💡 LED 灯带控制
- 水位 LED 显示：高（绿）/低（红）
- 电量 LED 显示：高（绿）/低（红）

---

## 📁 项目目录结构

```plaintext
sample_project/
├── main/                      # 主业务逻辑代码
│   ├── main.c                 # 程序入口
│   ├── uart_receiver.c/h      # 串口数据接收与解析
│   ├── display_manager.c/h    # 屏幕显示控制（LVGL）
│   ├── button_manager.c/h     # 按钮事件识别
│   ├── voice_manager.c/h      # AI语音识别模块
│   ├── led_manager.c/h        # LED 控制模块
│   ├── wifi_manager.c/h       # WiFi 初始化管理
│   ├── app_mqtt.c/h           # MQTT通信（树莓派）
│   ├── upload_manager.c/h     # HTTP 上传（服务器）
│   └── config_manager.c/h     # 系统配置参数管理
├── components/                # 第三方组件库（如 LVGL）
├── sdkconfig                  # 配置文件
├── CMakeLists.txt             # 编译配置
└── README.md                  # 项目说明文档
```

---

## 🛠️ 编译与运行

### ✅ 环境要求
- ESP-IDF v5.0+
- Python 3.7+
- LVGL v9
- VSCode + ESP-IDF 插件推荐

### 📦 编译命令

```bash
idf.py set-target esp32
idf.py build
idf.py -p PORT flash monitor
```

---

## 📷 效果展示

> 💡 添加截图或 GIF 展示你的项目运行状态，包括显示屏内容、AI 对话界面等。

---

## 📄 License

本项目遵循 [MIT License](LICENSE)。

---

## 🙌 致谢

- [Espressif ESP-IDF](https://github.com/espressif/esp-idf)
- [LVGL 图形库](https://lvgl.io/)
- [cJSON 解析库](https://github.com/DaveGamble/cJSON)
- 语音平台 & AI 平台（如需标注）

---

## 📬 联系方式

欢迎交流与合作！  
✉️ 邮箱：your_email@example.com  
📺 Bilibili：@你的B站用户名  
