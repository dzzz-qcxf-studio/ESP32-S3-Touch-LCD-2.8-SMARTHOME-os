# QMI8658 温度传感器使用指南

## 功能说明

QMI8658 芯片内置温度传感器，可以读取芯片内部温度。

**注意：** 这是芯片内部温度，而非环境温度。由于芯片工作时会发热，内部温度通常会高于环境温度。

## API 接口

### 函数原型

```c
float getTemperature(void);
```

**功能：** 读取 QMI8658 芯片内部温度

**参数：** 无

**返回值：** `float` - 温度值（单位：摄氏度 °C）

**头文件：** `QMI8658/QMI8658.h`

## 使用示例

### 示例1：读取并打印温度

```c
#include "QMI8658.h"
#include "esp_log.h"

static const char *TAG = "TEMP_SENSOR";

void temperature_test(void)
{
    // 读取温度
    float temperature = getTemperature();
    
    // 打印温度
    ESP_LOGI(TAG, "芯片温度: %.2f°C", temperature);
}
```

### 示例2：定期读取温度任务

```c
#include "QMI8658.h"
#include "LVGL_Example.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "TEMP_TASK";

void temperature_monitor_task(void *pvParameters)
{
    while(1) {
        // 读取温度
        float temp = getTemperature();
        
        ESP_LOGI(TAG, "当前温度: %.2f°C", temp);
        
        // 更新到 UI（假设湿度为 0，因为 QMI8658 没有湿度传感器）
        ui_update_environment(temp, 0);
        
        // 每 5 秒读取一次
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// 在 main.c 中创建任务
void app_main(void)
{
    // ... 其他初始化代码
    
    QMI8658_Init();  // 初始化 QMI8658
    
    // 创建温度监控任务
    xTaskCreate(temperature_monitor_task, "temp_monitor", 2048, NULL, 5, NULL);
    
    // ... 其他代码
}
```

### 示例3：结合加速度计和温度读取

```c
#include "QMI8658.h"
#include "esp_log.h"

static const char *TAG = "SENSOR";

void read_all_sensors(void)
{
    // 读取加速度
    getAccelerometer();
    ESP_LOGI(TAG, "加速度: X=%.2f Y=%.2f Z=%.2f", 
             Accel.x, Accel.y, Accel.z);
    
    // 读取陀螺仪
    getGyroscope();
    ESP_LOGI(TAG, "陀螺仪: X=%.2f Y=%.2f Z=%.2f", 
             Gyro.x, Gyro.y, Gyro.z);
    
    // 读取温度
    float temp = getTemperature();
    ESP_LOGI(TAG, "温度: %.2f°C", temp);
}
```

## 完整集成示例

在 `main.c` 中添加温度监控：

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "QMI8658.h"
#include "LVGL_Example.h"

static const char *TAG = "MAIN";

/**
 * 温度监控任务
 * 定期读取 QMI8658 温度并更新到 UI
 */
void temperature_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Temperature monitoring task started");
    
    while(1) {
        // 读取芯片温度
        float chip_temp = getTemperature();
        
        // 打印调试信息
        ESP_LOGI(TAG, "Chip temperature: %.2f°C", chip_temp);
        
        // 更新到 UI
        // 注意：这是芯片温度，不是环境温度
        // 如果没有湿度传感器，可以传 0 或不显示
        ui_update_environment(chip_temp, 0);
        
        // 每 10 秒更新一次（避免过于频繁）
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void app_main(void)
{
    // 初始化 I2C
    I2C_Init();
    
    // 初始化 QMI8658
    QMI8658_Init();
    
    // 初始化 LVGL
    LVGL_Init();
    
    // 初始化 UI
    smart_ui_main();
    
    // 创建温度监控任务
    xTaskCreate(temperature_task, "temp_task", 2048, NULL, 5, NULL);
    
    // LVGL 主循环
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_timer_handler();
    }
}
```

## 技术细节

### 数据格式

```c
// 温度数据存储在两个寄存器中
QMI8658_TEMP_L  0x33  // 低字节
QMI8658_TEMP_H  0x34  // 高字节

// 16位有符号整数
int16_t raw_temp = (buf[1] << 8) | buf[0];

// 转换为摄氏度
float temp = raw_temp / 256.0f;
```

### 温度范围

- **测量范围：** 通常为 -40°C ~ +85°C
- **精度：** ±3°C（典型值）
- **分辨率：** 1/256 °C ≈ 0.004°C

### 读取时间

- 单次读取时间：< 1ms
- 建议读取间隔：≥ 1秒

## 注意事项

### ⚠️ 重要提示

1. **这是芯片内部温度，不是环境温度**
   ```
   芯片温度 ≠ 环境温度
   
   芯片工作时会发热，内部温度会高于环境温度
   温差通常为 5-15°C
   ```

2. **如果需要准确的环境温度**
   - 需要添加专用温度传感器（如 DHT22、SHT30、BME280）
   - 或根据经验校准偏移值

3. **校准示例**
   ```c
   float getAmbientTemperature(void)
   {
       float chip_temp = getTemperature();
       
       // 经验偏移值（需要根据实际情况调整）
       float offset = -10.0f;  // 芯片比环境温度高约 10°C
       
       return chip_temp + offset;
   }
   ```

### 适用场景

✅ **适合：**
- 监控芯片工作状态
- 检测过热保护
- 系统温度管理
- 温度补偿（用于补偿加速度计/陀螺仪）

❌ **不适合：**
- 精确的环境温度测量
- 室温监控
- 天气监测

## 性能考虑

### 读取频率建议

```c
// ✅ 推荐：每 5-10 秒读取一次
vTaskDelay(pdMS_TO_TICKS(5000));  // 5秒

