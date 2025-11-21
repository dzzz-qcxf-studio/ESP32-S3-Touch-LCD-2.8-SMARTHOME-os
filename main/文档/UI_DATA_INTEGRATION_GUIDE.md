# UI 数据集成指南 / UI Data Integration Guide

## 概述 / Overview

本指南说明如何将真实的数据源（传感器、网络接口等）集成到智能家居 UI 中。

This guide explains how to integrate real data sources (sensors, network interfaces, etc.) into the smart home UI.

---

## 架构 / Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    数据源 / Data Sources                     │
│  (传感器/Sensors, 网络/Network, 数据库/Database, etc.)      │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              smart_ui_data 模块 / Data Module               │
│  - 数据存储 / Data Storage                                  │
│  - 数据验证 / Data Validation                               │
│  - 回调触发 / Callback Triggering                           │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│           LVGL_Example UI 模块 / UI Module                  │
│  - UI 更新 / UI Update                                      │
│  - 数据显示 / Data Display                                  │
│  - 用户交互 / User Interaction                              │
└─────────────────────────────────────────────────────────────┘
```

---

## 快速开始 / Quick Start

### 1. 环境数据更新 / Update Environment Data

```c
#include "LVGL_Example.h"

// 从传感器读取温度和湿度
float temperature = read_temperature_sensor();
uint8_t humidity = read_humidity_sensor();

// 更新 UI
ui_update_environment(temperature, humidity);
```

### 2. 能耗数据更新 / Update Energy Data

```c
// 从能耗计读取今日能耗
float daily_energy = read_energy_meter();

// 更新 UI
ui_update_energy(daily_energy);
```

### 3. 安防状态更新 / Update Security Status

```c
// 获取安防系统状态
const char *security_status = get_security_status();  // "全部布防", "客厅有人", etc.

// 更新 UI
ui_update_security(security_status);
```

### 4. 房间设备状态更新 / Update Room Device Status

```c
// 更新客厅（房间 0）的设备状态
uint8_t total_devices = 5;      // 总设备数
uint8_t online_devices = 3;     // 在线设备数

ui_update_room(0, total_devices, online_devices);
```

### 5. 系统状态更新 / Update System Status

```c
// 获取 Wi-Fi 状态
const char *wifi_status = "家庭网络";
int8_t rssi = -55;  // Wi-Fi 信号强度 (dBm)

// 获取固件版本
const char *fw_version = "v1.0.0";

// 获取电源电压
float power_voltage = 4.8f;

