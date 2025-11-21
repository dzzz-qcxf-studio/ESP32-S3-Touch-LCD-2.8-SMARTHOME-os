# 中文文本参考 / Chinese Text Reference

## 📋 Home 页面 / Home Tab

### 导航栏 / Navigation Bar
```
标题: "智能家居控制"
Wi-Fi 芯片: "网络正常" (或其他 Wi-Fi 状态)
```

### 状态卡片 / Status Cards
```
卡片1标题: "环境"
卡片1初始值: "暂无数据"

卡片2标题: "能耗"
卡片2初始值: "暂无数据"

卡片3标题: "安防"
卡片3初始值: "暂无数据"
```

### 卡片更新内容 / Card Update Content
```
环境卡片: "温度 25.5°C / 湿度 60%"
能耗卡片: "今日 3.25 kWh"
安防卡片: "全部布防" 或 "客厅有人" 等
```

---

## 📱 Rooms 页面 / Rooms Tab

### 房间列表 / Room List
```
房间0: "客厅"
房间1: "主卧"
房间2: "次卧"
房间3: "厨房"
房间4: "书房"
房间5: "车库"
```

### 房间设备状态 / Room Device Status
```
初始状态: "暂无数据"
更新后: "设备5 · 在线3" (例如)
```

---

## ⚙️ System 页面 / System Tab

### 系统状态面板 / System Panel
```
面板标题: "系统状态"

Wi-Fi 标签: "Wi-Fi: 暂无数据"
更新后: "Wi-Fi: 已连接 · -50dBm"

固件标签: "固件: 暂无数据"
更新后: "固件: v1.0.0"

电源标签: "电源: 暂无数据"
更新后: "电源: 4.80V"

背光标题: "背光亮度"
背光值: "100%" (例如)
```

---

## 🎯 完整中文文本清单 / Complete Chinese Text List

### 必需的中文字符 / Required Chinese Characters

```
智 能 家 居 控 制
网 络 正 常
环 境
能 耗
安 防
暂 无 数 据
客 厅
主 卧
次 卧
厨 房
书 房
车 库
设 备
在 线
系 统 状 态
W i - F i
已 连 接
固 件
电 源
背 光 亮 度
温 度 摄 氏 度
湿 度
今 日
k W h
全 部 布 防
```

### Unicode 范围 / Unicode Range

```
中文汉字: U+4E00 ~ U+9FFF (CJK Unified Ideographs)
当前需要的范围: U+4E00 ~ U+7CFB (约 20000+ 个字符)
```

---

## 🔧 Wi-Fi 图标问题解决 / Wi-Fi Icon Issue Solution

### 问题原因 / Problem Cause
- `LV_SYMBOL_WIFI` 是 LVGL 内置符号，不在自定义中文字体中
- 显示为方框表示字体缺少该符号

### 解决方案 / Solutions

#### 方案 1: 使用系统字体显示符号（推荐）

```c
static void create_nav_bar(lv_obj_t * parent)
{
    // ... 其他代码 ...
    
    lv_obj_t *chip = lv_label_create(nav);
    lv_obj_add_style(chip, &style_chip, 0);
    
    /* 使用系统字体显示 Wi-Fi 符号 */
    lv_label_set_text_fmt(chip, LV_SYMBOL_WIFI " 网络正常");
    lv_obj_set_style_text_font(chip, &lv_font_montserrat_14, 0);  /* 使用系统字体 */
}
```

#### 方案 2: 使用中文文本替代符号

```c
static void create_nav_bar(lv_obj_t * parent)
{
    // ... 其他代码 ...
    
    lv_obj_t *chip = lv_label_create(nav);
    lv_obj_add_style(chip, &style_chip, 0);
    
    /* 用中文文本替代符号 */
    lv_label_set_text_fmt(chip, "📡 网络正常");  /* 如果字体支持 emoji */
    // 或
    lv_label_set_text_fmt(chip, "● 网络正常");  /* 用圆点替代 */
}
```

#### 方案 3: 分离符号和文本

```c
static void create_nav_bar(lv_obj_t * parent)
{
    // ... 其他代码 ...
    
    /* 创建符号标签 */
    lv_obj_t *icon = lv_label_create(nav);
    lv_label_set_text(icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_14, 0);  /* 系统字体 */
    
    /* 创建文本标签 */
    lv_obj_t *text = lv_label_create(nav);
    lv_label_set_text(text, "网络正常");
    lv_obj_add_style(text, &style_chip, 0);  /* 中文字体 */
}
```

#### 方案 4: 自定义符号（最灵活）

