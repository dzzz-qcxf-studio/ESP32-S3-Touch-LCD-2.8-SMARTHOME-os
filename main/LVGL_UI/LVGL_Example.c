#include "LVGL_Example.h"
#include "my_font.h"
#include "smart_ui_data.h"
#include "room_ui.h"
#include "ai_chat_ui.h"
#include "BAT_Driver.h"
#include "QMI8658.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/**********************
 *      DEFINES
 **********************/
#define CARD_GAP          8
#define ROOM_BTN_MIN_W    105
#define ROOM_BTN_MIN_H    90

#if LV_FONT_MONTSERRAT_20
#define SMART_FONT_NAV   (&lv_font_montserrat_20)
#else
#define SMART_FONT_NAV   LV_FONT_DEFAULT
#endif

#if LV_FONT_MONTSERRAT_18
#define SMART_FONT_VALUE (&lv_font_montserrat_18)
#else
#define SMART_FONT_VALUE LV_FONT_DEFAULT
#endif

#if defined(MY_FONT) && MY_FONT
#define SMART_FONT_CN (&my_font)
#elif LV_FONT_SIMSUN_16_CJK
#define SMART_FONT_CN (&lv_font_simsun_16_cjk)
#else
#define SMART_FONT_CN LV_FONT_DEFAULT
#endif

#define LOCAL_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_style_t style_screen;
static lv_style_t style_nav_title;
static lv_style_t style_muted;
static lv_style_t style_card;
static lv_style_t style_chip;
static lv_style_t style_nav_container;  /* 导航栏容器样式 */

static lv_obj_t *env_value_label;
static lv_obj_t *energy_value_label;
static lv_obj_t *security_value_label;
static lv_obj_t *system_wifi_label;
static lv_obj_t *system_fw_label;
static lv_obj_t *system_power_label;
static lv_obj_t *system_temp_label;  /* 系统温度标签 */
static lv_obj_t *backlight_value_label;
static lv_obj_t *room_status_labels[6];  /* 房间状态标签数组 */
static lv_obj_t *nav_wifi_text;  /* 导航栏 Wi-Fi 状态文本 */
static lv_timer_t *ui_refresh_timer;
static const char *rooms[] = {"客厅", "主卧", "次卧", "厨房", "书房", "车库"};  // 房间列表

