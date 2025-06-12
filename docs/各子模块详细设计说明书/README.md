# 模块详细设计说明书

## 一、文档目的

本说明书旨在对“智慧植物灌溉与监控系统”中的关键功能模块（STM32、ESP32、树莓派AI、Web前后端）进行逐一详细分析，明确其输入输出、功能职责、逻辑结构、状态转换、数据结构与接口规范，确保各模块具备良好的可实现性、可维护性与可测试性。

## 二、STM32 传感控制模块

### 2.1 功能概述

采集并处理传感器数据，执行水泵控制命令，与 ESP32 进行 UART 通信。

### 2.2 输入输出定义

| 类型 | 描述                                                       |
| ---- | ---------------------------------------------------------- |
| 输入 | UART指令串，GPIO中断（按键、ESP唤醒）、传感器模拟/数字信号 |
| 输出 | 结构化数据包（串口输出）、水泵控制GPIO、高低电平LED指示    |

### 2.3 核心流程图（文字简略）

1. 初始化传感器与定时器；
2. 每5秒触发ADC采样 → 计算平均值 → 封装为结构体；
3. 判断是否触发本地灌溉逻辑（应急模式）；
4. 通过串口发送数据帧至ESP32；
5. 监听来自ESP32的指令，进行控制逻辑切换。

### 2.4 数据结构设计（伪代码）

```
struct SensorData {
  float temperature;
  float humidity;
  float soil_moisture;
  int light;
  bool water_low;
  int battery;
};
```

## 三、ESP32 中继与交互模块

### 3.1 功能概述

承接 STM32 数据，处理本地控制逻辑（OLED、LED、语音、指令），与服务器及树莓派进行双向通信。

### 3.2 子任务划分

- Task 1：串口监听与数据上传
- Task 2：模式状态切换（通过按钮/语音）
- Task 3：OLED 数据轮播与动画更新
- Task 4：LED 灯带状态反馈
- Task 5：语音处理与在线对话（WIFI接入）

### 3.3 状态机示意（模式切换）

```
[default] --(voice/button)--> [watering]
   |                              |
   +--------<-- timeout ----------+
   |
   +--------(low battery)-------> [low_power]
```

### 3.4 关键数据包结构

```
{
  "device_id": "plant001",
  "status": "watering",
  "voice_mode": true,
  "threshold": {
    "min_moisture": 20,
    "max_moisture": 40
  }
}
```

## 四、树莓派 AI 模块

### 4.1 功能概述

接收历史与实时环境数据，运行强化学习模型，输出策略阈值调整建议。

### 4.2 模型框架

- 使用 Gym 环境构造状态空间 S = {soil, temp, humidity, light}
- 动作空间 A = {不变、提高阈值、降低阈值}
- 奖励函数：灌溉后湿度处于最佳范围获得正值，过度灌溉给予惩罚
- 学习策略：DQN + Experience Replay + 目标网络延迟更新

### 4.3 策略下发示例

```
{
  "strategy_id": "rl-0529v2",
  "threshold": {
    "min_moisture": 24,
    "max_moisture": 36
  }
}
```

### 4.4 模型版本控制

- 每次模型更新保留配置文件、权重文件、训练日志
- 所有策略发布需关联版本号与参数元数据

## 五、Web前后端模块

### 5.1 前端架构

- 基于 Vue.js 框架
- 使用 Vue Router 实现页面跳转：主页、设备管理页、控制页、社区页
- 图表组件：ECharts（历史曲线、对比图）
- 组件库：Element UI + Bootstrap

### 5.2 后端设计

- 基于 Flask + SQLAlchemy
- RESTful API 路由结构如下：

```
POST /api/login
GET  /api/plant/<id>/status
POST /api/control/send
POST /api/ai/update-threshold
GET  /api/plant/<id>/history
```

### 5.3 数据库表结构简述

| 表名       | 字段                                             | 描述           |
| ---------- | ------------------------------------------------ | -------------- |
| users      | id, username, password, role                     | 用户信息与权限 |
| devices    | id, user_id, plant_name, location                | 植物设备信息   |
| data_logs  | id, device_id, timestamp, soil, temp, humid...   | 实时/历史数据  |
| irrigation | id, device_id, start_time, duration, strategy_id | 灌溉记录       |

## 六、接口规范与调用示意

### 6.1 控制指令接口调用（伪例）

```
POST /api/control/send
{
  "device_id": "plant001",
  "action": "irrigate_now"
}
```

### 6.2 异常上报接口

```
POST /api/device/report
{
  "device_id": "plant001",
  "error_code": 302,
  "description": "battery low"
}
```

## 七、测试建议与约束

- 所有模块需支持独立部署测试
- 各模块支持 Debug 模式与日志输出（UART日志 / 文件日志）
- 模型评估应提供：收敛性曲线、误差分析图、动作分布变化图
- 前端与后端接口联调需引入 Swagger 或 Postman 自动化工具支持

> 本说明书作为系统核心模块工程化实现的重要依据，建议在后续开发过程中根据版本演进进行迭代维护。