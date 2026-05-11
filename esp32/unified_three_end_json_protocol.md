# 智慧灌溉系统三端统一 JSON 协议说明

## 1. 文档目的

本文档用于统一 **STM32、ESP32、网站端（后端/前端）** 三端之间的数据传输内容与 JSON 结构，便于后续联调、接口实现、日志排查和协议扩展。

本协议优先考虑以下目标：

- 字段命名统一
- 语义清晰
- 三端职责边界明确
- 支持上报、下发、回执、告警、按键、本地事件等场景
- 便于后期扩展 AI、自动策略、OTA、日志诊断等功能

---

## 2. 三端角色说明

### 2.1 STM32

负责：

- 传感器采集
- 执行器控制
- 本地按键检测
- 底层状态机与安全保护
- 硬件故障与异常检测

### 2.2 ESP32

负责：

- 与 STM32 通过 UART 通信
- 对数据进行接收、解析、转发
- 连接 Wi-Fi 与网站后端通信
- 本地 UI / 灯效 / 音频交互
- 将网站命令下发到 STM32
- 回传网络状态、云连接状态、本地交互状态

### 2.3 网站端

负责：

- 设备数据展示
- 历史数据存储
- 用户下发控制命令
- 参数配置管理
- 告警展示与记录
- 设备与植物绑定关系管理

---

## 3. 协议统一原则

### 3.1 命名规范

- 全部使用 `snake_case`
- 字段名尽量使用完整英文单词
- 状态字段与数值字段分开
- 时间统一使用：`YYYY-MM-DD HH:mm:ss`

### 3.2 标识字段

- `device_id`：设备唯一编号，硬件侧主标识
- `plant_id`：植物业务编号，网站侧使用，可为空
- `trace_id`：消息链路编号，用于命令和回执匹配

### 3.3 通用包格式

除极简串口场景外，建议三端尽量统一使用以下消息外层结构：

```json
{
  "msg_type": "telemetry_report",
  "device_id": "MCU01",
  "plant_id": "",
  "trace_id": "20260409-000001",
  "timestamp": "2026-04-09 15:00:00",
  "require_ack": false,
  "payload": {}
}
```

### 3.4 通用字段说明

| 字段 | 含义 |
|---|---|
| `msg_type` | 消息类型 |
| `device_id` | 设备编号 |
| `plant_id` | 植物编号，可选 |
| `trace_id` | 消息跟踪编号 |
| `timestamp` | 消息时间 |
| `require_ack` | 是否要求回执 |
| `payload` | 消息正文 |

---

## 4. STM32 -> ESP32 消息结构

STM32 上传给 ESP32 的内容以“**真实设备状态**”为核心，建议分为以下 6 类消息。

### 4.1 实时传感器数据上报 `telemetry_report`

用于上传环境与设备采集值。

```json
{
  "msg_type": "telemetry_report",
  "device_id": "MCU01",
  "plant_id": "",
  "trace_id": "20260409-000101",
  "timestamp": "2026-04-09 15:00:00",
  "require_ack": false,
  "payload": {
    "temperature": 27.0,
    "humidity": 51.0,
    "soil_moisture": 0.0,
    "light": 3802,
    "water_level": null,
    "water_status": "normal",
    "battery": null
  }
}
```

字段说明：

- `temperature`：温度
- `humidity`：空气湿度
- `soil_moisture`：土壤湿度
- `light`：光照值
- `water_level`：数值型水位，暂时没有可传 `null`
- `water_status`：水位状态，如 `low` / `normal` / `high`
- `battery`：电池电量或电压，暂时没有可传 `null`

---

### 4.2 执行器状态上报 `actuator_status_report`

用于上报真实执行结果，而非仅表示“命令已收到”。