```c
/* 在 LVGL_Example.c 顶部定义 */
#define WIFI_ICON "📶"  /* 或其他符号 */
#define DEVICE_ICON "🏠"
#define POWER_ICON "🔋"

static void create_nav_bar(lv_obj_t * parent)
{
    // ... 其他代码 ...
    
    lv_obj_t *chip = lv_label_create(nav);
    lv_obj_add_style(chip, &style_chip, 0);
    lv_label_set_text_fmt(chip, WIFI_ICON " 网络正常");
}
```

---

## 📝 推荐的 Wi-Fi 状态文本 / Recommended Wi-Fi Status Text

```c
/* Wi-Fi 连接状态 */
"未连接"        /* Not connected */
"连接中"        /* Connecting */
"已连接"        /* Connected */
"连接失败"      /* Connection failed */
"信号弱"        /* Weak signal */
"信号强"        /* Strong signal */

/* 完整 Wi-Fi 显示 */
"Wi-Fi: 已连接 · -50dBm"
"Wi-Fi: 未连接"
"Wi-Fi: 连接中..."
```

---

## 🎨 字体配置建议 / Font Configuration Recommendations

### 当前配置 / Current Configuration

```c
#if LV_FONT_MONTSERRAT_20
#define SMART_FONT_NAV   (&lv_font_montserrat_20)
#else
#define SMART_FONT_NAV   (&lv_font_montserrat_14)
#endif

#define SMART_FONT_CN    (&my_font)  /* 自定义中文字体 */
```

### 改进建议 / Improvement Suggestions

```c
/* 为不同用途定义字体 */
#define FONT_TITLE       SMART_FONT_CN      /* 标题用中文字体 */
#define FONT_TEXT        SMART_FONT_CN      /* 文本用中文字体 */
#define FONT_SYMBOL      (&lv_font_montserrat_20)  /* 符号用系统字体 */
#define FONT_MUTED       SMART_FONT_CN      /* 灰色文本用中文字体 */
```

---

## 🔄 动态 Wi-Fi 状态更新 / Dynamic Wi-Fi Status Update

### 在 smart_ui_tick_cb 中更新

```c
static void smart_ui_tick_cb(lv_timer_t * timer)
{
    LV_UNUSED(timer);
    char buf[128];
    
    /* 更新系统数据 */
    const smart_ui_system_data_t *system_data = smart_ui_get_system_data();
    
    if (system_wifi_label) {
        if (system_data && system_data->wifi_valid) {
            snprintf(buf, sizeof(buf), "Wi-Fi: %s · -%ddBm", 
                     system_data->wifi_status, -system_data->wifi_rssi);
            lv_label_set_text(system_wifi_label, buf);
        } else {
            lv_label_set_text(system_wifi_label, "Wi-Fi: 暂无数据");
        }
    }
    
    // ... 其他更新 ...
}
```

---

## 📊 中文文本使用统计 / Chinese Text Usage Statistics

| 位置 | 文本数量 | 类型 |
|------|--------|------|
| Home 导航栏 | 2 | 标题、状态 |
| Home 卡片 | 6 | 标题、初始值 |
| Rooms 房间 | 6 | 房间名称 |
| Rooms 状态 | 多 | 设备状态 |
| System 面板 | 8 | 标题、标签、值 |
| **总计** | **约 30+** | **混合** |

---

## ✅ 检查清单 / Checklist

- [ ] 所有中文文本都使用了 `SMART_FONT_CN` 字体
- [ ] Wi-Fi 符号使用了系统字体或中文替代
- [ ] 所有标签都有初始值（通常是"暂无数据"）
- [ ] 数据更新时正确刷新标签
- [ ] 没有硬编码的英文标签（除了符号）

---

## 🎯 快速修复 Wi-Fi 图标 / Quick Fix for Wi-Fi Icon

**最简单的方案：**

```c
// 在 create_nav_bar 函数中，改这一行：
// 从:
lv_label_set_text_fmt(chip, LV_SYMBOL_WIFI " 网络正常");

// 改为:
lv_label_set_text_fmt(chip, "● 网络正常");  /* 用圆点替代 Wi-Fi 符号 */
```

或者使用中文：

```c
lv_label_set_text_fmt(chip, "📡 网络正常");  /* 如果支持 emoji */
```

或者分离字体：

```c
/* 创建一个包含符号的标签，使用系统字体 */
lv_obj_t *wifi_icon = lv_label_create(nav);
lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
lv_obj_set_style_text_font(wifi_icon, &lv_font_montserrat_14, 0);

/* 创建文本标签，使用中文字体 */
lv_obj_t *wifi_text = lv_label_create(nav);
lv_label_set_text(wifi_text, "网络正常");
lv_obj_add_style(wifi_text, &style_chip, 0);
```

---

## 相关文件 / Related Files

- `LVGL_Example.c` - UI 实现
- `smart_ui_data.c` - 数据管理
- `my_font.c` - 自定义中文字体
- `QUICK_REFERENCE.md` - 快速参考