// ⚠️ 可接受：每 1 秒读取一次
vTaskDelay(pdMS_TO_TICKS(1000));  // 1秒

// ❌ 不推荐：过于频繁
vTaskDelay(pdMS_TO_TICKS(100));   // 0.1秒 - 太快！
```

**原因：**
- 温度变化缓慢，无需频繁读取
- 减少 I2C 总线占用
- 降低功耗

### 与其他传感器的关系

```c
// QMI8658 同时提供多种数据
void read_all_qmi8658_data(void)
{
    getAccelerometer();   // 加速度计
    getGyroscope();       // 陀螺仪
    float temp = getTemperature();  // 温度
    
    // 可以一次性读取，无需多次 I2C 通信
}
```

## 调试技巧

### 1. 验证温度读取

```c
void test_temperature_sensor(void)
{
    ESP_LOGI(TAG, "Testing temperature sensor...");
    
    for (int i = 0; i < 5; i++) {
        float temp = getTemperature();
        ESP_LOGI(TAG, "Reading %d: %.2f°C", i + 1, temp);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

**预期结果：**
```
I (1234) TAG: Testing temperature sensor...
I (1234) TAG: Reading 1: 28.35°C
I (2234) TAG: Reading 2: 28.37°C
I (3234) TAG: Reading 3: 28.39°C
I (4234) TAG: Reading 4: 28.41°C
I (5234) TAG: Reading 5: 28.43°C
```

### 2. 检查温度范围

```c
void check_temperature_range(void)
{
    float temp = getTemperature();
    
    if (temp < -40.0f || temp > 85.0f) {
        ESP_LOGW(TAG, "Temperature out of normal range: %.2f°C", temp);
        // 可能是读取错误或传感器故障
    } else if (temp > 70.0f) {
        ESP_LOGW(TAG, "Chip temperature high: %.2f°C", temp);
        // 芯片过热，可能需要散热
    } else {
        ESP_LOGI(TAG, "Temperature normal: %.2f°C", temp);
    }
}
```

### 3. 温度变化监控

```c
static float last_temp = 0.0f;

void monitor_temperature_change(void)
{
    float current_temp = getTemperature();
    float delta = current_temp - last_temp;
    
    ESP_LOGI(TAG, "Current: %.2f°C, Last: %.2f°C, Delta: %.2f°C", 
             current_temp, last_temp, delta);
    
    if (delta > 5.0f) {
        ESP_LOGW(TAG, "Temperature increased rapidly!");
    }
    
    last_temp = current_temp;
}
```

## 常见问题

### Q1：为什么温度读数比室温高？

**A：** 这是正常现象。QMI8658 测量的是芯片内部温度，芯片工作时会发热。典型偏差为 5-15°C。

### Q2：如何获得准确的环境温度？

**A：** 有两种方案：
1. **添加专用温度传感器**（推荐）
   - DHT22：温湿度传感器
   - SHT30：高精度温湿度传感器
   - BME280：温湿度气压传感器

2. **校准偏移值**
   ```c
   float ambient_temp = getTemperature() - 10.0f;  // 经验值
   ```

### Q3：温度读数总是 0 或异常值？

**A：** 检查以下几点：
1. 确认 QMI8658 已正确初始化：`QMI8658_Init()`
2. 检查 I2C 通信是否正常
3. 确认芯片地址正确（0x6A 或 0x6B）
4. 查看错误日志

### Q4：可以用温度补偿加速度计/陀螺仪吗？

**A：** 可以，但 QMI8658 通常有内部温度补偿。如果需要额外补偿：

```c
void compensated_reading(void)
{
    float temp = getTemperature();
    getAccelerometer();
    
    // 温度补偿系数（需要校准）
    float temp_coefficient = (temp - 25.0f) * 0.001f;
    
    Accel.x = Accel.x * (1.0f + temp_coefficient);
    Accel.y = Accel.y * (1.0f + temp_coefficient);
    Accel.z = Accel.z * (1.0f + temp_coefficient);
}
```

## 总结

### 优势
- ✅ 简单易用，单函数调用
- ✅ 无需额外硬件
- ✅ 可用于芯片温度监控
- ✅ 可用于温度补偿

### 局限性
- ❌ 不是环境温度
- ❌ 精度有限（±3°C）
- ❌ 受芯片发热影响

### 推荐用途
- 监控系统温度状态
- 过热保护
- 温度补偿算法
- 调试和诊断

如果需要精确的环境温度测量，建议添加专用温度传感器！
