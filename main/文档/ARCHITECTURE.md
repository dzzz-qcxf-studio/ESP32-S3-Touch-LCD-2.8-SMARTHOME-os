# 智能家居 UI 架构 / Smart Home UI Architecture

## 模块化设计 / Modular Design

```
┌──────────────────────────────────────────────────────────────────┐
│                      应用层 / Application Layer                   │
│  (传感器驱动、网络模块、业务逻辑 / Sensors, Network, Logic)      │
└────────────────────────┬─────────────────────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────────────────┐
│                    数据接口层 / Data Interface Layer              │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │  LVGL_Example.h - 便利函数 / Convenience Functions         │  │
│  │  - ui_update_environment()                                 │  │
│  │  - ui_update_energy()                                      │  │
│  │  - ui_update_security()                                    │  │
│  │  - ui_update_room()                                        │  │
│  │  - ui_update_system()                                      │  │
│  └────────────────────────────────────────────────────────────┘  │
└────────────────────────┬─────────────────────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────────────────┐
│                   数据管理层 / Data Management Layer              │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │  smart_ui_data.h/c - 数据模块 / Data Module               │  │
│  │  - 数据存储 / Data Storage                                 │  │
│  │  - 数据更新 / Data Update                                  │  │
│  │  - 数据获取 / Data Retrieval                               │  │
│  │  - 回调管理 / Callback Management                          │  │
│  └────────────────────────────────────────────────────────────┘  │
└────────────────────────┬─────────────────────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────────────────┐
│                     UI 渲染层 / UI Rendering Layer                │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │  LVGL_Example.c - UI 实现 / UI Implementation             │  │
│  │  - 界面创建 / UI Creation                                  │  │
│  │  - 数据显示 / Data Display                                 │  │
│  │  - 事件处理 / Event Handling                               │  │
│  │  - 样式管理 / Style Management                             │  │
│  └────────────────────────────────────────────────────────────┘  │
└────────────────────────┬─────────────────────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────────────────┐
│                    LVGL 图形库 / LVGL Graphics Library            │
└──────────────────────────────────────────────────────────────────┘
```

---

## 文件结构 / File Structure

```
main/LVGL_UI/
├── LVGL_Example.h              # UI 头文件 + 数据接口
├── LVGL_Example.c              # UI 实现
├── smart_ui_data.h             # 数据模块头文件
├── smart_ui_data.c             # 数据模块实现
├── UI_DATA_INTEGRATION_GUIDE.md # 数据集成指南
└── ARCHITECTURE.md             # 本文件
```

---

## 核心概念 / Core Concepts

### 1. 数据结构 / Data Structures

每个数据类别都有对应的结构体，包含：
- 数据字段 / Data fields
- 有效性标志 / Validity flag (`is_valid`)

```c
typedef struct {
    float temperature;
    uint8_t humidity;
    bool is_valid;
} smart_ui_env_data_t;
```

### 2. 数据更新流程 / Data Update Flow

```
应用层获取数据 / Application gets data
    ↓
调用 ui_update_*() 函数 / Call ui_update_*()
    ↓
数据模块存储数据 / Data module stores data
    ↓
触发回调函数 / Trigger callback
    ↓
UI 读取数据并更新显示 / UI reads and updates display
```

### 3. 回调机制 / Callback Mechanism

- 数据模块维护一个回调函数指针
- 每次数据更新时，自动调用回调函数
- 回调函数（`smart_ui_tick_cb`）读取所有数据并更新 UI

```c
// 注册回调
smart_ui_register_update_callback(smart_ui_tick_cb);

// 更新数据时自动触发回调
smart_ui_update_env_data(&data);  // 内部调用 g_update_callback()
```

---

## 数据流向 / Data Flow

### 场景 1: 传感器数据更新 / Scenario 1: Sensor Data Update

```
传感器任务 / Sensor Task
    ↓
读取温度/湿度 / Read temperature/humidity
    ↓
调用 ui_update_environment() / Call ui_update_environment()
    ↓
smart_ui_data 模块存储 / Data module stores
    ↓
触发 smart_ui_tick_cb() / Trigger callback
    ↓
UI 更新环境卡片 / UI updates environment card
    ↓
显示新数据 / Display new data
```

### 场景 2: 网络数据更新 / Scenario 2: Network Data Update

```
网络任务 / Network Task
    ↓
从服务器获取数据 / Fetch from server
    ↓
调用多个 ui_update_*() 函数 / Call multiple ui_update_*()
    ↓
数据模块存储所有数据 / Data module stores all
    ↓
触发回调 / Trigger callback
    ↓
UI 更新所有相关卡片 / UI updates all cards
    ↓
显示最新数据 / Display latest data
```

---

## 接口说明 / Interface Documentation

### 应用层接口 / Application Layer Interface

在 `LVGL_Example.h` 中提供便利函数：

```c
// 环境数据
void ui_update_environment(float temp, uint8_t humidity);

// 能耗数据
void ui_update_energy(float daily_kwh);

// 安防状态
void ui_update_security(const char *status);

// 房间设备状态
void ui_update_room(uint8_t room_idx, uint8_t total, uint8_t online);

// 系统状态
void ui_update_system(const char *wifi_status, int8_t rssi,
                      const char *fw_version, float voltage);
```

