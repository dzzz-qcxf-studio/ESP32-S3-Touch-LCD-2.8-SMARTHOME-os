# 房间界面模块 - 快速参考

## 房间索引

| 索引 | 房间名称 | 常数 |
|------|---------|------|
| 0 | 客厅 | `ROOM_LIVING_ROOM` |
| 1 | 主卧 | `ROOM_MASTER_BEDROOM` |
| 2 | 次卧 | `ROOM_SECOND_BEDROOM` |
| 3 | 厨房 | `ROOM_KITCHEN` |
| 4 | 书房 | `ROOM_STUDY` |
| 5 | 车库 | `ROOM_GARAGE` |

## API 速查表

### 创建房间界面
```c
lv_obj_t *room_screen = room_ui_create(parent, ROOM_LIVING_ROOM);
```

### 销毁房间界面
```c
room_ui_destroy(room_screen);
```

### 获取房间名称
```c
const char *name = room_ui_get_name(ROOM_LIVING_ROOM);  // 返回 "客厅"
```

## 工作流程

```
房间列表
  ↓
点击房间按钮 → room_btn_event()
  ↓
room_ui_create(parent, room_idx)
  ↓
显示房间详细界面
  ↓
点击返回按钮 → room_back_btn_event()
  ↓
lv_obj_del(room_screen)
  ↓
返回房间列表
```

## 事件处理

### 房间按钮点击
```c
static void room_btn_event(lv_event_t * e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *parent = (lv_obj_t *)lv_event_get_user_data(e);
    uint8_t room_idx = (uint8_t)(uintptr_t)lv_obj_get_user_data(btn);
    
    room_ui_create(parent, room_idx);
}
```

### 返回按钮点击
```c
static void room_back_btn_event(lv_event_t *e)
{
    lv_obj_t *room_screen = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_del(room_screen);
}
```

## 集成步骤

### 1. 包含头文件
```c
#include "room_ui.h"
```

### 2. 为房间按钮添加事件
```c
for (size_t i = 0; i < 6; i++) {
    lv_obj_t *btn = lv_btn_create(grid);
    
    /* 保存房间索引 */
    lv_obj_set_user_data(btn, (void *)(uintptr_t)i);
    
    /* 添加点击事件 */
    lv_obj_add_event_cb(btn, room_btn_event, LV_EVENT_CLICKED, parent);
    
    // ... 其他按钮配置 ...
}
```

### 3. 编译配置
在 `CMakeLists.txt` 中添加：
```cmake
"./LVGL_UI/room_ui.c"
```

## 样式自定义

### 修改头部样式
```c
lv_style_set_bg_color(&style_room_header, lv_color_hex(0xE8ECFF));
lv_style_set_radius(&style_room_header, 10);
lv_style_set_pad_all(&style_room_header, 12);
```

### 修改内容样式
```c
lv_style_set_bg_color(&style_room_content, lv_color_hex(0xFFFFFF));
lv_style_set_radius(&style_room_content, 12);
lv_style_set_pad_all(&style_room_content, 14);
```

## 扩展内容

在 `room_ui_create_content()` 中添加：

### 添加设备列表
```c
lv_obj_t *device_list = lv_obj_create(content);
lv_obj_set_flex_flow(device_list, LV_FLEX_FLOW_COLUMN);

for (int i = 0; i < device_count; i++) {
    lv_obj_t *device_item = lv_label_create(device_list);
    lv_label_set_text_fmt(device_item, "设备 %d: %s", i, device_names[i]);
}
```

### 添加控制按钮
```c
lv_obj_t *control_btn = lv_btn_create(content);
lv_obj_set_size(control_btn, 100, 40);
lv_obj_add_event_cb(control_btn, control_btn_event, LV_EVENT_CLICKED, NULL);

lv_obj_t *btn_label = lv_label_create(control_btn);
lv_label_set_text(btn_label, "控制");
lv_obj_center(btn_label);
```

### 添加温度/湿度显示
```c
lv_obj_t *temp_label = lv_label_create(content);
lv_label_set_text_fmt(temp_label, "温度: %.1f°C", 22.5);

lv_obj_t *humidity_label = lv_label_create(content);
lv_label_set_text_fmt(humidity_label, "湿度: %u%%", 65);
```

## 数据更新

### 更新房间设备状态
```c
/* 在应用层调用 */
ui_update_room(ROOM_LIVING_ROOM, 5, 3);  // 客厅：5个设备，3个在线
```

### 在房间界面中显示数据
```c
const smart_ui_room_status_t *room_data = smart_ui_get_room_status(room_idx);
if (room_data && room_data->is_valid) {
    lv_label_set_text_fmt(status_label, "设备%u · 在线%u",
                          room_data->total_devices,
                          room_data->online_devices);
}
```

## 文件清单

| 文件 | 说明 |
|------|------|
| `room_ui.h` | 头文件，包含API定义 |
| `room_ui.c` | 实现文件，包含界面创建逻辑 |
| `LVGL_Example.c` | 主UI文件，集成房间界面 |
| `CMakeLists.txt` | 编译配置，包含room_ui.c |

## 常用代码片段

### 创建并显示房间界面
```c
lv_obj_t *room_screen = room_ui_create(parent_tab, room_index);
```

### 获取房间名称用于显示
```c
const char *room_name = room_ui_get_name(room_index);
lv_label_set_text_fmt(label, "房间: %s", room_name);
```

### 在事件中获取房间索引
```c
uint8_t room_idx = (uint8_t)(uintptr_t)lv_obj_get_user_data(btn);
```

### 在事件中获取父容器
```c
lv_obj_t *parent = (lv_obj_t *)lv_event_get_user_data(e);
```

## 调试技巧

### 检查房间索引是否正确
```c
ESP_LOGI("ROOM_UI", "Room index: %d, Name: %s", room_idx, room_ui_get_name(room_idx));
```

### 验证事件是否触发
```c
static void room_btn_event(lv_event_t * e)
{
    ESP_LOGI("ROOM_UI", "Room button clicked!");
    // ... 其他代码 ...
}
```

### 检查界面是否创建成功
```c
lv_obj_t *room_screen = room_ui_create(parent, room_idx);
if (room_screen) {
    ESP_LOGI("ROOM_UI", "Room screen created successfully");
} else {
    ESP_LOGE("ROOM_UI", "Failed to create room screen");
}
```
