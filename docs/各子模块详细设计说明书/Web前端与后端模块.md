## Web 前端与后端模块

### 5.1 模块定位

Web 前后端模块为系统的远程可视化平台与用户交互界面，主要负责：

- 实时/历史数据展示；
- 灌溉控制与策略参数设置；
- 多设备管理与用户权限控制；
- 与树莓派/ESP32 进行 HTTP/MQTT 通信。

该模块采用前后端分离架构，前端基于 Vue.js 框架，后端使用 Flask（Python）构建。

### 5.2 前端架构设计

#### 5.2.1 技术栈

- 框架：Vue.js 3 + Vite
- UI 组件库：Element Plus + ECharts
- 状态管理：Pinia
- 路由管理：Vue Router
- 网络请求：Axios（封装 API 调用）

#### 5.2.2 页面设计

| 页面       | 功能说明                             |
| ---------- | ------------------------------------ |
| 登录页     | 用户登录/注册、Token 获取            |
| 控制台     | 展示植物状态、策略控制、实时反馈图表 |
| 历史数据页 | 支持时间区间查询与多维指标可视化     |
| 策略页     | 当前策略查看与修改、AI推荐策略接入   |
| 用户中心   | 多设备管理、权限绑定、偏好设置       |

#### 5.2.3 数据展示

- 使用 ECharts 展示：
  - 湿度/温度/光照变化曲线
  - 灌溉行为时间轴
  - AI 策略触发记录图

### 5.3 后端系统设计

#### 5.3.1 技术栈

- 框架：Flask + Gunicorn + Nginx
- 数据库：MySQL + SQLAlchemy
- API 风格：RESTful + JSON 数据格式
- 权限认证：JWT + OAuth2（可拓展）
- 模块组织：Blueprints + 分层 MVC 架构

#### 5.3.2 数据库设计

| 表名            | 字段                                          | 描述             |
| --------------- | --------------------------------------------- | ---------------- |
| users           | id, username, password, email, role           | 用户信息         |
| devices         | id, user_id, name, location, status           | 设备与植物元数据 |
| data_logs       | id, device_id, timestamp, soil, temp, light   | 实时数据记录     |
| irrigation_logs | id, device_id, start_time, duration           | 灌溉行为日志     |
| ai_strategies   | id, device_id, model_id, timestamp, threshold | AI 策略记录      |

#### 5.3.3 API 接口示例

```
GET /api/device/<id>/data
POST /api/device/<id>/irrigate
POST /api/user/login
GET /api/strategy/current
POST /api/strategy/manual-update
```

#### 5.3.4 模块职责

| 模块     | 描述                        |
| -------- | --------------------------- |
| auth     | 用户身份认证，Token 管理    |
| data     | 数据上传与下发、缓存管理    |
| strategy | AI 策略管理、接口桥接树莓派 |
| notify   | 灌溉通知与系统异常报警      |

### 5.4 通信接口与部署逻辑

#### 5.4.1 与 ESP32 通信

- 使用 HTTP/HTTPS 协议，ESP32 定时上传 JSON 数据包
- Flask 提供 `/api/device/report` 接口解析数据并存入数据库

#### 5.4.2 与树莓派通信

- 使用 MQTT 或 REST API 通信
- 每日推送 AI 建议策略：`POST /api/strategy/ai`
- 提供策略查询接口供前端展示

#### 5.4.3 系统部署架构

```
用户浏览器 → Nginx（反向代理）
                  ↓
           Gunicorn + Flask
                  ↓
               MySQL DB
               + 日志文件
```

支持部署于树莓派本地，或远程服务器。

### 5.5 异常与安全机制

| 异常类型             | 应对措施                           |
| -------------------- | ---------------------------------- |
| 数据包字段缺失       | 返回 400 错误并记录日志            |
| Token 过期/伪造      | 强制登出，返回 401 响应            |
| 用户操作越权         | 拒绝并返回 403 权限不足            |
| 后端崩溃或数据库断连 | 使用 Supervisor 自动重启，记录日志 |

### 5.6 可测试性说明

- 前端使用 Cypress/VitePress 编写自动化测试用例
- 后端使用 pytest + mock + coverage 进行接口回归测试
- 提供 Swagger 在线文档用于接口调试
- 所有数据表支持版本日志追踪（可恢复误操作）