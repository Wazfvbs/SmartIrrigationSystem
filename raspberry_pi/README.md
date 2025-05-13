# 🌿 Smart Irrigation - Raspberry Pi Module

树莓派模块是智慧浇水系统的核心智能大脑，负责数据存储、策略训练与反馈控制，通过强化学习实现科学、自适应的植物灌溉决策。

---

## 📦 功能概览

| 模块                  | 描述                                                         |
| --------------------- | ------------------------------------------------------------ |
| 📥 数据接收模块        | 接收 ESP32 发送的 JSON 数据（温湿度、光照、土壤湿度等）      |
| 🗄 数据存储模块        | 将数据存储至本地（CSV 或 SQLite）以便后续分析与模型训练      |
| 🧹 数据预处理模块      | 对数据进行异常值过滤、平滑等处理，构建强化学习训练所需状态向量 |
| 🤖 强化学习模块        | 使用 Q-Learning/DQN 等算法训练最优浇水策略，输出阈值区间     |
| 🔁 控制反馈模块        | 将训练结果转换为标准 JSON 控制指令发送回 ESP32，实现阈值更新 |
| 🌐 本地API模块（可选） | 提供 Flask 网页接口，用于实时查看数据、手动训练、导出日志等  |

---

## 📊 数据格式

### 📨 接收数据格式（来自 ESP32）

```json
{
  "timestamp": "2025-05-06T14:32:12",
  "plant_id": "001",
  "plant_name": "小绿",
  "species": "绿萝",
  "temperature": 24.5,
  "humidity": 61.2,
  "soil_moisture": 38.7,
  "light": 524,
  "water_level": 1,
  "power": 82,
  "mode": "default"
}
```

### 📤 输出命令格式（发给 ESP32）

```
json复制编辑{
  "command": "update_threshold",
  "soil_moisture_min": 36.0,
  "soil_moisture_max": 70.0,
  "from": "raspberry"
}
```

------

## 🧠 强化学习概述

- **状态空间**：`[soil_moisture, temperature, humidity]`
- **动作空间**：更新土壤湿度阈值区间（增加/减少/保持）
- **奖励函数**：根据水资源使用效率与植物健康度动态调整
- **目标**：最大化植物健康、最小化水资源浪费

------

## 📁 项目结构建议

```
bash复制编辑raspberry_pi/
├── main.py                # 主程序入口
├── data/
│   └── sensor_data.csv    # 数据存储
├── model/
│   └── q_learning.py      # 强化学习模块
├── utils/
│   ├── preprocessor.py    # 数据预处理逻辑
│   └── logger.py          # 日志工具
├── api/
│   └── flask_server.py    # 可选的本地Web API
├── config.py              # 配置文件
└── README.md              # 当前说明文档
```

------

## 🚀 快速开始

1. 安装依赖：

```
pip install flask pandas numpy
```

1. 启动服务监听：

```
python main.py
```

1. （可选）启动 Flask API：

```
python api/flask_server.py
```

------

## 🔧 TODO List

-  完善 Q-learning 模型调参逻辑
-  添加异常处理与超时控制机制
-  增加多植物支持与模型个性化配置
-  集成 matplotlib 绘图显示训练效果
-  实现 SQLite 数据存储替代 CSV

------

## 📜 许可证

本项目遵循 MIT License，欢迎学习与扩展。如需商用，请联系作者授权。