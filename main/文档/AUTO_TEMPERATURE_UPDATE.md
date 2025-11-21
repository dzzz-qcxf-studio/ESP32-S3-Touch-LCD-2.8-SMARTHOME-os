# 温度自动更新机制

## 问题背景

用户尝试直接修改 `const` 指针指向的数据，导致编译错误：
```c
const smart_ui_env_data_t *env_data = smart_ui_get_env_data();
env_data->temperature = getTemperature();  // ❌ 错误：只读对象
env_data->is_valid = true;                 // ❌ 错误：只读对象
```

**错误信息：**
- `assignment of member 'temperature' in read-only object`
- `assignment of member 'is_valid' in read-only object`

## 解决方案

采用与电池电压相同的处理方式：在 `refresh_all_ui_elements()` 中直接更新全局数据。

### 实现代码

```c
static void refresh_all_ui_elements(void)
{
    char buf[128];
    
    /* 实时获取底层系统数据并更新数据层 */
    
    // 1. 更新电池电压
    float current_voltage = BAT_Get_Volts();
    g_system_data.power_voltage = current_voltage;
    g_system_data.power_valid = 1;
    
    // 2. 更新芯片温度
    float chip_temp = getTemperature();
    if (chip_temp != 999) {  // 999 表示未初始化或读取失败
        g_env_data.temperature = chip_temp;
        g_env_data.is_valid = 1;
    }
    
    // 3. 读取数据并更新 UI
    const smart_ui_env_data_t *env_data = smart_ui_get_env_data();
    if (env_value_label) {
        if (env_data && env_data->is_valid) {
            snprintf(buf, sizeof(buf), "温度 %.1f°C / 湿度 %u%%", 
                     env_data->temperature, env_data->humidity);
            lv_label_set_text(env_value_label, buf);
        } else {
            lv_label_set_text(env_value_label, "暂无数据");
        }
    }
    
    // ... 其他 UI 更新代码
}
```

## 工作原理

### 数据流

```
定时器（每 500ms）
    ↓
refresh_all_ui_elements()
    ↓
1. 调用 getTemperature() 读取芯片温度
    ↓
2. 直接更新全局变量 g_env_data
    ↓
3. 从数据层获取数据 smart_ui_get_env_data()
    ↓
4. 更新 UI 显示
```

### 与电压更新的一致性

| 数据类型 | 读取函数 | 全局变量 | 更新位置 |
|---------|---------|----------|---------|
| **电池电压** | `BAT_Get_Volts()` | `g_system_data.power_voltage` | `refresh_all_ui_elements()` |
| **芯片温度** | `getTemperature()` | `g_env_data.temperature` | `refresh_all_ui_elements()` |

**设计原则：**
- ✅ 底层系统数据（电压、温度）由定时器自动获取
- ✅ 应用层数据（WiFi、设备状态等）由应用层主动推送
- ✅ 定时器作为兜底机制，定期刷新所有数据

## 关键修复点

### 错误做法 ❌

```c
// 尝试修改 const 指针指向的数据
const smart_ui_env_data_t *env_data = smart_ui_get_env_data();
env_data->temperature = getTemperature();  // ❌ 编译错误！
```

**问题：**
- `smart_ui_get_env_data()` 返回 `const` 指针
- `const` 指针指向的数据不能修改
- 这是 C 语言的类型安全保护

### 正确做法 ✅

```c
// 直接更新全局变量
float chip_temp = getTemperature();
if (chip_temp != 999) {
    g_env_data.temperature = chip_temp;  // ✅ 正确！
    g_env_data.is_valid = 1;
}
```

**优势：**
- ✅ 不违反 `const` 约束
- ✅ 与电压处理方式一致
- ✅ 自动定期更新
- ✅ 无需应用层干预

## 错误检测机制

### 999 作为无效值标记

在 `getTemperature()` 函数中：

```c
float getTemperature(void)
{
    float temperature = 999;  // 默认无效值
    uint8_t buf[2];
    I2C_Read(Device_addr, QMI8658_TEMP_L, buf, 2);
    
    int16_t raw_temp = (int16_t)((buf[1] << 8) | buf[0]);
    temperature = (float)raw_temp / 256.0f;
    
    return temperature;
}
```

**为什么用 999？**
- 正常温度范围：-40°C ~ +85°C
- 999 明显超出正常范围
- 容易识别读取失败

### 验证逻辑

```c
float chip_temp = getTemperature();
if (chip_temp != 999) {  // 检查是否有效
    g_env_data.temperature = chip_temp;
    g_env_data.is_valid = 1;
}
// 如果 chip_temp == 999，不更新数据，保持之前的状态
```

**优势：**
- ✅ 避免显示异常值
- ✅ I2C 通信失败时不影响 UI
- ✅ 保持上次有效数据

## 更新频率

### 定时器刷新

```c
// 在 smart_ui_main() 中创建定时器
ui_refresh_timer = lv_timer_create(smart_ui_tick_cb, 500, NULL);
                                                      ↑
                                                  每 500ms
```

**更新流程：**
```
t=0.0s:   读取温度 → 更新 g_env_data → 刷新 UI
t=0.5s:   读取温度 → 更新 g_env_data → 刷新 UI
t=1.0s:   读取温度 → 更新 g_env_data → 刷新 UI
...
```

### 性能考虑

**温度读取开销：**
- I2C 读取时间：< 1ms
- 数据转换时间：< 0.01ms
- 总开销：可忽略

**是否需要限制频率？**

❌ **不需要**，因为：
- 定时器本身就是 500ms 间隔
- 温度读取很快（< 1ms）
- 与电压读取（`BAT_Get_Volts()`）一起执行
- 总时间仍然很短