/* UI 刷新互斥锁 - 保护多任务并发访问 LVGL */
static SemaphoreHandle_t ui_refresh_mutex = NULL;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void smart_ui_theme_init(void);
static void create_nav_bar(lv_obj_t * parent);
static void create_status_cards(lv_obj_t * parent);
static void create_room_grid(lv_obj_t * parent);
static void create_system_panel(lv_obj_t * parent);
static void configure_tab(lv_obj_t * tab);
static lv_obj_t *create_card(lv_obj_t * parent, const char * title, const char * value);
static void smart_ui_tick_cb(lv_timer_t * timer);
static void smart_ui_refresh_callback(void);  /* 数据更新时的无参数回调 */
static void backlight_slider_event(lv_event_t * e);
static void room_btn_event(lv_event_t * e);
static void ai_chat_btn_event(lv_event_t * e);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void smart_ui_main(void)
{
    lv_obj_t *screen = lv_scr_act();
    lv_obj_clean(screen);

    /* 创建 UI 刷新互斥锁 */
    if (ui_refresh_mutex == NULL) {
        ui_refresh_mutex = xSemaphoreCreateMutex();
        if (ui_refresh_mutex == NULL) {
            /* 互斥锁创建失败 - 严重错误 */
            return;
        }
    }
    
    /* 初始化数据模块 */
    smart_ui_data_init();
    /* 注册数据更新回调 - 当数据改变时立即刷新 UI */
    smart_ui_register_update_callback(smart_ui_refresh_callback);

    smart_ui_theme_init();
    lv_obj_add_style(screen, &style_screen, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

    lv_obj_t *tabview = lv_tabview_create(screen, LV_DIR_TOP, 36);
    lv_obj_set_size(tabview, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(tabview, LV_OPA_TRANSP, 0);

    lv_obj_t *tab_home = lv_tabview_add_tab(tabview, "Home");
    configure_tab(tab_home);
    create_nav_bar(tab_home);
    create_status_cards(tab_home);

    lv_obj_t *tab_rooms = lv_tabview_add_tab(tabview, "Rooms");
    configure_tab(tab_rooms);
    create_room_grid(tab_rooms);

    lv_obj_t *tab_system = lv_tabview_add_tab(tabview, "System");
    configure_tab(tab_system);
    create_system_panel(tab_system);

    if (ui_refresh_timer) {
        lv_timer_del(ui_refresh_timer);
    }
    /* 定时器用于定期检查数据更新 */
    ui_refresh_timer = lv_timer_create(smart_ui_tick_cb, 500, NULL);
}

void LVGL_Backlight_adjustment(uint8_t brightness)
{
    Set_Backlight(brightness);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void smart_ui_theme_init(void)
{
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, lv_color_hex(0xF8F9FB));
    lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);
    lv_style_set_text_color(&style_screen, lv_color_hex(0x1F1F1F));
    lv_style_set_text_font(&style_screen, SMART_FONT_CN);

    lv_style_init(&style_nav_title);
    lv_style_set_text_font(&style_nav_title, SMART_FONT_CN);
    lv_style_set_text_color(&style_nav_title, lv_color_hex(0x141414));

    lv_style_init(&style_muted);
    lv_style_set_text_color(&style_muted, lv_color_hex(0x6F7782));
    lv_style_set_text_font(&style_muted, SMART_FONT_CN);

    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, lv_color_hex(0xFFFFFF));
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
    lv_style_set_radius(&style_card, 12);
    lv_style_set_pad_all(&style_card, 14);
    lv_style_set_pad_row(&style_card, 6);
    lv_style_set_pad_column(&style_card, 6);
    lv_style_set_shadow_width(&style_card, 6);
    lv_style_set_shadow_color(&style_card, lv_color_hex(0xE3E6EE));
    lv_style_set_shadow_opa(&style_card, LV_OPA_40);

    lv_style_init(&style_chip);
    lv_style_set_bg_color(&style_chip, lv_color_hex(0xEEF2FF));
    lv_style_set_bg_opa(&style_chip, LV_OPA_COVER);
    lv_style_set_radius(&style_chip, 18);
    lv_style_set_pad_all(&style_chip, 8);
    lv_style_set_text_color(&style_chip, lv_color_hex(0x223267));
    lv_style_set_text_font(&style_chip, SMART_FONT_CN);

    /* 导航栏容器样式 - 二级导航栏 */
    lv_style_init(&style_nav_container);
    lv_style_set_bg_color(&style_nav_container, lv_color_hex(0xE8ECFF));  /* 浅蓝色背景 */
    lv_style_set_bg_opa(&style_nav_container, LV_OPA_COVER);
    lv_style_set_radius(&style_nav_container, 10);
    lv_style_set_pad_all(&style_nav_container, 6);
    lv_style_set_text_color(&style_nav_container, lv_color_hex(0x223267));
    lv_style_set_text_font(&style_nav_container, SMART_FONT_CN);
}

