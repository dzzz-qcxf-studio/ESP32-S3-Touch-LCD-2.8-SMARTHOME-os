# 数据初始化示例 / Data Initialization Example

## 概述 / Overview

本文档说明如何初始化 UI 数据并填写各种信息，特别是版本号。

---

## 数据初始化流程 / Data Initialization Flow

### 1. 自动初始化 / Automatic Initialization

在 `smart_ui_main()` 中已经自动调用：

```c
void smart_ui_main(void)
{
    // ...
    /* 初始化数据模块 */
    smart_ui_data_init();  // 这里自动清空所有数据
    smart_ui_register_update_callback(smart_ui_tick_cb);
    // ...
}
```

### 2. 手动初始化数据 / Manual Data Initialization

在应用启动时初始化默认数据：

```c
#include "LVGL_Example.h"

void app_main(void)
{
    // ... 其他初始化 ...
    
    // 启动 UI
    smart_ui_main();
    
    // 初始化默认数据
    initialize_default_data();
    
    // ... 启动任务 ...
}

void initialize_default_data(void)
{
    /* 初始化系统数据 - 包括版本号 */
    smart_ui_system_data_t sys_data = {
        .wifi_status = "未连接",
        .wifi_rssi = 0,
        .firmware_version = "v1.0.0",  // 设置版本号
        .power_voltage = 4.8f,
        .wifi_valid = 1,
        .fw_valid = 1,
        .power_valid = 1
    };
    smart_ui_update_system_data(&sys_data);
    
    /* 初始化环境数据 */
    smart_ui_env_data_t env_data = {
        .temperature = 0.0f,
        .humidity = 0,
        .is_valid = 0  // 暂无数据
    };
    smart_ui_update_env_data(&env_data);
    
    /* 初始化能耗数据 */
    smart_ui_energy_data_t energy_data = {
        .daily_energy = 0.0f,
        .is_valid = 0  // 暂无数据
    };
    smart_ui_update_energy_data(&energy_data);
    
    /* 初始化安防数据 */
    smart_ui_security_data_t security_data = {
        .status = "未配置",
        .is_valid = 0  // 暂无数据
    };
    smart_ui_update_security_data(&security_data);
    
    /* 初始化房间数据 */
    for (int i = 0; i < 6; i++) {
        smart_ui_room_status_t room_data = {
            .total_devices = 0,
            .online_devices = 0,
            .is_valid = 0  // 暂无数据
        };
        smart_ui_update_room_status(i, &room_data);
    }
}
```

---

## 版本号管理 / Version Number Management

### 方案 1: 硬编码版本号

```c
#define FIRMWARE_VERSION "v1.0.0"

void update_system_info(void)
{
    smart_ui_system_data_t data = {
        .firmware_version = FIRMWARE_VERSION,
        .fw_valid = 1
    };
    smart_ui_update_system_data(&data);
}
```

### 方案 2: 从配置读取版本号

```c
#include "esp_app_desc.h"

void update_system_info(void)
{
    const esp_app_desc_t *app_desc = esp_app_get_description();
    
    smart_ui_system_data_t data = {0};
    strncpy(data.firmware_version, app_desc->version, 
            sizeof(data.firmware_version) - 1);
    data.fw_valid = 1;
    
    smart_ui_update_system_data(&data);
}
```

### 方案 3: 从 NVS 读取版本号

```c
#include "nvs_flash.h"
#include "nvs.h"

void update_system_info(void)
{
    nvs_handle_t handle;
    char version[16] = {0};
    
    esp_err_t err = nvs_open("system", NVS_READONLY, &handle);
    if (err == ESP_OK) {
        size_t len = sizeof(version);
        nvs_get_str(handle, "fw_version", version, &len);
        nvs_close(handle);
    }
    
    if (version[0] == '\0') {
        strcpy(version, "v1.0.0");  // 默认版本
    }
    
    smart_ui_system_data_t data = {0};
    strncpy(data.firmware_version, version, sizeof(data.firmware_version) - 1);
    data.fw_valid = 1;
    
    smart_ui_update_system_data(&data);
}
```

---

## 完整示例 / Complete Example

### 在 main.c 中使用

```c
#include "LVGL_Example.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 定义版本号 */
#define FIRMWARE_VERSION "v1.0.0"
#define DEVICE_NAME "Smart Home Hub"

void initialize_ui_data(void)
{
    /* 初始化系统数据 */
    smart_ui_system_data_t sys_data = {
        .wifi_status = "未连接",
        .wifi_rssi = 0,
        .firmware_version = FIRMWARE_VERSION,
        .power_voltage = 4.8f,
        .wifi_valid = 1,
        .fw_valid = 1,
        .power_valid = 1
    };
    smart_ui_update_system_data(&sys_data);
    
    printf("UI initialized with firmware version: %s\n", FIRMWARE_VERSION);
}

void sensor_task(void *arg)
{
    while (1) {
        /* 读取传感器 */
        float temp = 25.5f;
        uint8_t humidity = 60;
        
        /* 更新 UI */
        ui_update_environment(temp, humidity);
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void)
{
    /* 初始化 LVGL */
    smart_ui_main();
    
    /* 初始化 UI 数据 */
    initialize_ui_data();
    
    /* 创建传感器任务 */
    xTaskCreate(sensor_task, "sensor", 2048, NULL, 5, NULL);
}
```

