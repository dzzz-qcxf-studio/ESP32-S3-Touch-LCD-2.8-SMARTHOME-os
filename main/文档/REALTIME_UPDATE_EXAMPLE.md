# 实时数据更新示例

## 回调机制原理

### 系统架构

```
应用层              数据层              UI层
(main.c等)          (smart_ui_data)     (LVGL_Example)

传感器读取          
  │                                      
  ├──> ui_update_environment()          
       │                                
       └──> smart_ui_update_env_data()
              │
              ├─ 更新 g_env_data
              │
              └─ 触发回调: g_update_callback()
                           │
                           └──> smart_ui_refresh_callback()
                                  │
                                  └─ refresh_all_ui_elements()
                                       └─ 立即刷新UI显示

同时运行:
定时器 (每500ms)
  └──> smart_ui_tick_cb()
         └─ refresh_all_ui_elements()
              └─ 定期兜底刷新
```

### 双重更新机制

1. **立即更新（回调触发）**：
   - 当调用 `ui_update_xxx()` 函数时
   - 数据层更新后立即调用回调函数
   - UI **马上**刷新，无延迟

2. **定期更新（定时器）**：
   - 每500ms自动刷新一次
   - 作为兜底机制，防止遗漏更新
   - 即使不调用更新函数，UI也会定期刷新

## 使用示例

### 1. 在传感器任务中更新环境数据

```c
#include "LVGL_Example.h"

void temperature_sensor_task(void *pvParameters)
{
    while(1) {
        // 读取传感器
        float temperature = read_temperature_sensor();
        uint8_t humidity = read_humidity_sensor();
        
        // 更新UI - 会立即触发刷新
        ui_update_environment(temperature, humidity);
        
        // 每5秒更新一次
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

### 2. 在WiFi任务中更新系统状态

```c
#include "LVGL_Example.h"

void wifi_monitor_task(void *pvParameters)
{
    while(1) {
        if (wifi_is_connected()) {
            // 获取WiFi信息
            char ssid[32];
            wifi_get_ssid(ssid, sizeof(ssid));
            int8_t rssi = wifi_get_rssi();
            
            // 获取系统信息
            float voltage = BAT_Get_Volts();
            
            // 更新UI - 立即刷新
            ui_update_system(ssid, rssi, "v1.0.0", voltage);
        } else {
            // WiFi断开
            ui_update_system("未连接", 0, "v1.0.0", BAT_Get_Volts());
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

### 3. 在设备控制任务中更新房间状态

```c
#include "LVGL_Example.h"

void device_manager_task(void *pvParameters)
{
    while(1) {
        // 遍历所有房间
        for (uint8_t room_idx = 0; room_idx < 6; room_idx++) {
            // 扫描该房间的设备
            uint8_t total = get_total_devices(room_idx);
            uint8_t online = get_online_devices(room_idx);
            
            // 更新UI - 立即刷新
            ui_update_room(room_idx, total, online);
        }
        
        // 每2秒扫描一次
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
```

### 4. 在能耗监控任务中更新能耗数据

```c
#include "LVGL_Example.h"

static float daily_energy_kwh = 0.0f;

void energy_monitor_task(void *pvParameters)
{
    while(1) {
        // 读取当前功率
        float current_power_watts = read_power_meter();
        
        // 累加能耗（假设每秒更新一次）
        daily_energy_kwh += (current_power_watts / 1000.0f) / 3600.0f;
        
        // 更新UI - 立即刷新
        ui_update_energy(daily_energy_kwh);
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

### 5. 在安防系统中更新安防状态

```c
#include "LVGL_Example.h"

void security_monitor_task(void *pvParameters)
{
    while(1) {
        // 检查安防状态
        if (all_sensors_armed()) {
            ui_update_security("全部布防");
        } else if (no_sensors_armed()) {
            ui_update_security("撤防");
        } else {
            ui_update_security("部分布防");
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

## 完整应用示例

### main.c 中创建所有任务

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "LVGL_Example.h"

void app_main(void)
{
    // ... 硬件初始化 ...
    
    // 启动LVGL UI
    smart_ui_main();
    
    // 创建数据更新任务
    xTaskCreate(temperature_sensor_task, "temp_task", 2048, NULL, 5, NULL);
    xTaskCreate(wifi_monitor_task, "wifi_task", 2048, NULL, 5, NULL);
    xTaskCreate(device_manager_task, "device_task", 2048, NULL, 5, NULL);
    xTaskCreate(energy_monitor_task, "energy_task", 2048, NULL, 5, NULL);
    xTaskCreate(security_monitor_task, "security_task", 2048, NULL, 5, NULL);
    
    // ... 其他初始化 ...
}
```

## 关键要点

### ✅ 正确做法

1. **在各个任务中调用 `ui_update_xxx()` 函数**
   - 数据改变时立即调用
   - 会自动触发UI刷新
   - 无需手动操作LVGL控件

2. **利用回调机制实现实时更新**
   - 数据更新 → 回调触发 → UI立即刷新
   - 延迟极低，用户体验好

3. **定时器作为兜底机制**
   - 即使忘记调用更新函数
   - 定时器也会定期刷新UI
   - 确保显示不会长期错误

### ❌ 错误做法

1. **直接操作LVGL控件**
   ```c
   // ❌ 不要这样做！
   extern lv_obj_t *env_value_label;  // 跨文件访问UI对象
   lv_label_set_text_fmt(env_value_label, "..."); // 违反模块化
   ```

2. **不调用更新函数**
   ```c
   // ❌ 只修改数据，不通知UI
   extern smart_ui_env_data_t g_env_data;
   g_env_data.temperature = 25.0f;  // UI不会刷新！
   ```

3. **在非LVGL任务中直接操作UI**
   ```c
   // ❌ LVGL不是线程安全的！
   void some_task(void *pvParameters) {
       lv_label_set_text(...);  // 可能导致崩溃
   }
   ```

## 性能考虑

- **更新频率**：建议每个数据源1-5秒更新一次
- **回调开销**：非常小，只是函数调用
- **定时器开销**：500ms一次，对性能影响极小
- **线程安全**：`ui_update_xxx()` 函数内部会正确处理

## 调试技巧

### 验证更新是否生效

```c
#include "esp_log.h"

static const char *TAG = "UI_UPDATE";

void test_update_task(void *pvParameters)
{
    int counter = 0;
    while(1) {
        // 模拟温度变化
        float temp = 20.0f + (counter % 10);
        uint8_t humidity = 50 + (counter % 30);
        
        ESP_LOGI(TAG, "Updating temp=%.1f, humidity=%u", temp, humidity);
        ui_update_environment(temp, humidity);
        
        counter++;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
```

在串口监视器中应该看到：
```
I (xxx) UI_UPDATE: Updating temp=20.0, humidity=50
I (xxx) UI_UPDATE: Updating temp=21.0, humidity=51
...
```

同时UI上的温度数值应该每2秒变化一次。

## 总结

**当前为什么没有实时更新？**
- 因为没有任何地方调用 `ui_update_xxx()` 函数
- 数据一直是初始化时的值（5个设备，3个在线）
- 只有定时器在读取和显示这个固定值

**如何实现实时更新？**
1. 在您的应用任务中调用 `ui_update_xxx()` 函数
2. 数据层会自动触发回调
3. UI会立即刷新显示
4. 定时器作为兜底保证

**推荐的集成方式：**
- 在传感器任务、WiFi任务、设备管理任务等地方
- 当数据改变时调用对应的 `ui_update_xxx()` 函数
- 系统会自动处理UI刷新，您无需关心LVGL细节
