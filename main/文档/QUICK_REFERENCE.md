# 快速参考 / Quick Reference

## 初始化 / Initialization

```c
#include "LVGL_Example.h"

// 在 main 中调用
smart_ui_main();
```

---

## 数据更新 API / Data Update API

### 环境数据 / Environment Data
```c
ui_update_environment(25.5f, 60);  // 温度(°C), 湿度(%)
```

### 能耗数据 / Energy Data
```c
ui_update_energy(3.25f);  // 今日能耗(kWh)
```

### 安防状态 / Security Status
```c
ui_update_security("全部布防");  // 状态文本
ui_update_security("客厅有人");
```

### 房间设备 / Room Devices
```c
// 房间索引: 0=客厅, 1=主卧, 2=次卧, 3=厨房, 4=书房, 5=车库
ui_update_room(0, 5, 3);  // 房间, 总设备数, 在线设备数
```

### 系统状态 / System Status
```c
ui_update_system("家庭网络", -50, "v1.0.0", 4.8f);
// Wi-Fi状态, RSSI(dBm), 固件版本, 电源电压(V)
```

---

## 常见集成模式 / Common Integration Patterns

### 模式 1: 定时更新 / Pattern 1: Periodic Update

```c
void sensor_task(void *arg) {
    while (1) {
        ui_update_environment(get_temp(), get_humidity());
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

### 模式 2: 事件驱动 / Pattern 2: Event-Driven

```c
void event_handler(void *arg, esp_event_base_t event_base,
                   int32_t event_id, void *event_data) {
    if (event_id == SOME_EVENT) {
        ui_update_security("新状态");
    }
}
```

### 模式 3: 网络获取 / Pattern 3: Network Fetch

```c
void network_task(void *arg) {
    while (1) {
        data_t data = fetch_from_server();
        if (data.valid) {
            ui_update_environment(data.temp, data.humidity);
            ui_update_energy(data.energy);
            // ... 更新其他数据
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
```

---

## 数据有效性 / Data Validity

- 当 `is_valid = 1` 时，UI 显示数据
- 当 `is_valid = 0` 时，UI 显示"暂无数据"
- 便利函数自动设置 `is_valid = 1`

---

## 房间索引映射 / Room Index Mapping

| 索引 | 房间 | Index | Room |
|------|------|-------|------|
| 0 | 客厅 | 0 | Living Room |
| 1 | 主卧 | 1 | Master Bedroom |
| 2 | 次卧 | 2 | Guest Bedroom |
| 3 | 厨房 | 3 | Kitchen |
| 4 | 书房 | 4 | Study |
| 5 | 车库 | 5 | Garage |

---

## 文件位置 / File Locations

```
main/LVGL_UI/
├── LVGL_Example.h          # 便利函数 / Convenience functions
├── LVGL_Example.c          # UI 实现 / UI implementation
├── smart_ui_data.h         # 数据模块 / Data module
├── smart_ui_data.c         # 数据实现 / Data implementation
├── ARCHITECTURE.md         # 架构文档 / Architecture docs
├── UI_DATA_INTEGRATION_GUIDE.md  # 集成指南 / Integration guide
└── QUICK_REFERENCE.md      # 本文件 / This file
```

---

## 常见错误 / Common Mistakes

❌ **错误** / Wrong:
```c
smart_ui_env_data_t data = {25.5f, 60, 1};
smart_ui_update_env_data(&data);
```

✅ **正确** / Correct:
```c
ui_update_environment(25.5f, 60);
```

---

❌ **错误** / Wrong:
```c
ui_update_room(0, 5, 3);  // 没有检查数据有效性
```

✅ **正确** / Correct:
```c
if (room_data_valid) {
    ui_update_room(0, 5, 3);
}
```

---

## 调试技巧 / Debugging Tips

1. **检查数据是否更新**
   ```c
   const smart_ui_env_data_t *data = smart_ui_get_env_data();
   printf("Temp: %.1f, Valid: %d\n", data->temperature, data->is_valid);
   ```

2. **检查 UI 是否响应**
   - 确保调用了 `smart_ui_main()`
   - 检查定时器是否运行

3. **检查数据范围**
   - 温度: -50 ~ 100°C
   - 湿度: 0 ~ 100%
   - 电压: 0 ~ 10V

---

## 性能指标 / Performance Metrics

| 指标 | 值 | Metric | Value |
|------|-----|--------|-------|
| 内存占用 | ~200 字节 | Memory | ~200 bytes |
| 更新延迟 | <50ms | Update latency | <50ms |
| 推荐更新频率 | 500ms-2s | Recommended frequency | 500ms-2s |

---

## 支持的数据类型 / Supported Data Types

- ✅ 浮点数 / Float
- ✅ 整数 / Integer
- ✅ 字符串 / String (max 32-48 chars)
- ✅ 布尔值 / Boolean

---

## 下一步 / Next Steps

1. 阅读 `ARCHITECTURE.md` 了解完整架构
2. 阅读 `UI_DATA_INTEGRATION_GUIDE.md` 了解集成方法
3. 在应用中调用 `ui_update_*()` 函数
4. 测试数据更新和 UI 显示

---

## 获取帮助 / Getting Help

- 查看 `UI_DATA_INTEGRATION_GUIDE.md` 中的示例
- 检查 `LVGL_Example.h` 中的函数文档
- 查看 `smart_ui_data.h` 中的数据结构定义
