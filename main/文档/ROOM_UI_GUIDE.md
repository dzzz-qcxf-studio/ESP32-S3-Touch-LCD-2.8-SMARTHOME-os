# 房间界面模块使用指南

## 概述

房间界面模块（`room_ui`）提供了一个模块化的房间详细界面管理系统。用户可以从房间列表点击房间按钮，跳转到该房间的详细界面。

## 文件结构

```
LVGL_UI/
├── room_ui.h          # 房间界面模块头文件
├── room_ui.c          # 房间界面模块实现
└── LVGL_Example.c     # 主UI文件（集成房间界面）
```

## 核心功能

### 1. 房间索引定义

```c
typedef enum {
    ROOM_LIVING_ROOM = 0,    /* 客厅 */
    ROOM_MASTER_BEDROOM = 1, /* 主卧 */
    ROOM_SECOND_BEDROOM = 2, /* 次卧 */
    ROOM_KITCHEN = 3,        /* 厨房 */
    ROOM_STUDY = 4,          /* 书房 */
    ROOM_GARAGE = 5,         /* 车库 */
    ROOM_COUNT = 6
} room_index_t;
```

### 2. 主要API

#### 创建房间界面
```c
lv_obj_t *room_ui_create(lv_obj_t *parent, uint8_t room_idx);
```
- **参数**：
  - `parent`: 父容器（通常是标签页）
  - `room_idx`: 房间索引（0-5）
- **返回值**：房间界面对象指针
- **功能**：创建并显示房间详细界面

#### 销毁房间界面
```c
void room_ui_destroy(lv_obj_t *room_screen);
```
- **参数**：`room_screen` - 房间界面对象指针
- **功能**：删除房间界面

#### 获取房间名称
```c
const char *room_ui_get_name(uint8_t room_idx);
```
- **参数**：`room_idx` - 房间索引
- **返回值**：房间名称字符串
- **功能**：获取房间的中文名称

## 工作流程

### 1. 房间列表界面
- 显示6个房间按钮
- 每个按钮显示房间名称和设备状态
- 按钮可点击

### 2. 按钮点击事件
```c
static void room_btn_event(lv_event_t * e)
{
    /* 获取房间索引 */
    uint8_t room_idx = (uint8_t)(uintptr_t)lv_obj_get_user_data(btn);
    
    /* 创建房间详细界面 */
    room_ui_create(parent, room_idx);
}
```

### 3. 房间详细界面
- **头部**：返回按钮 + 房间名称
- **内容**：占位符（待开发）
- **返回**：点击返回按钮删除房间界面

## 界面布局

```
┌─────────────────────────────┐
│ ◀ 客厅                      │  <- 头部（返回按钮 + 房间名称）
├─────────────────────────────┤
│                             │
│                             │
│      房间详情               │  <- 内容区域（占位符）
│      （待开发）             │
│                             │
│                             │
└─────────────────────────────┘
```

## 样式定义

房间界面使用三个主要样式：

### 1. `style_room_screen` - 屏幕背景
- 背景色：#F8F9FB（浅灰蓝）
- 文本色：#1F1F1F（深灰）
- 字体：中文字体

### 2. `style_room_header` - 头部容器
- 背景色：#E8ECFF（浅蓝）
- 圆角：10px
- 内边距：12px
- 文本色：#223267（深蓝）

### 3. `style_room_content` - 内容容器
- 背景色：#FFFFFF（白色）
- 圆角：12px
- 内边距：14px
- 阴影：6px

## 扩展指南

### 添加房间功能

在 `room_ui.c` 的 `room_ui_create_content` 函数中添加实际内容：

```c
static void room_ui_create_content(lv_obj_t *parent, uint8_t room_idx)
{
    lv_obj_t *content = lv_obj_create(parent);
    lv_obj_add_style(content, &style_room_content, 0);
    lv_obj_set_width(content, LV_PCT(100));
    lv_obj_set_height(LV_PCT(100));
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    
    /* 示例：添加设备列表 */
    lv_obj_t *device_list = lv_obj_create(content);
    // ... 添加设备项
    
    /* 示例：添加控制按钮 */
    lv_obj_t *control_btn = lv_btn_create(content);
    // ... 添加控制逻辑
}
```

### 添加新房间

1. 在 `room_index_t` 枚举中添加新房间
2. 在 `room_names` 数组中添加房间名称
3. 更新 `ROOM_COUNT` 常数

```c
typedef enum {
    // ... 现有房间 ...
    ROOM_BALCONY = 6,        /* 阳台 */
    ROOM_COUNT = 7           /* 更新计数 */
} room_index_t;

static const char *room_names[] = {
    // ... 现有房间 ...
    "阳台"                    /* 添加新房间名称 */
};
```

## 数据流

```
用户点击房间按钮
    ↓
room_btn_event() 事件处理
    ↓
获取房间索引
    ↓
room_ui_create() 创建界面
    ↓
显示房间详细界面
    ↓
用户点击返回按钮
    ↓
room_back_btn_event() 处理
    ↓
lv_obj_del() 删除界面
    ↓
返回房间列表
```

## 集成示例

在 `LVGL_Example.c` 中的集成：

```c
/* 1. 包含头文件 */
#include "room_ui.h"

/* 2. 在房间按钮创建时添加事件 */
lv_obj_set_user_data(btn, (void *)(uintptr_t)i);
lv_obj_add_event_cb(btn, room_btn_event, LV_EVENT_CLICKED, parent);

/* 3. 实现事件处理函数 */
static void room_btn_event(lv_event_t * e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *parent = (lv_obj_t *)lv_event_get_user_data(e);
    uint8_t room_idx = (uint8_t)(uintptr_t)lv_obj_get_user_data(btn);
    
    if (parent) {
        room_ui_create(parent, room_idx);
    }
}
```

## 编译配置

在 `CMakeLists.txt` 中添加源文件：

```cmake
SRCS 
    "./LVGL_UI/room_ui.c"
    # ... 其他文件 ...
```

## 常见问题

### Q: 如何更新房间数据？
A: 使用 `smart_ui_data` 模块的 `smart_ui_update_room_status()` 函数更新房间设备状态。房间列表会自动刷新。

### Q: 如何在房间界面中显示实时数据？
A: 在 `room_ui_create_content()` 中创建数据标签，然后在主UI的 `smart_ui_tick_cb()` 中更新这些标签。

### Q: 如何添加房间控制功能？
A: 在 `room_ui_create_content()` 中添加按钮或滑块，并为其添加事件回调。

## 总结

房间界面模块提供了：
- ✅ 模块化的房间界面管理
- ✅ 清晰的导航流程（列表 → 详情 → 返回）
- ✅ 易于扩展的架构
- ✅ 一致的UI样式
- ✅ 完整的事件处理