---

## 数据结构详解 / Data Structure Details

### 系统数据结构 / System Data Structure

```c
typedef struct {
    char wifi_status[48];       /* Wi-Fi 状态文本，例如："已连接", "未连接", "连接中" */
    int8_t wifi_rssi;           /* Wi-Fi 信号强度 (dBm)，范围 -100 ~ 0 */
    char firmware_version[16];  /* 固件版本，例如："v1.0.0", "v2.1.3" */
    float power_voltage;        /* 电源电压 (V)，例如：4.8, 5.0 */
    bool wifi_valid;            /* Wi-Fi 数据是否有效 */
    bool fw_valid;              /* 固件数据是否有效 */
    bool power_valid;           /* 电源数据是否有效 */
} smart_ui_system_data_t;
```

### 环境数据结构 / Environment Data Structure

```c
typedef struct {
    float temperature;  /* 温度 (°C)，范围 -50 ~ 100 */
    uint8_t humidity;   /* 湿度 (%)，范围 0 ~ 100 */
    bool is_valid;      /* 数据是否有效 */
} smart_ui_env_data_t;
```

### 能耗数据结构 / Energy Data Structure

```c
typedef struct {
    float daily_energy;  /* 今日能耗 (kWh) */
    bool is_valid;       /* 数据是否有效 */
} smart_ui_energy_data_t;
```

### 安防数据结构 / Security Data Structure

```c
typedef struct {
    char status[32];  /* 安防状态文本，例如："全部布防", "客厅有人" */
    bool is_valid;    /* 数据是否有效 */
} smart_ui_security_data_t;
```

### 房间状态结构 / Room Status Structure

```c
typedef struct {
    uint8_t total_devices;   /* 房间内总设备数 */
    uint8_t online_devices;  /* 房间内在线设备数 */
    bool is_valid;           /* 数据是否有效 */
} smart_ui_room_status_t;
```

---

## 常见初始化场景 / Common Initialization Scenarios

### 场景 1: 启动时显示默认信息

```c
void app_main(void)
{
    smart_ui_main();
    
    /* 显示启动信息 */
    smart_ui_system_data_t data = {
        .firmware_version = "v1.0.0",
        .fw_valid = 1
    };
    smart_ui_update_system_data(&data);
}
```

### 场景 2: Wi-Fi 连接后更新信息

```c
void wifi_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_STA_CONNECTED) {
        smart_ui_system_data_t data = {
            .wifi_status = "已连接",
            .wifi_rssi = -50,
            .wifi_valid = 1
        };
        smart_ui_update_system_data(&data);
    }
}
```

### 场景 3: 定期更新所有信息

```c
void system_info_task(void *arg)
{
    while (1) {
        smart_ui_system_data_t data = {
            .wifi_status = get_wifi_status(),
            .wifi_rssi = get_wifi_rssi(),
            .firmware_version = FIRMWARE_VERSION,
            .power_voltage = get_battery_voltage(),
            .wifi_valid = 1,
            .fw_valid = 1,
            .power_valid = 1
        };
        smart_ui_update_system_data(&data);
        
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
```

---

## 数据有效性标志 / Data Validity Flags

### 何时设置为 1 (有效) / When to Set to 1 (Valid)

- 数据已成功获取
- 数据在合理范围内
- 数据来源可靠

### 何时设置为 0 (无效) / When to Set to 0 (Invalid)

- 数据获取失败
- 传感器离线
- 网络连接断开
- 数据超出合理范围

### 示例 / Example

```c
/* 有效数据 */
smart_ui_env_data_t valid_data = {
    .temperature = 25.5f,
    .humidity = 60,
    .is_valid = 1  /* 设置为有效 */
};

/* 无效数据 */
smart_ui_env_data_t invalid_data = {
    .temperature = 0.0f,
    .humidity = 0,
    .is_valid = 0  /* 设置为无效 - UI 会显示"暂无数据" */
};
```

---

## 最佳实践 / Best Practices

1. **在启动时初始化** / Initialize at Startup
   - 在 `app_main()` 中调用初始化函数
   - 设置默认值和版本号

2. **定期更新** / Update Regularly
   - 使用 FreeRTOS 任务定期更新数据
   - 避免在 UI 线程中进行阻塞操作

3. **检查有效性** / Check Validity
   - 始终设置 `is_valid` 标志
   - 只在数据有效时才更新

4. **错误处理** / Error Handling
   - 如果数据获取失败，设置 `is_valid = 0`
   - UI 会自动显示"暂无数据"

5. **版本号管理** / Version Management
   - 使用宏定义版本号
   - 或从 `esp_app_desc_t` 读取
   - 或从 NVS 存储读取

---

## 调试技巧 / Debugging Tips

### 检查数据是否更新

```c
const smart_ui_system_data_t *data = smart_ui_get_system_data();
printf("FW Version: %s, Valid: %d\n", data->firmware_version, data->fw_valid);
```

### 检查回调是否被调用

```c
void debug_callback(void)
{
    printf("UI update callback triggered!\n");
}

// 在初始化时注册
smart_ui_register_update_callback(debug_callback);
```

---

## 相关文件 / Related Files

- `smart_ui_data.h` - 数据结构定义
- `smart_ui_data.c` - 数据管理实现
- `LVGL_Example.h` - UI 便利函数
- `QUICK_REFERENCE.md` - 快速参考