### 数据模块接口 / Data Module Interface

在 `smart_ui_data.h` 中定义：

```c
// 初始化
void smart_ui_data_init(void);

// 注册回调
void smart_ui_register_update_callback(smart_ui_data_callback_t callback);

// 更新函数
void smart_ui_update_env_data(const smart_ui_env_data_t *data);
void smart_ui_update_energy_data(const smart_ui_energy_data_t *data);
void smart_ui_update_security_data(const smart_ui_security_data_t *data);
void smart_ui_update_room_status(uint8_t room_index, const smart_ui_room_status_t *status);
void smart_ui_update_system_data(const smart_ui_system_data_t *data);

// 获取函数
const smart_ui_env_data_t* smart_ui_get_env_data(void);
const smart_ui_energy_data_t* smart_ui_get_energy_data(void);
const smart_ui_security_data_t* smart_ui_get_security_data(void);
const smart_ui_room_status_t* smart_ui_get_room_status(uint8_t room_index);
const smart_ui_system_data_t* smart_ui_get_system_data(void);
```

---

## 设计优势 / Design Advantages

### 1. 解耦 / Decoupling
- 应用层不需要了解 UI 实现细节
- UI 层不需要了解数据源
- 通过数据模块进行通信

### 2. 可维护性 / Maintainability
- 每个模块职责清晰
- 易于修改和扩展
- 便于测试

### 3. 可扩展性 / Extensibility
- 添加新的数据类型只需扩展数据结构
- 添加新的 UI 元素只需在 UI 层修改
- 不影响其他模块

### 4. 数据有效性 / Data Validity
- 每个数据都有有效性标志
- 无效数据自动显示"暂无数据"
- 提高用户体验

### 5. 自动更新 / Automatic Updates
- 回调机制自动触发 UI 更新
- 无需手动调用 UI 更新函数
- 减少错误

---

## 使用示例 / Usage Example

### 完整集成示例 / Complete Integration Example

```c
#include "LVGL_Example.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 传感器任务
void sensor_task(void *arg)
{
    while (1) {
        // 读取传感器
        float temp = read_temperature();
        uint8_t humidity = read_humidity();
        float energy = read_energy();
        
        // 更新 UI
        ui_update_environment(temp, humidity);
        ui_update_energy(energy);
        
        // 延迟
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// Wi-Fi 事件处理
void wifi_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_STA_CONNECTED) {
        ui_update_system("已连接", -50, "v1.0.0", 4.8f);
    }
}

// 主函数
void app_main(void)
{
    // 初始化 UI
    smart_ui_main();
    
    // 创建传感器任务
    xTaskCreate(sensor_task, "sensor", 2048, NULL, 5, NULL);
    
    // 注册 Wi-Fi 事件处理
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                               &wifi_event_handler, NULL);
}
```

---

## 扩展指南 / Extension Guide

### 添加新的数据类型 / Adding New Data Type

1. 在 `smart_ui_data.h` 中定义新的数据结构
2. 在 `smart_ui_data.c` 中添加存储和更新函数
3. 在 `LVGL_Example.h` 中添加便利函数
4. 在 `LVGL_Example.c` 中添加 UI 更新逻辑

### 添加新的 UI 元素 / Adding New UI Element

1. 在 `LVGL_Example.c` 中创建新的 UI 组件
2. 保存组件引用以便更新
3. 在 `smart_ui_tick_cb()` 中添加更新逻辑

---

## 性能考虑 / Performance Considerations

1. **内存占用** / Memory Usage
   - 数据模块使用固定大小的全局变量
   - 总占用约 200 字节

2. **更新频率** / Update Frequency
   - 建议 500ms 至 2000ms 更新一次
   - 避免过于频繁的更新

3. **线程安全** / Thread Safety
   - 当前实现不是线程安全的
   - 如需多线程，建议添加互斥锁

---

## 故障排除 / Troubleshooting

### 问题：UI 不显示数据 / Problem: UI Not Displaying Data

**原因** / Cause:
- 数据的 `is_valid` 标志为 0
- 数据未被正确更新

**解决方案** / Solution:
- 确保调用了 `ui_update_*()` 函数
- 检查 `is_valid` 标志是否设置为 1

### 问题：显示"暂无数据" / Problem: Showing "No Data"

**原因** / Cause:
- 这是正常行为
- 表示数据无效或未设置

**解决方案** / Solution:
- 检查数据源是否正确提供数据
- 确保数据的 `is_valid` 标志被正确设置

---

## 相关文档 / Related Documentation

- `UI_DATA_INTEGRATION_GUIDE.md` - 数据集成指南
- `smart_ui_data.h` - 数据模块 API 文档
- `LVGL_Example.h` - UI 接口文档

---

## 版本历史 / Version History

- v1.0 - 初始版本，完整的模块化架构
  - 数据模块实现
  - UI 更新回调机制
  - 便利函数接口
  - 集成指南

---

## 许可证 / License

MIT License