```json
{
  "msg_type": "actuator_status_report",
  "device_id": "MCU01",
  "plant_id": "",
  "trace_id": "20260409-000102",
  "timestamp": "2026-04-09 15:00:03",
  "require_ack": false,
  "payload": {
    "pump_on": true,
    "valve_on": false,
    "fill_light_on": false,
    "fan_on": false,
    "watering_active": true,
    "remaining_duration_sec": 8,
    "current_stage": "watering"
  }
}
```

---

### 4.3 设备运行状态上报 `device_status_report`

用于表示当前模式和系统整体工作状态。

```json
{
  "msg_type": "device_status_report",
  "device_id": "MCU01",
  "plant_id": "",
  "trace_id": "20260409-000103",
  "timestamp": "2026-04-09 15:00:05",
  "require_ack": false,
  "payload": {
    "mode": "auto",
    "system_state": "running",
    "allow_watering": true,
    "low_power": false,
    "locked": false,
    "control_source": "auto"
  }
}
```

---

### 4.4 硬件异常与告警上报 `alert_report`

用于主动上传故障、告警、异常状态。

```json
{
  "msg_type": "alert_report",
  "device_id": "MCU01",
  "plant_id": "",
  "trace_id": "20260409-000104",
  "timestamp": "2026-04-09 15:01:10",
  "require_ack": true,
  "payload": {
    "alert_code": "LOW_WATER_LEVEL",
    "alert_level": "warning",
    "alert_message": "Water level is low",
    "sensor_name": "water_level",
    "current_value": null,
    "suggestion": "Please refill water tank"
  }
}
```

推荐 `alert_level`：

- `info`
- `warning`
- `error`
- `critical`

---

### 4.5 本地配置回执 `config_ack_report`

用于响应来自 ESP32 的配置或控制命令执行结果。

```json
{
  "msg_type": "config_ack_report",
  "device_id": "MCU01",
  "plant_id": "",
  "trace_id": "20260409-000105",
  "timestamp": "2026-04-09 15:01:20",
  "require_ack": false,
  "payload": {
    "ack_type": "set_threshold",
    "ack_status": "success",
    "result_code": 0,
    "result_message": "Threshold updated successfully",
    "effective_time": "2026-04-09 15:01:20"
  }
}
```

推荐 `ack_status`：

- `success`
- `failed`
- `ignored`
- `queued`

---

### 4.6 本地按键与交互事件上报 `key_event_report`

用于记录用户本地按键操作与触发结果。

```json
{
  "msg_type": "key_event_report",
  "device_id": "MCU01",
  "plant_id": "",
  "trace_id": "20260409-000106",
  "timestamp": "2026-04-09 15:02:00",
  "require_ack": false,
  "payload": {
    "key_id": "key_water",
    "key_name": "watering_button",
    "event_type": "short_press",
    "key_state": "released",
    "trigger_action": "start_watering",
    "action_result": "success",
    "control_source": "local"
  }
}
```

推荐 `event_type`：

- `press`
- `release`
- `short_press`
- `long_press`
- `double_click`
- `repeat_press`

---

## 5. ESP32 -> STM32 消息结构

ESP32 下发给 STM32 的内容以“**控制、配置、同步、上下文**”为核心，建议分为以下 5 类消息。

### 5.1 即时控制命令 `control_command`

用于直接控制执行器或当前动作。

```json
{
  "msg_type": "control_command",
  "device_id": "MCU01",
  "plant_id": "",
  "trace_id": "20260409-000201",
  "timestamp": "2026-04-09 15:05:00",
  "require_ack": true,
  "payload": {
    "cmd": "water_control",
    "action": "start",
    "duration_sec": 10,
    "force": false
  }
}
```

常见 `cmd`：

- `water_control`
- `pump_control`
- `valve_control`
- `fill_light_control`
- `fan_control`
- `self_check`
- `clear_alert`

---

### 5.2 模式切换命令 `mode_command`

用于改变 STM32 主控制逻辑。

