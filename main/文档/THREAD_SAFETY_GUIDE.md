# LVGL 多任务并发安全指南

## ⚠️ 重要：会有冲突！

### 当前架构的并发问题

我们有两种UI刷新机制同时工作：

```
┌─────────────────────────────────────────┐
│  LVGL 任务 (app_main)                   │
│  ├─ lv_timer_handler()                 │
│  └─ smart_ui_tick_cb() ────────┐       │
└────────────────────────────────┼───────┘
                                 │
                         同时操作 LVGL 对象
                                 │
┌────────────────────────────────┼───────┐
│  其他任务 (传感器、WiFi等)      │       │
│  ├─ ui_update_environment()    │       │
│  └─ smart_ui_refresh_callback()┘       │
└─────────────────────────────────────────┘
```

**问题：LVGL 不是线程安全的！**

## 冲突场景示例

### 场景1：显示错乱

```c
时刻 t=0.000s: LVGL 任务开始更新温度标签
               lv_label_set_text(label, "温度 25.5°C")
                 ↓ (内部修改标签的文本缓冲区)
               
时刻 t=0.001s: 传感器任务调用 ui_update_environment()
               触发回调，也要更新温度标签
               lv_label_set_text(label, "温度 26.0°C")
                 ↓ (同时修改标签的文本缓冲区)
                 
结果: ❌ 数据竞争，可能显示 "温度 25.6°C" (混乱的值)
```

### 场景2：系统崩溃

```c
LVGL 任务: 正在重新布局UI
           ├─ 修改对象的坐标
           ├─ 重新计算尺寸
           └─ 更新内部缓冲区
           
传感器任务: 同时更新标签文本
           └─ 触发重新布局
           
结果: ❌ 内部状态不一致，可能触发断言或硬件异常
```

## 解决方案

### 方案1：使用 LVGL 内置锁（已实现）✅

```c
static void smart_ui_refresh_callback(void)
{
    lv_lock();    // ← 获取 LVGL 锁
    refresh_all_ui_elements();  // 安全地操作 LVGL 对象
    lv_unlock();  // ← 释放锁
}
```

**工作原理：**
```
LVGL 任务正在运行 lv_timer_handler()
    ↓ (持有锁)
传感器任务调用 lv_lock()
    ↓ (等待锁释放)
LVGL 任务完成，释放锁
    ↓
传感器任务获取锁
    ↓ (现在可以安全操作)
传感器任务释放锁
```

### 启用 LVGL 锁机制

#### 检查 LVGL 配置

1. **打开项目配置**
   ```bash
   idf.py menuconfig
   ```

2. **导航到 LVGL 配置**
   ```
   Component config → LVGL configuration → Operating System
   ```

3. **确认以下选项已启用**
   ```
   [*] Enable LVGL operating system support (LV_USE_OS)
       Operating system → FreeRTOS
   [*] Enable LVGL locking API
   ```

4. **保存并重新编译**
   ```bash
   idf.py build
   ```

#### 验证锁是否工作

添加调试日志：

```c
#include "esp_log.h"
static const char *TAG = "UI_LOCK";

static void smart_ui_refresh_callback(void)
{
    ESP_LOGI(TAG, "Acquiring LVGL lock...");
    lv_lock();
    ESP_LOGI(TAG, "Lock acquired, refreshing UI");
    refresh_all_ui_elements();
    lv_unlock();
    ESP_LOGI(TAG, "Lock released");
}
```

预期输出：
```
I (1234) UI_LOCK: Acquiring LVGL lock...
I (1234) UI_LOCK: Lock acquired, refreshing UI
I (1250) UI_LOCK: Lock released
```

### 方案2：如果没有锁机制的备选方案

如果您的 LVGL 版本不支持锁，或者锁未启用：

#### 选项A：使用消息队列（推荐）

```c
#include "freertos/queue.h"

// 定义UI更新消息
typedef enum {
    UI_UPDATE_ENV,
    UI_UPDATE_ENERGY,
    UI_UPDATE_ROOM,
    // ...
} ui_update_type_t;

typedef struct {
    ui_update_type_t type;
    union {
        struct { float temp; uint8_t humidity; } env;
        struct { float daily_kwh; } energy;
        struct { uint8_t room_idx; uint8_t total; uint8_t online; } room;
    } data;
} ui_update_msg_t;

static QueueHandle_t ui_update_queue = NULL;

// 初始化队列
void ui_queue_init(void) {
    ui_update_queue = xQueueCreate(10, sizeof(ui_update_msg_t));
}

// 应用层发送更新请求
void ui_update_environment(float temp, uint8_t humidity) {
    ui_update_msg_t msg = {
        .type = UI_UPDATE_ENV,
        .data.env = { .temp = temp, .humidity = humidity }
    };
    xQueueSend(ui_update_queue, &msg, 0);  // 不阻塞
}

// 在 LVGL 任务中处理队列
static void smart_ui_tick_cb(lv_timer_t * timer) {
    ui_update_msg_t msg;
    
    // 处理所有待处理的更新
    while (xQueueReceive(ui_update_queue, &msg, 0) == pdTRUE) {
        switch (msg.type) {
            case UI_UPDATE_ENV:
                // 更新数据层
                smart_ui_env_data_t env_data = {
                    .temperature = msg.data.env.temp,
                    .humidity = msg.data.env.humidity,
                    .is_valid = 1
                };
                smart_ui_update_env_data(&env_data);
                break;
            // ... 其他类型
        }
    }
    
    // 刷新UI（在LVGL任务中，安全）
    refresh_all_ui_elements();
}
```