// 更新 UI
ui_update_system(wifi_status, rssi, fw_version, power_voltage);
```

---

## 数据结构 / Data Structures

### 环境数据 / Environment Data

```c
typedef struct {
    float temperature;      // 温度 (°C)
    uint8_t humidity;       // 湿度 (%)
    bool is_valid;          // 数据是否有效
} smart_ui_env_data_t;
```

### 能耗数据 / Energy Data

```c
typedef struct {
    float daily_energy;     // 今日能耗 (kWh)
    bool is_valid;          // 数据是否有效
} smart_ui_energy_data_t;
```

### 安防数据 / Security Data

```c
typedef struct {
    char status[32];        // 安防状态文本
    bool is_valid;          // 数据是否有效
} smart_ui_security_data_t;
```

### 房间设备状态 / Room Device Status

```c
typedef struct {
    uint8_t total_devices;  // 总设备数
    uint8_t online_devices; // 在线设备数
    bool is_valid;          // 数据是否有效
} smart_ui_room_status_t;
```

### 系统状态 / System Status

```c
typedef struct {
    char wifi_status[48];   // Wi-Fi 状态文本
    int8_t wifi_rssi;       // Wi-Fi 信号强度 (dBm)
    char firmware_version[16]; // 固件版本
    float power_voltage;    // 电源电压 (V)
    bool wifi_valid;        // Wi-Fi 数据是否有效
    bool fw_valid;          // 固件数据是否有效
    bool power_valid;       // 电源数据是否有效
} smart_ui_system_data_t;
```

---

## 集成示例 / Integration Examples

### 示例 1: 定时更新传感器数据 / Example 1: Periodic Sensor Update

```c
// 在某个定时任务中
void sensor_task(void *arg)
{
    while (1) {
        // 读取传感器
        float temp = read_temperature_sensor();
        uint8_t humidity = read_humidity_sensor();
        
        // 更新 UI
        ui_update_environment(temp, humidity);
        
        // 延迟 5 秒
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

### 示例 2: Wi-Fi 连接状态更新 / Example 2: Wi-Fi Connection Status Update

```c
// 在 Wi-Fi 事件处理中
void wifi_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ui_update_system("家庭网络", -50, "v1.0.0", 4.8f);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ui_update_system("未连接", 0, "v1.0.0", 4.8f);
    }
}
```

### 示例 3: 从网络获取数据 / Example 3: Fetch Data from Network

```c
// 在网络任务中
void network_task(void *arg)
{
    while (1) {
        // 从服务器获取数据
        smart_home_data_t data = fetch_from_server();
        
        if (data.valid) {
            ui_update_environment(data.temperature, data.humidity);
            ui_update_energy(data.daily_energy);
            ui_update_security(data.security_status);
            
            // 更新所有房间
            for (int i = 0; i < 6; i++) {
                ui_update_room(i, data.rooms[i].total, data.rooms[i].online);
            }
        }
        
        // 延迟 10 秒
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
```

---

## 数据有效性 / Data Validity

所有数据结构都包含 `is_valid` 标志。当 `is_valid` 为 `false` 时，UI 将显示"暂无数据"。

All data structures include an `is_valid` flag. When `is_valid` is `false`, the UI will display "暂无数据" (no data).

```c
// 示例：只有在数据有效时才更新
smart_ui_env_data_t data = {
    .temperature = 25.5f,
    .humidity = 60,
    .is_valid = 1  // 标记数据有效
};
smart_ui_update_env_data(&data);
```

---

## 回调机制 / Callback Mechanism

当数据更新时，UI 会自动刷新。这是通过回调机制实现的：

When data is updated, the UI automatically refreshes. This is implemented through a callback mechanism:

```c
// 在 smart_ui_main() 中已注册回调
smart_ui_register_update_callback(smart_ui_tick_cb);

// 当调用任何 smart_ui_update_*() 函数时，会自动触发回调
// 回调函数读取数据并更新 UI 标签
```

---

## 最佳实践 / Best Practices

1. **定期更新** / Regular Updates
   - 使用 FreeRTOS 任务定期更新数据
   - 避免在 UI 线程中进行阻塞操作

2. **数据验证** / Data Validation
   - 始终设置 `is_valid` 标志
   - 检查数据范围的合理性

3. **错误处理** / Error Handling
   - 如果数据获取失败，设置 `is_valid = 0`
   - UI 将自动显示"暂无数据"

4. **性能优化** / Performance Optimization
   - 避免过于频繁的更新
   - 只在数据实际改变时才调用更新函数

---

## 故障排除 / Troubleshooting

### UI 不更新 / UI Not Updating

- 检查 `is_valid` 标志是否设置为 `1`
- 确认调用了正确的 `ui_update_*()` 函数
- 检查数据是否在合理范围内

### 显示"暂无数据" / Showing "No Data"

- 这是正常行为，表示数据无效或未设置
- 确保数据源正确提供数据
- 检查 `is_valid` 标志

---

## 相关文件 / Related Files

- `smart_ui_data.h` - 数据模块头文件 / Data module header
- `smart_ui_data.c` - 数据模块实现 / Data module implementation
- `LVGL_Example.h` - UI 头文件（包含便利函数） / UI header with convenience functions
- `LVGL_Example.c` - UI 实现 / UI implementation

---

## 许可证 / License

MIT License
