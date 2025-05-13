# 🌱 STM32 Firmware - Smart Irrigation System

本模块为智慧浇水系统的底层传感与控制单元，基于 STM32 微控制器实现，负责环境数据采集、执行控制命令（如启动浇水）、并通过 UART 与 ESP32 进行通信。

---

## 📌 功能概述

- 周期性采集环境数据：
  - 温度 / 湿度（DHT11 / DHT22）
  - 土壤湿度（模拟电压）
  - 光照强度（模拟光敏电阻）
  - 水位状态（浮球或导电式）
  - 当前电量（分压 ADC 检测）
  - 按键状态（单击 / 长按）

- 控制执行装置：
  - 电磁阀 / 水泵控制（GPIO 驱动）

- UART 通信协议（使用 JSON）：
  - 向 ESP32 发送传感器数据
  - 接收 ESP32 下发的控制命令并执行

- 异常状态反馈与保护机制：
  - 电量过低 / 无水 / 传感器异常

---

## 📊 数据格式（JSON）

### → 发送至 ESP32 的数据包格式：

```json
{
  "device": "stm32",
  "temperature": 24.5,
  "humidity": 61.2,
  "soil_moisture": 38.7,
  "light": 524,
  "water_level": 1,
  "button_state": "none",
  "power": 82
}
```

### ← 接收 ESP32 下发的控制命令格式：

```
json复制编辑{ "command": "start_watering" }
{ "command": "stop_watering" }
{ "command": "set_threshold", "soil_moisture_min": 30.0, "soil_moisture_max": 60.0 }
{ "command": "ping" }
```

------

## 🔧 硬件连接参考

| 功能模块     | 接口方式 | 建议引脚（示例） | 外设说明               |
| ------------ | -------- | ---------------- | ---------------------- |
| 温湿度传感器 | GPIO     | PA0              | DHT11 / DHT22          |
| 土壤湿度     | ADC      | PA1 (ADC_IN1)    | 模拟输出土壤湿度模块   |
| 光敏电阻     | ADC      | PA2 (ADC_IN2)    | 分压光敏模块           |
| 水位检测     | GPIO     | PA3              | 浮球开关 / 电导检测    |
| 电量检测     | ADC      | PA4 (ADC_IN4)    | 分压取样电池电压       |
| 控制水泵     | GPIO     | PB0              | N-MOS 控制继电器或驱动 |
| 按钮输入     | EXTI     | PA5              | 兼容短按/长按逻辑      |
| 串口通信     | UART     | USART1：PA9/PA10 | 与 ESP32 通信          |



------

## 📂 项目结构说明

```
bash复制编辑stm32/
├── Core/                   # STM32CubeMX 生成的核心代码
│   ├── Inc/                # 头文件目录
│   └── Src/                # 主体源文件目录
├── Drivers/                # STM32 HAL 驱动库
├── Utilities/              # JSON 解析模块（如 cJSON）
├── Makefile / .ioc         # 编译与工程配置文件
└── README.md               # 当前文档
```

------

## 🧠 关键模块说明

- `main.c`：主循环逻辑（采集 → 发送 → 接收命令）
- `sensor.c`：传感器采集与数据封装
- `uart_comm.c`：UART 通信处理与 JSON 封装/解析
- `control.c`：水泵控制、阈值管理、异常处理

------

## ✅ 开发环境

- **芯片型号**：STM32F103C8T6（可选其他型号）
- **开发工具**：STM32CubeMX + Keil / VS Code + ARM GCC
- **通信协议**：UART @ 115200 baud + JSON
- **外设驱动**：HAL Driver（基于 STM32Cube）

------

## 📌 注意事项

- 电源建议使用稳压 3.3V/5V 电源模块，注意电流裕量
- 所有模拟信号需滤波并避免干扰
- 水泵控制使用 N-MOS 或继电器，并做隔离保护
- 使用看门狗（IWDG）防止系统卡死
- 电量检测应经过电阻分压后送入 ADC，注意校准系数

------

## 📬 后续计划

- 支持多组植物传感器扩展（I2C 编址 / 多通道 ADC）
- 增加断电保存本地配置（Flash / EEPROM）
- 支持 OTA 升级与版本报告机制
- 支持无线独立工作（结合 LoRa / NRF 模块）

------

## 📞 联系与反馈

如需协助、反馈问题或协作开发，请通过本项目主仓库提交 Issue 或联系开发者。