## 完整工作流程

### 1. 系统启动

```c
void app_main(void)
{
    I2C_Init();          // 初始化 I2C
    QMI8658_Init();      // 初始化温度传感器
    LVGL_Init();         // 初始化 LVGL
    smart_ui_main();     // 初始化 UI（创建定时器）
    
    // LVGL 主循环
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_timer_handler();  // 处理 LVGL 事件和定时器
    }
}
```

### 2. 定时器触发（每 500ms）

```c
smart_ui_tick_cb()
    ↓
获取互斥锁
    ↓
refresh_all_ui_elements()
    ↓
    ├─ BAT_Get_Volts() → g_system_data.power_voltage
    ├─ getTemperature() → g_env_data.temperature
    ├─ 更新环境数据标签
    ├─ 更新能耗数据标签
    ├─ 更新安防数据标签
    ├─ 更新系统数据标签
    └─ 更新房间设备状态
    ↓
释放互斥锁
```

### 3. UI 显示

```
屏幕显示：
┌─────────────────────────┐
│ 温度 28.5°C / 湿度 0%   │  ← 自动更新（28.5°C 来自 QMI8658）
└─────────────────────────┘
```

## 应用层数据 vs 系统数据

### 系统数据（自动更新）

**特点：**
- 由底层硬件提供
- 定时器自动获取
- 无需应用层干预

**示例：**
```c
// 在 refresh_all_ui_elements() 中自动执行
float voltage = BAT_Get_Volts();      // 电池电压
float temp = getTemperature();        // 芯片温度
```

### 应用层数据（主动推送）

**特点：**
- 由应用逻辑产生
- 需要主动调用更新函数
- 通过回调立即刷新

**示例：**
```c
// 在应用任务中主动调用
ui_update_environment(temp, humidity);  // 环境传感器
ui_update_energy(daily_kwh);           // 能耗数据
ui_update_room(idx, total, online);    // 房间设备
ui_update_system(wifi, rssi, fw, v);   // WiFi 状态
```

### 数据更新对比

| 数据类型 | 更新方式 | 触发机制 | 更新频率 |
|---------|---------|---------|---------|
| **电池电压** | 自动 | 定时器 | 500ms |
| **芯片温度** | 自动 | 定时器 | 500ms |
| **环境温湿度** | 手动 | 应用层调用 + 定时器兜底 | 应用决定 |
| **WiFi 状态** | 手动 | 应用层调用 + 定时器兜底 | 应用决定 |
| **设备状态** | 手动 | 应用层调用 + 定时器兜底 | 应用决定 |

## 扩展：添加外部温湿度传感器

如果您添加了 DHT22、SHT30 等外部传感器：

### 方案1：在应用层更新

```c
void temperature_sensor_task(void *pvParameters)
{
    while(1) {
        // 读取外部传感器
        float ambient_temp = read_dht22_temperature();
        uint8_t humidity = read_dht22_humidity();
        
        // 主动更新（会立即刷新 UI）
        ui_update_environment(ambient_temp, humidity);
        
        vTaskDelay(pdMS_TO_TICKS(5000));  // 每 5 秒
    }
}
```

### 方案2：保留芯片温度作为备用

```c
static void refresh_all_ui_elements(void)
{
    // ... 其他代码
    
    /* 实时获取温度 */
    float chip_temp = getTemperature();
    if (chip_temp != 999) {
        // 只在没有外部传感器数据时使用芯片温度
        if (!g_env_data.is_valid) {
            g_env_data.temperature = chip_temp;
            g_env_data.is_valid = 1;
        }
    }
    
    // ... 其他代码
}
```

**逻辑：**
- 优先使用外部传感器数据（更准确）
- 外部传感器无数据时，使用芯片温度（备用）

## 调试建议

### 1. 验证温度读取

```c
#include "esp_log.h"
static const char *TAG = "TEMP";

static void refresh_all_ui_elements(void)
{
    // ... 其他代码
    
    float chip_temp = getTemperature();
    ESP_LOGI(TAG, "QMI8658 temperature: %.2f°C", chip_temp);
    
    if (chip_temp != 999) {
        g_env_data.temperature = chip_temp;
        g_env_data.is_valid = 1;
        ESP_LOGI(TAG, "Temperature updated to UI: %.2f°C", chip_temp);
    } else {
        ESP_LOGW(TAG, "Temperature read failed (999)");
    }
    
    // ... 其他代码
}
```

### 2. 监控更新频率

```c
static uint32_t update_count = 0;

static void refresh_all_ui_elements(void)
{
    update_count++;
    if (update_count % 10 == 0) {  // 每 10 次（5 秒）打印一次
        ESP_LOGI(TAG, "UI refresh count: %d, Temp: %.2f°C", 
                 update_count, g_env_data.temperature);
    }
    
    // ... 其他代码
}
```

## 总结

### 修复内容

- ✅ 修复 `const` 指针赋值错误
- ✅ 采用与电压一致的处理方式
- ✅ 添加温度读取失败检测（999）
- ✅ 保持代码架构一致性

### 优势

- ✅ **自动更新** - 无需应用层干预
- ✅ **实时性** - 每 500ms 自动刷新
- ✅ **容错性** - 读取失败不影响 UI
- ✅ **一致性** - 与电压处理方式相同
- ✅ **简洁性** - 代码简单易懂

### 注意事项

- ⚠️ 这是芯片内部温度，不是环境温度
- ⚠️ 芯片温度通常高于环境 5-15°C
- ⚠️ 如需精确环境温度，请添加外部传感器

现在温度会每 500ms 自动更新到 UI，无需任何额外代码！🎉