static void create_nav_bar(lv_obj_t * parent)
{
    lv_obj_t *nav = lv_obj_create(parent);
    lv_obj_remove_style_all(nav);
    lv_obj_set_width(nav, LV_PCT(100));
    lv_obj_set_height(nav, LV_PCT(20));
    lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(nav, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 左侧标题容器 - 使用导航栏容器样式 */
    lv_obj_t *title_container = lv_obj_create(nav);
    lv_obj_remove_style_all(title_container);
    lv_obj_set_flex_flow(title_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(title_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_style(title_container, &style_nav_container, 0);  /* 使用导航栏容器样式 */
    lv_obj_set_flex_grow(title_container, 1);  /* 平分空间 */
    
    /* 左侧图标 - 使用系统字体 */
    lv_obj_t *title_icon = lv_label_create(title_container);
    lv_label_set_text(title_icon, LV_SYMBOL_HOME);
    lv_obj_set_style_text_font(title_icon, &lv_font_montserrat_14, 0);
    
    /* 标题按钮 - 点击进入AI聊天界面 */
    lv_obj_t *title_btn = lv_btn_create(title_container);
    lv_obj_remove_style_all(title_btn);  /* 移除按钮默认样式，使其看起来像文本 */
    lv_obj_set_style_bg_opa(title_btn, LV_OPA_TRANSP, 0);  /* 透明背景 */
    lv_obj_set_style_shadow_width(title_btn, 0, 0);  /* 无阴影 */
    lv_obj_add_event_cb(title_btn, ai_chat_btn_event, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *title = lv_label_create(title_btn);
    lv_label_set_text(title, " AI管家");
    lv_obj_set_style_text_font(title, SMART_FONT_CN, 0);
    lv_obj_center(title);

    /* 右侧 Wi-Fi 图标和文本分离处理 */
    lv_obj_t *chip_container = lv_obj_create(nav);
    lv_obj_remove_style_all(chip_container);
    lv_obj_set_flex_flow(chip_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(chip_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_style(chip_container, &style_nav_container, 0);  /* 使用导航栏容器样式 */
    lv_obj_set_flex_grow(chip_container, 1);  /* 平分空间 */
    
    /* Wi-Fi 符号 - 使用系统字体 */
    lv_obj_t *wifi_icon = lv_label_create(chip_container);
    lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(wifi_icon, &lv_font_montserrat_14, 0);
    
    /* 文本 - 使用中文字体 */
    nav_wifi_text = lv_label_create(chip_container);
    lv_label_set_text(nav_wifi_text, " 未连接");
    lv_obj_set_style_text_font(nav_wifi_text, SMART_FONT_CN, 0);
}

static void create_status_cards(lv_obj_t * parent)
{
    lv_obj_t *stack = lv_obj_create(parent);
    lv_obj_remove_style_all(stack);
    lv_obj_set_width(stack, LV_PCT(100));
    lv_obj_set_height(stack, LV_PCT(80));
    lv_obj_set_flex_flow(stack, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(stack, 6, 0);

    env_value_label = create_card(stack, "环境", "暂无数据");
    energy_value_label = create_card(stack, "能耗", "暂无数据");
    security_value_label = create_card(stack, "安防", "暂无数据");
}

static void create_room_grid(lv_obj_t * parent)
{


    lv_obj_t *section = lv_obj_create(parent);
    lv_obj_remove_style_all(section);
    lv_obj_set_width(section, LV_PCT(100));
    lv_obj_set_height(section, LV_PCT(100));  // 设置高度为父容器100%
    lv_obj_set_style_pad_top(section, 6, 0);
    lv_obj_set_flex_flow(section, LV_FLEX_FLOW_COLUMN);
    
    // 允许滚动
    lv_obj_set_scrollbar_mode(section, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(section, LV_DIR_VER);

    lv_obj_t *title = lv_label_create(section);
    lv_obj_add_style(title, &style_muted, 0);
    lv_label_set_text(title, "房间列表");

    lv_obj_t *grid = lv_obj_create(section);
    lv_obj_remove_style_all(grid);
    lv_obj_set_width(grid, LV_PCT(100));
    lv_obj_set_height(grid, LV_PCT(100));  // 网格高度充满剩余空间
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(grid, 10, 0);
    lv_obj_set_style_pad_top(grid, 8, 0);
    
    // 允许网格内容超出时滚动
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(grid, LV_DIR_VER);
    lv_obj_set_flex_grow(grid, 1);  // 让网格占据所有剩余空间

    for (size_t i = 0; i < LOCAL_ARRAY_SIZE(rooms); i++) {
        lv_obj_t *btn = lv_btn_create(grid);
        lv_obj_add_style(btn, &style_card, 0);
        lv_obj_set_size(btn, ROOM_BTN_MIN_W, ROOM_BTN_MIN_H);
        lv_obj_set_style_pad_all(btn, 10, 0);
        
        /* 为按钮添加房间索引作为用户数据，用于事件处理 */
        lv_obj_set_user_data(btn, (void *)(uintptr_t)i);
        lv_obj_add_event_cb(btn, room_btn_event, LV_EVENT_CLICKED, parent);

        lv_obj_t *col = lv_obj_create(btn);
        lv_obj_remove_style_all(col);
        lv_obj_set_size(col, LV_PCT(100), LV_PCT(100));
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_gap(col, 4, 0);
        
        /* 关键：禁用容器的事件处理，让触摸事件穿透到按钮 */
        //lv_obj_add_flag(col, LV_OBJ_FLAG_IGNORE_LAYOUT);
        lv_obj_clear_flag(col, LV_OBJ_FLAG_CLICKABLE);

        /* 创建一个标签同时显示房间名称和设备状态 */
        lv_obj_t *label = lv_label_create(col);
        lv_obj_set_style_text_font(label, SMART_FONT_CN, 0);
        lv_obj_set_width(label, LV_PCT(90));
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        // 启用富文本模式（默认可能未开启）
        lv_label_set_recolor(label, true);
        
        /* 设置富文本格式，第一行房间名称加粗，第二行状态较小较淡 */
        lv_label_set_text_fmt(label, 
            "#000000 %s#\n"  // 第一行：黑色文本，闭合 #
            "#6F7782 设备%d#\n" // 第二行：灰色文本，闭合 #
            "#6F7782 在线%d#",  // 第三行：灰色文本，闭合 #
            rooms[i], 
            g_room_status[i].total_devices,
            g_room_status[i].online_devices
        );

        /* 设置样式 */
        static lv_style_t style_room_label;
        lv_style_init(&style_room_label);
        lv_style_set_text_line_space(&style_room_label, 4);  // 设置行间距
        lv_obj_add_style(label, &style_room_label, 0);

        /* 保存标签引用以便后续更新 */
        room_status_labels[i] = label;
    }
}

static void create_system_panel(lv_obj_t * parent)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_add_style(panel, &style_card, 0);
    lv_obj_set_width(panel, LV_PCT(100));
    lv_obj_set_height(panel, LV_PCT(100));  // 父容器高度的50%
    lv_obj_set_style_pad_gap(panel, 10, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, "系统状态");

    system_wifi_label = lv_label_create(panel);
    lv_obj_add_style(system_wifi_label, &style_muted, 0);
    lv_label_set_text(system_wifi_label, "Wi-Fi: 暂无数据");

    system_fw_label = lv_label_create(panel);
    lv_obj_add_style(system_fw_label, &style_muted, 0);
    lv_label_set_text(system_fw_label, "固件: 暂无数据");

    system_power_label = lv_label_create(panel);
    lv_obj_add_style(system_power_label, &style_muted, 0);
    lv_label_set_text(system_power_label, "电源: 暂无数据");

    system_temp_label = lv_label_create(panel);
    lv_obj_add_style(system_temp_label, &style_muted, 0);
    lv_label_set_text(system_temp_label, "温度: 暂无数据");

    lv_obj_t *slider_row = lv_obj_create(panel);
    lv_obj_remove_style_all(slider_row);
    lv_obj_set_width(slider_row, LV_PCT(100));
    lv_obj_set_flex_flow(slider_row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_top(slider_row, 6, 0);

    lv_obj_t *slider_title = lv_label_create(slider_row);
    lv_label_set_text(slider_title, "背光亮度");

    lv_obj_t *slider = lv_slider_create(slider_row);
    lv_slider_set_range(slider, 5, Backlight_MAX);
    lv_slider_set_value(slider, LCD_Backlight, LV_ANIM_OFF);
    lv_obj_set_width(slider, LV_PCT(100));
    lv_obj_add_event_cb(slider, backlight_slider_event, LV_EVENT_VALUE_CHANGED, NULL);

    backlight_value_label = lv_label_create(slider_row);
    lv_obj_add_style(backlight_value_label, &style_muted, 0);
    lv_label_set_text_fmt(backlight_value_label, "%d%%", LCD_Backlight);
}

static void configure_tab(lv_obj_t * tab)
{
    lv_obj_remove_style_all(tab);
    lv_obj_set_style_pad_all(tab, 8, 0);
    lv_obj_set_style_pad_gap(tab, 6, 0);
    lv_obj_set_size(tab, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
}

static lv_obj_t *create_card(lv_obj_t * parent, const char * title, const char * value)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_add_style(card, &style_card, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(card, 6, 0);
    lv_obj_set_width(card, LV_PCT(100));

    lv_obj_t *title_label = lv_label_create(card);
    lv_obj_add_style(title_label, &style_muted, 0);
    lv_label_set_text(title_label, title);

    lv_obj_t *value_label = lv_label_create(card);
    lv_label_set_text(value_label, value);
    lv_obj_set_style_text_font(value_label, SMART_FONT_CN, 0);

    return value_label;
}

/**
 * 刷新所有UI元素
 * 从数据层读取数据并更新UI显示
 */
static void refresh_all_ui_elements(void)
{
    char buf[128];  /* 增加缓冲区大小以避免截断 */
    
    /* 实时获取底层系统数据并更新数据层 */
    float current_voltage = BAT_Get_Volts();
    g_system_data.power_voltage = current_voltage;
    g_system_data.power_valid = 1;
    
    /* 实时获取芯片温度 */
    float chip_temp = getTemperature();
    
    /* 更新环境数据 */
    const smart_ui_env_data_t *env_data = smart_ui_get_env_data();
    if (env_value_label) {
        if (env_data && env_data->is_valid) {
            snprintf(buf, sizeof(buf), "湿度 %u%%", env_data->humidity);
            lv_label_set_text(env_value_label, buf);
        } else {
            lv_label_set_text(env_value_label, "暂无数据");
        }
    }
    
    /* 更新能耗数据 */
    const smart_ui_energy_data_t *energy_data = smart_ui_get_energy_data();
    if (energy_value_label) {
        if (energy_data && energy_data->is_valid) {
            snprintf(buf, sizeof(buf), "今日 %.2f kWh", energy_data->daily_energy);
            lv_label_set_text(energy_value_label, buf);
        } else {
            lv_label_set_text(energy_value_label, "暂无数据");
        }
    }
    
    /* 更新安防数据 */
    const smart_ui_security_data_t *security_data = smart_ui_get_security_data();
    if (security_value_label) {
        if (security_data && security_data->is_valid) {
            lv_label_set_text(security_value_label, security_data->status);
        } else {
            lv_label_set_text(security_value_label, "暂无数据");
        }
    }
    
    /* 更新系统数据 */
    const smart_ui_system_data_t *system_data = smart_ui_get_system_data();
    
    /* 更新导航栏 Wi-Fi 状态 */
    if (nav_wifi_text) {
        if (system_data && system_data->wifi_valid) {
            lv_label_set_text(nav_wifi_text, system_data->wifi_status);
        } else {
            lv_label_set_text(nav_wifi_text, " 未连接");
        }
    }
    
    if (system_wifi_label) {
        if (system_data && system_data->wifi_valid) {
            snprintf(buf, sizeof(buf), "Wi-Fi: %s · -%ddBm", 
                     system_data->wifi_status, -system_data->wifi_rssi);
            lv_label_set_text(system_wifi_label, buf);
        } else {
            lv_label_set_text(system_wifi_label, "Wi-Fi: 暂无数据");
        }
    }
    
    if (system_fw_label) {
        if (system_data && system_data->fw_valid) {
            snprintf(buf, sizeof(buf), "固件: %s", system_data->firmware_version);
            lv_label_set_text(system_fw_label, buf);
        } else {
            lv_label_set_text(system_fw_label, "固件: 暂无数据");
        }
    }
    
    if (system_power_label) {
        /* 实时读取电池电压 - 每次刷新都获取最新值 */
        float current_voltage = BAT_Get_Volts();
        snprintf(buf, sizeof(buf), "电源: %.2fV", current_voltage);
        lv_label_set_text(system_power_label, buf);
    }
    
    if (system_temp_label) {
        /* 实时读取芯片温度 */
        if (chip_temp != 999) {  // 999 表示未初始化或读取失败
            snprintf(buf, sizeof(buf), "温度: %.1f°C (芯片)", chip_temp);
            lv_label_set_text(system_temp_label, buf);
        } else {
            lv_label_set_text(system_temp_label, "温度: 暂无数据");
        }
    }
    
    /* 更新房间设备状态 */
    for (uint8_t i = 0; i < 6; i++) {
        if (room_status_labels[i]) {
            const smart_ui_room_status_t *room_data = smart_ui_get_room_status(i);
            if (room_data && room_data->is_valid) {
                // 在定时器更新时保留富文本样式
                snprintf(buf, sizeof(buf), 
                    "#000000 %s#\n"  // 房间名称（需传入房间名）
                    "#6F7782 设备%u#\n"
                    "#6F7782 在线%u#",
                    rooms[i],  // 需将 rooms 数组声明为全局或通过参数传入
                    room_data->total_devices, 
                    room_data->online_devices
                );
                lv_label_set_text(room_status_labels[i], buf);
            } else {
                lv_label_set_text(room_status_labels[i], "暂无数据");
            }
        }
    }
}

/**
 * 定时器回调函数
 * 每500ms定期刷新UI（用于兜底，防止遗漏更新）
 * 
 * 注意：虽然定时器在 LVGL 任务中运行，但仍使用互斥锁保护
 * 以防止与其他任务的回调冲突
 */
static void smart_ui_tick_cb(lv_timer_t * timer)
{
    LV_UNUSED(timer);
    
    /* 使用互斥锁保护，防止与回调冲突 */
    if (xSemaphoreTake(ui_refresh_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        refresh_all_ui_elements();
        xSemaphoreGive(ui_refresh_mutex);
    }
}

/**
 * 数据更新回调函数
 * 当应用层调用 ui_update_xxx() 时立即触发UI刷新
 * 
 * 使用互斥锁保证线程安全
 */
static void smart_ui_refresh_callback(void)
{
    /* 获取互斥锁：保护多任务并发访问 LVGL */
    if (xSemaphoreTake(ui_refresh_mutex, portMAX_DELAY) == pdTRUE) {
        refresh_all_ui_elements();
        xSemaphoreGive(ui_refresh_mutex);
    }
}

static void backlight_slider_event(lv_event_t * e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    uint8_t value = (uint8_t)lv_slider_get_value(slider);
    LVGL_Backlight_adjustment(value);
    if (backlight_value_label) {
        lv_label_set_text_fmt(backlight_value_label, "%u%%", value);
    }
}

/**
 * 房间按钮点击事件处理
 * 创建房间详细界面并显示
 */
static void room_btn_event(lv_event_t * e)
{
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *parent = (lv_obj_t *)lv_event_get_user_data(e);
    
    /* 获取按钮对象（可能是按钮或其子对象） */
    lv_obj_t *btn = target;
    
    /* 如果触摸的是子对象，向上查找按钮 */
    while (btn && lv_obj_get_parent(btn) != NULL) {
        void *user_data = lv_obj_get_user_data(btn);
        if (user_data != NULL) {
            break;
        }
        btn = lv_obj_get_parent(btn);
    }
    
    /* 获取房间索引 */
    uint8_t room_idx = (uint8_t)(uintptr_t)lv_obj_get_user_data(btn);
    
    /* 创建房间详细界面 */
    if (parent) {
        room_ui_create(parent, room_idx);
    }
}

/**
 * AI聊天按钮点击事件处理
 * 创建AI聊天控制界面
 */
static void ai_chat_btn_event(lv_event_t * e)
{
    /* 创建AI聊天界面 */
    ai_chat_ui_create(NULL);
}