**优势：**
- ✅ 完全线程安全
- ✅ 解耦应用层和UI层
- ✅ 可以缓冲多个更新

**劣势：**
- ❌ 更新不是立即的（最大延迟 = 定时器周期）
- ❌ 需要额外内存（队列）

#### 选项B：禁用立即回调，只用定时器

```c
void smart_ui_main(void) {
    smart_ui_data_init();
    
    // ❌ 不注册回调
    // smart_ui_register_update_callback(smart_ui_refresh_callback);
    
    // ✅ 只用定时器（在LVGL任务中运行，安全）
    ui_refresh_timer = lv_timer_create(smart_ui_tick_cb, 500, NULL);
}
```

**优势：**
- ✅ 简单，无需锁
- ✅ 完全线程安全

**劣势：**
- ❌ 失去了立即响应的优势
- ❌ 最大延迟 500ms

### 方案3：使用 LVGL 的异步调用（LVGL 9.0+）

如果使用较新的 LVGL 版本：

```c
static void async_refresh_callback(void *user_data) {
    refresh_all_ui_elements();
}

static void smart_ui_refresh_callback(void) {
    // 将刷新操作安排到 LVGL 任务中执行
    lv_async_call(async_refresh_callback, NULL);
}
```

## 当前实现状态

### ✅ 已添加保护

```c
static void smart_ui_refresh_callback(void)
{
    lv_lock();    // ← 已添加
    refresh_all_ui_elements();
    lv_unlock();  // ← 已添加
}
```

### ✅ 不冲突的部分

定时器回调不需要锁（已经在 LVGL 任务中运行）：

```c
static void smart_ui_tick_cb(lv_timer_t * timer)
{
    LV_UNUSED(timer);
    refresh_all_ui_elements();  // 不需要锁
}
```

## 性能影响

### 锁的开销

```c
lv_lock()    // 约 1-5 微秒 (FreeRTOS 互斥锁)
操作 LVGL     // 主要耗时
lv_unlock()  // 约 1-5 微秒
```

**总开销：可忽略不计**

### 锁竞争场景

```
LVGL 任务 (优先级 5): 每 10ms 运行一次
传感器任务 (优先级 5): 每 5秒 更新一次

锁竞争概率: 极低 (< 0.1%)
等待时间: 通常 < 20ms
```

## 调试并发问题

### 症状检查清单

❌ **如果出现以下问题，可能是并发冲突：**

1. UI 显示错乱或闪烁
2. 随机崩溃（尤其在调用 `ui_update_xxx()` 后）
3. 硬件异常（LoadProhibited、StoreProhibited）
4. 断言失败（`assert failed: ...`）

### 调试技巧

#### 1. 启用 LVGL 断言

在 `lv_conf.h` 或 menuconfig 中：
```c
#define LV_USE_ASSERT_OBJ 1
#define LV_USE_ASSERT_STYLE 1
```

#### 2. 添加任务标记

```c
static void smart_ui_refresh_callback(void)
{
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    ESP_LOGI(TAG, "Refresh called from task: %s", pcTaskGetName(current_task));
    
    lv_lock();
    refresh_all_ui_elements();
    lv_unlock();
}
```

#### 3. 使用 ESP-IDF 的任务监控

```c
#include "esp_task_wdt.h"

// 监控死锁
esp_task_wdt_add(NULL);
lv_lock();
// ... 操作
lv_unlock();
esp_task_wdt_reset();
```

## 最佳实践总结

### ✅ 推荐做法

1. **启用 LVGL 锁机制**
   - 在 menuconfig 中配置
   - 在回调中使用 `lv_lock()` / `lv_unlock()`

2. **限制更新频率**
   ```c
   // ✅ 不要每次数据变化都更新
   static uint32_t last_update = 0;
   uint32_t now = xTaskGetTickCount();
   if (now - last_update > pdMS_TO_TICKS(100)) {  // 最少间隔100ms
       ui_update_environment(...);
       last_update = now;
   }
   ```

3. **批量更新**
   ```c
   // ✅ 一次更新多个数据
   lv_lock();
   ui_update_environment(...);
   ui_update_energy(...);
   ui_update_room(...);
   lv_unlock();
   ```

### ❌ 避免的做法

1. **不要在中断中调用**
   ```c
   // ❌ 错误！不要在ISR中调用
   void IRAM_ATTR sensor_isr(void) {
       ui_update_environment(...);  // 危险！
   }
   ```

2. **不要高频更新**
   ```c
   // ❌ 太频繁！
   while(1) {
       ui_update_environment(...);
       vTaskDelay(pdMS_TO_TICKS(10));  // 每10ms更新
   }
   ```

3. **不要跨任务直接操作 LVGL 对象**
   ```c
   // ❌ 错误！
   extern lv_obj_t *my_label;
   lv_label_set_text(my_label, "...");  // 没有锁保护
   ```

## 总结

### 问题

- ❌ 两种刷新机制会冲突
- ❌ LVGL 不是线程安全的
- ❌ 可能导致崩溃或显示错乱

### 解决方案（已实现）

- ✅ 使用 `lv_lock()` / `lv_unlock()` 保护回调
- ✅ 定时器不需要锁（已在 LVGL 任务中）
- ✅ 性能影响可忽略

### 验证步骤

1. 检查 menuconfig 中 LVGL 锁已启用
2. 编译运行，观察是否有崩溃
3. 使用多个任务同时更新UI进行测试
4. 长时间运行测试稳定性

### 如果锁不可用

- 使用消息队列方案
- 或禁用立即回调，只用定时器