```json
{
  "msg_type": "mode_command",
  "device_id": "MCU01",
  "plant_id": "",
  "trace_id": "20260409-000202",
  "timestamp": "2026-04-09 15:05:20",
  "require_ack": true,
  "payload": {
    "mode": "manual",
    "reason": "remote_user_switch",
    "force": true
  }
}
```

推荐 `mode`：

- `auto`
- `manual`
- `debug`
- `eco`
- `protection`
- `offline_local`
- `remote_managed`

---

### 5.3 参数配置命令 `config_command`

用于修改阈值、周期、校准值等参数。

```json
{
  "msg_type": "config_command",
  "device_id": "MCU01",
  "plant_id": "",
  "trace_id": "20260409-000203",
  "timestamp": "2026-04-09 15:05:40",
  "require_ack": true,
  "payload": {
    "config_type": "threshold",
    "config": {
      "soil_moisture_min": 35.0,
      "soil_moisture_max": 65.0,
      "watering_duration_sec": 10,
      "watering_interval_min": 30,
      "water_level_alert_threshold": 20.0
    }
  }
}
```

---

### 5.4 同步类信息 `sync_info`

用于保持 STM32 与上层系统在时间和版本上的一致性。

```json
{
  "msg_type": "sync_info",
  "device_id": "MCU01",
  "plant_id": "",
  "trace_id": "20260409-000204",
  "timestamp": "2026-04-09 15:06:00",
  "require_ack": false,
  "payload": {
    "sync_time": "2026-04-09 15:06:00",
    "config_version": "cfg_v1.2",
    "strategy_version": "strategy_v1.0",
    "plant_profile": {
      "plant_name": "mint_01",
      "species": "mint"
    }
  }
}
```

---

### 5.5 命令上下文信息 `command_context`

用于帮助 STM32 处理优先级、冲突和回执匹配。

```json
{
  "msg_type": "command_context",
  "device_id": "MCU01",
  "plant_id": "",
  "trace_id": "20260409-000205",
  "timestamp": "2026-04-09 15:06:20",
  "require_ack": false,
  "payload": {
    "command_source": "web",
    "priority": "high",
    "allow_override": true,
    "require_ack": true,
    "operator_id": "user_001",
    "remark": "User manually starts watering from web"
  }
}
```

推荐 `command_source`：

- `web`
- `local_ui`
- `voice`
- `key`
- `auto`
- `maintenance`

---

## 6. ESP32 -> 网站端 消息结构

ESP32 向网站端上传的数据建议在 STM32 原始数据基础上做统一封装，主要包括以下 5 类。

### 6.1 设备综合状态上报 `device_full_report`

用于网站展示设备实时情况，建议作为主要上报格式。

```json
{
  "msg_type": "device_full_report",
  "device_id": "MCU01",
  "plant_id": "plant_001",
  "trace_id": "20260409-000301",
  "timestamp": "2026-04-09 15:10:00",
  "require_ack": false,
  "payload": {
    "telemetry": {
      "temperature": 27.0,
      "humidity": 51.0,
      "soil_moisture": 0.0,
      "light": 3802,
      "water_level": null,
      "water_status": "normal",
      "battery": null
    },
    "actuator_status": {
      "pump_on": false,
      "valve_on": false,
      "fill_light_on": false,
      "fan_on": false,
      "watering_active": false,
      "remaining_duration_sec": 0
    },
    "device_status": {
      "mode": "auto",
      "system_state": "running",
      "allow_watering": true,
      "low_power": false,
      "locked": false,
      "control_source": "auto"
    },
    "network_status": {
      "wifi_connected": true,
      "cloud_connected": true,
      "softap_enabled": false,
      "ip": "192.168.1.110",
      "rssi": -55
    }
  }
}
```

---

### 6.2 心跳与在线状态 `heartbeat_report`

用于网站判定设备是否在线。

