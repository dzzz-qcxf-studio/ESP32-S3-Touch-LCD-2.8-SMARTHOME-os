#include "room_ui.h"
#include "my_font.h"
#include <stdio.h>

/**********************
 *      DEFINES
 **********************/
#define SMART_FONT_CN (&my_font)

/**********************
 *  STATIC VARIABLES
 **********************/
/* 房间名称数组 */
static const char *room_names[] = {
    "客厅",
    "主卧",
    "次卧",
    "厨房",
    "书房",
    "车库"
};

/* 房间界面样式 */
static lv_style_t style_room_screen;
static lv_style_t style_room_header;
static lv_style_t style_room_content;

/**********************
 *  STATIC FUNCTIONS
 **********************/

/**
 * 初始化房间界面样式
 */
static void room_ui_theme_init(void)
{
    /* 屏幕背景样式 */
    lv_style_init(&style_room_screen);
    lv_style_set_bg_color(&style_room_screen, lv_color_hex(0xF8F9FB));
    lv_style_set_bg_opa(&style_room_screen, LV_OPA_COVER);
    lv_style_set_text_color(&style_room_screen, lv_color_hex(0x1F1F1F));
    lv_style_set_text_font(&style_room_screen, SMART_FONT_CN);

    /* 房间头部样式 */
    lv_style_init(&style_room_header);
    lv_style_set_bg_color(&style_room_header, lv_color_hex(0xE8ECFF));
    lv_style_set_bg_opa(&style_room_header, LV_OPA_COVER);
    lv_style_set_radius(&style_room_header, 10);
    lv_style_set_pad_all(&style_room_header, 12);
    lv_style_set_text_color(&style_room_header, lv_color_hex(0x223267));
    lv_style_set_text_font(&style_room_header, SMART_FONT_CN);

    /* 房间内容样式 */
    lv_style_init(&style_room_content);
    lv_style_set_bg_color(&style_room_content, lv_color_hex(0xFFFFFF));
    lv_style_set_bg_opa(&style_room_content, LV_OPA_COVER);
    lv_style_set_radius(&style_room_content, 12);
    lv_style_set_pad_all(&style_room_content, 14);
    lv_style_set_shadow_width(&style_room_content, 6);
    lv_style_set_shadow_color(&style_room_content, lv_color_hex(0xE3E6EE));
    lv_style_set_shadow_opa(&style_room_content, LV_OPA_40);
}

/**
 * 返回按钮事件处理
 */
static void room_back_btn_event(lv_event_t *e)
{
    lv_obj_t *room_screen = (lv_obj_t *)lv_event_get_user_data(e);
    if (room_screen) {
        lv_obj_del(room_screen);
    }
}

/**
 * 创建房间头部（返回按钮 + 房间名称）
 */
static void room_ui_create_header(lv_obj_t *parent, uint8_t room_idx)
{
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_remove_style_all(header);
    lv_obj_set_width(header, LV_PCT(100));
    lv_obj_set_height(header, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_style(header, &style_room_header, 0);
    lv_obj_set_style_pad_gap(header, 10, 0);

    /* 返回按钮 */
    lv_obj_t *back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 40, 40);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0xD0D8FF), 0);
    lv_obj_set_style_radius(back_btn, 8, 0);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(back_label, &lv_font_montserrat_16, 0);
    lv_obj_center(back_label);

    /* 房间名称 */
    lv_obj_t *room_name = lv_label_create(header);
    lv_label_set_text(room_name, room_names[room_idx]);
    lv_obj_set_style_text_font(room_name, SMART_FONT_CN, 0);

    /* 返回按钮事件处理 - 保存父容器作为用户数据 */
    lv_obj_add_event_cb(back_btn, room_back_btn_event, LV_EVENT_CLICKED, parent);
}

/**
 * 创建房间内容区域（空白占位符）
 */
static void room_ui_create_content(lv_obj_t *parent, uint8_t room_idx)
{
    lv_obj_t *content = lv_obj_create(parent);
    lv_obj_add_style(content, &style_room_content, 0);
    lv_obj_set_width(content, LV_PCT(100));
    lv_obj_set_height(content, LV_PCT(100));
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 占位符文本 */
    lv_obj_t *placeholder = lv_label_create(content);
    lv_label_set_text(placeholder, "房间详情\n（待开发）");
    lv_obj_set_style_text_align(placeholder, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(placeholder, SMART_FONT_CN, 0);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * 创建房间详细界面
 */
lv_obj_t *room_ui_create(lv_obj_t *parent, uint8_t room_idx)
{
    /* 验证房间索引 */
    if (room_idx >= ROOM_COUNT) {
        return NULL;
    }

    /* 初始化样式（仅首次调用） */
    static uint8_t style_initialized = 0;
    if (!style_initialized) {
        room_ui_theme_init();
        style_initialized = 1;
    }

    /* 获取屏幕对象作为父容器，这样房间界面会全屏显示 */
    lv_obj_t *screen = lv_scr_act();
    
    /* 创建房间屏幕容器 - 全屏覆盖 */
    lv_obj_t *room_screen = lv_obj_create(screen);
    lv_obj_remove_style_all(room_screen);
    lv_obj_set_size(room_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(room_screen, &style_room_screen, 0);
    lv_obj_set_flex_flow(room_screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(room_screen, 8, 0);
    lv_obj_set_style_pad_gap(room_screen, 8, 0);

    /* 创建头部（返回按钮 + 房间名称） */
    room_ui_create_header(room_screen, room_idx);

    /* 创建内容区域 */
    room_ui_create_content(room_screen, room_idx);

    return room_screen;
}

/**
 * 销毁房间详细界面
 */
void room_ui_destroy(lv_obj_t *room_screen)
{
    if (room_screen) {
        lv_obj_del(room_screen);
    }
}

/**
 * 获取房间名称
 */
const char *room_ui_get_name(uint8_t room_idx)
{
    if (room_idx < ROOM_COUNT) {
        return room_names[room_idx];
    }
    return "未知房间";
}