```json
{
  "msg_type": "heartbeat_report",
  "device_id": "MCU01",
  "plant_id": "plant_001",
  "trace_id": "20260409-000302",
  "timestamp": "2026-04-09 15:10:30",
  "require_ack": false,
  "payload": {
    "online": true,
    "wifi_connected": true,
    "cloud_connected": true,
    "uptime_sec": 3600,
    "firmware_version": "esp32_v1.0.0",
  }
}
```

---

### 6.3 告警与事件上报 `event_report`

用于网站记录重要事件与故障。

```json
{
  "msg_type": "event_report",
  "device_id": "MCU01",
  "plant_id": "plant_001",
  "trace_id": "20260409-000303",
  "timestamp": "2026-04-09 15:11:00",
  "require_ack": true,
  "payload": {
    "event_type": "alert",
    "event_code": "LOW_WATER_LEVEL",
    "event_level": "warning",
    "event_message": "Water level is low",
    "origin": "stm32"
  }
}
```

推荐 `event_type`：

- `alert`
- `button`
- `network`
- `command_ack`
- `restart`
- `upgrade`

---

### 6.4 命令执行回执 `command_ack_report`

网站下发命令后，ESP32 应回传命令是否转发、STM32 是否执行成功。

```json
{
  "msg_type": "command_ack_report",
  "device_id": "MCU01",
  "plant_id": "plant_001",
  "trace_id": "20260409-000304",
  "timestamp": "2026-04-09 15:11:20",
  "require_ack": false,
  "payload": {
    "origin_trace_id": "20260409-000401",
    "cmd": "water_control",
    "ack_stage": "stm32_executed",
    "ack_status": "success",
    "result_code": 0,
    "result_message": "Watering started successfully"
  }
}
```

推荐 `ack_stage`：

- `server_received`
- `esp32_received`
- `stm32_forwarded`
- `stm32_executed`
- `completed`

---

### 6.5 本地交互事件上报 `local_interaction_report`

用于网站感知本地按钮、语音、页面操作等。

```json
{
  "msg_type": "local_interaction_report",
  "device_id": "MCU01",
  "plant_id": "plant_001",
  "trace_id": "20260409-000305",
  "timestamp": "2026-04-09 15:11:50",
  "require_ack": false,
  "payload": {
    "interaction_type": "button",
    "interaction_source": "stm32",
    "interaction_name": "watering_button",
    "event_type": "short_press",
    "action": "start_watering",
    "action_result": "success"
  }
}
```

---

## 7. 网站端 -> ESP32 消息结构

网站端向 ESP32 下发的数据以“**控制、配置、运维、配网、管理**”为核心，建议分为以下 5 类。

### 7.1 用户控制命令 `remote_control_command`

```json
{
  "msg_type": "remote_control_command",
  "device_id": "MCU01",
  "plant_id": "plant_001",
  "trace_id": "20260409-000401",
  "timestamp": "2026-04-09 15:15:00",
  "require_ack": true,
  "payload": {
    "cmd": "water_control",
    "action": "start",
    "duration_sec": 10,
    "operator_id": "user_001"
  }
}
```

---

### 7.2 参数配置命令 `remote_config_command`

```json
{
  "msg_type": "remote_config_command",
  "device_id": "MCU01",
  "plant_id": "plant_001",
  "trace_id": "20260409-000402",
  "timestamp": "2026-04-09 15:15:30",
  "require_ack": true,
  "payload": {
    "config_type": "threshold",
    "config": {
      "soil_moisture_min": 35.0,
      "soil_moisture_max": 65.0,
      "watering_duration_sec": 12,
      "watering_interval_min": 20
    },
    "operator_id": "user_001"
  }
}
```

---

### 7.3 网络配置命令 `network_config_command`

联网成功后，可由网站远程修改网络参数。

```json
{
  "msg_type": "network_config_command",
  "device_id": "MCU01",
  "plant_id": "plant_001",
  "trace_id": "20260409-000403",
  "timestamp": "2026-04-09 15:16:00",
  "require_ack": true,
  "payload": {
    "ssid": "Home_WiFi",
    "password": "12345678",
    "server_host": "101.200.161.110",
    "server_port": 8080,
    "upload_interval_sec": 10
  }
}
```

---

### 7.4 运维命令 `maintenance_command`

```json
{
  "msg_type": "maintenance_command",
  "device_id": "MCU01",
  "plant_id": "plant_001",
  "trace_id": "20260409-000404",
  "timestamp": "2026-04-09 15:16:30",
  "require_ack": true,
  "payload": {
    "cmd": "restart_device",
    "reason": "remote_maintenance",
    "operator_id": "admin_001"
  }
}
```

常见 `cmd`：

- `restart_device`
- `upload_logs`
- `clear_cache`
- `factory_reset`
- `self_check`
- `ota_prepare`

---

### 7.5 设备绑定与业务信息 `binding_info_command`

```json
{
  "msg_type": "binding_info_command",
  "device_id": "MCU01",
  "plant_id": "plant_001",
  "trace_id": "20260409-000405",
  "timestamp": "2026-04-09 15:17:00",
  "require_ack": false,
  "payload": {
    "plant_name": "mint_01",
    "species": "mint",
    "user_id": "user_001",
    "display_name": "Living Room Mint"
  }
}
```

---

## 8. 回执统一结构建议

无论是 STM32、ESP32 还是网站端，只要需要回执，建议统一使用以下结构：

```json
{
  "msg_type": "ack",
  "device_id": "MCU01",
  "plant_id": "plant_001",
  "trace_id": "20260409-000501",
  "timestamp": "2026-04-09 15:20:00",
  "require_ack": false,
  "payload": {
    "origin_trace_id": "20260409-000401",
    "ack_status": "success",
    "ack_stage": "completed",
    "result_code": 0,
    "result_message": "Command executed successfully"
  }
}
```

---

## 9. 第一版必须实现的消息

建议优先实现以下最小闭环：

### 9.1 STM32 -> ESP32

- `telemetry_report`
- `actuator_status_report`
- `device_status_report`
- `config_ack_report`
- `key_event_report`
- `alert_report`

### 9.2 ESP32 -> STM32

- `control_command`
- `mode_command`
- `config_command`
- `sync_info`
- `command_context`

### 9.3 ESP32 -> 网站端

- `device_full_report`
- `heartbeat_report`
- `event_report`
- `command_ack_report`

### 9.4 网站端 -> ESP32

- `remote_control_command`
- `remote_config_command`
- `network_config_command`
- `maintenance_command`

---

## 10. 实施建议

### 10.1 STM32 侧

- 优先保证上传“真实执行状态”和“真实告警状态”
- 本地按键事件必须单独上报，不要只在本地处理
- 所有需要执行的配置命令都要有回执

### 10.2 ESP32 侧

- 不建议简单透传，应负责协议整理与标准化
- 网站侧上报建议整合成 `device_full_report`
- 网站命令下发后要形成完整回执链路

### 10.3 网站端

- 以 `device_id` 作为硬件唯一键
- `plant_id` 用于业务展示和绑定
- 网站展示应区分“命令已发送”和“设备已执行”

---

## 11. 后续扩展建议

未来可继续扩展：

- OTA 状态消息
- 音频/语音状态消息
- 屏幕页面状态消息
- AI 策略推荐消息
- 历史数据批量上传消息
- 本地缓存重传消息

---

## 12. 总结

本协议建议将三端数据流统一为：

- **STM32 上传真实采集、真实执行、真实异常、本地按键事件**
- **ESP32 负责整合、联网、转发、回执、上下文管理**
- **网站端负责远程控制、参数管理、展示与记录**

推荐后续开发时，先按本说明完成消息类型与字段统一，再进入代码实现与数据库 DTO 设计阶段。