#include "ai_chat_ui.h"
#include "my_font.h"
#include <stdio.h>
#include <string.h>

/**********************
 *      DEFINES
 **********************/
#define SMART_FONT_CN (&my_font)
#define MAX_MESSAGES 10  /* 最多显示10条消息 */

/**********************
 *  STATIC VARIABLES
 **********************/
/* AI聊天界面样式 */
static lv_style_t style_chat_screen;
static lv_style_t style_chat_header;
static lv_style_t style_chat_content;
static lv_style_t style_user_msg;
static lv_style_t style_ai_msg;
static lv_style_t style_voice_area;
static lv_style_t style_voice_btn;

/* 消息列表容器 */
static lv_obj_t *message_list = NULL;
/* 语音按钮和状态标签 */
static lv_obj_t *voice_btn = NULL;
static lv_obj_t *voice_state_label = NULL;

/* 语音状态 */
static ai_voice_state_t current_voice_state = AI_VOICE_IDLE;

/* 语音事件回调函数 */
static ai_voice_start_callback_t voice_start_callback = NULL;
static ai_voice_stop_callback_t voice_stop_callback = NULL;
static ai_voice_cleanup_callback_t voice_cleanup_callback = NULL;

/**********************
 *  STATIC FUNCTIONS
 **********************/

/**
 * 初始化AI聊天界面样式
 */
static void ai_chat_ui_theme_init(void)
{
    /* 屏幕背景样式 */
    lv_style_init(&style_chat_screen);
    lv_style_set_bg_color(&style_chat_screen, lv_color_hex(0xF8F9FB));
    lv_style_set_bg_opa(&style_chat_screen, LV_OPA_COVER);
    lv_style_set_text_color(&style_chat_screen, lv_color_hex(0x1F1F1F));
    lv_style_set_text_font(&style_chat_screen, SMART_FONT_CN);

    /* 头部样式 */
    lv_style_init(&style_chat_header);
    lv_style_set_bg_color(&style_chat_header, lv_color_hex(0x5B9BFF));  /* 蓝色主题 */
    lv_style_set_bg_opa(&style_chat_header, LV_OPA_COVER);
    lv_style_set_radius(&style_chat_header, 10);
    lv_style_set_pad_all(&style_chat_header, 12);
    lv_style_set_text_color(&style_chat_header, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_chat_header, SMART_FONT_CN);

    /* 聊天内容区域样式 */
    lv_style_init(&style_chat_content);
    lv_style_set_bg_color(&style_chat_content, lv_color_hex(0xFFFFFF));
    lv_style_set_bg_opa(&style_chat_content, LV_OPA_COVER);
    lv_style_set_radius(&style_chat_content, 12);
    lv_style_set_pad_all(&style_chat_content, 10);
    lv_style_set_shadow_width(&style_chat_content, 6);
    lv_style_set_shadow_color(&style_chat_content, lv_color_hex(0xE3E6EE));
    lv_style_set_shadow_opa(&style_chat_content, LV_OPA_40);

    /* 用户消息样式 */
    lv_style_init(&style_user_msg);
    lv_style_set_bg_color(&style_user_msg, lv_color_hex(0x5B9BFF));
    lv_style_set_bg_opa(&style_user_msg, LV_OPA_COVER);
    lv_style_set_radius(&style_user_msg, 10);
    lv_style_set_pad_all(&style_user_msg, 10);
    lv_style_set_text_color(&style_user_msg, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_user_msg, SMART_FONT_CN);

    /* AI消息样式 */
    lv_style_init(&style_ai_msg);
    lv_style_set_bg_color(&style_ai_msg, lv_color_hex(0xEEF2FF));
    lv_style_set_bg_opa(&style_ai_msg, LV_OPA_COVER);
    lv_style_set_radius(&style_ai_msg, 10);
    lv_style_set_pad_all(&style_ai_msg, 10);
    lv_style_set_text_color(&style_ai_msg, lv_color_hex(0x223267));
    lv_style_set_text_font(&style_ai_msg, SMART_FONT_CN);

    /* 语音区域样式 */
    lv_style_init(&style_voice_area);
    lv_style_set_bg_color(&style_voice_area, lv_color_hex(0xF0F3FF));
    lv_style_set_bg_opa(&style_voice_area, LV_OPA_COVER);
    lv_style_set_radius(&style_voice_area, 12);
    lv_style_set_pad_all(&style_voice_area, 16);
    lv_style_set_text_font(&style_voice_area, SMART_FONT_CN);

    /* 语音按钮样式 */
    lv_style_init(&style_voice_btn);
    lv_style_set_bg_color(&style_voice_btn, lv_color_hex(0x5B9BFF));
    lv_style_set_bg_opa(&style_voice_btn, LV_OPA_COVER);
    lv_style_set_radius(&style_voice_btn, LV_RADIUS_CIRCLE);  /* 圆形按钮 */
    lv_style_set_shadow_width(&style_voice_btn, 8);
    lv_style_set_shadow_color(&style_voice_btn, lv_color_hex(0x5B9BFF));
    lv_style_set_shadow_opa(&style_voice_btn, LV_OPA_50);
}

/**
 * 内部清理函数 - 释放资源并重置状态
 */
static void ai_chat_ui_cleanup(void)
{
    /* 如果正在监听或处理，调用清理回调 */
    if (voice_cleanup_callback && current_voice_state != AI_VOICE_IDLE) {
        voice_cleanup_callback();
    }
    
    /* 重置所有状态 */
    message_list = NULL;
    voice_btn = NULL;
    voice_state_label = NULL;
    current_voice_state = AI_VOICE_IDLE;
}

/**
 * 返回按钮事件处理
 */
static void chat_back_btn_event(lv_event_t *e)
{
    lv_obj_t *chat_screen = (lv_obj_t *)lv_event_get_user_data(e);
    if (chat_screen) {
        ai_chat_ui_cleanup();  /* 清理资源 */
        lv_obj_del(chat_screen);
    }
}

/**
 * 语音按钮事件处理
 */
static void voice_btn_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        /* 切换语音状态 */
        if (current_voice_state == AI_VOICE_IDLE) {
            /* 开始监听 */
            ai_chat_ui_set_voice_state(AI_VOICE_LISTENING);
            if (voice_start_callback) {
                voice_start_callback();  /* 调用开始回调 */
            }
        } else if (current_voice_state == AI_VOICE_LISTENING) {
            /* 停止监听 */
            ai_chat_ui_set_voice_state(AI_VOICE_PROCESSING);
            if (voice_stop_callback) {
                voice_stop_callback();  /* 调用停止回调 */
            }
        }
    }
}

/**
 * 创建头部（返回按钮 + 标题）
 */
static void ai_chat_ui_create_header(lv_obj_t *parent, lv_obj_t *chat_screen)
{
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_remove_style_all(header);
    lv_obj_set_width(header, LV_PCT(100));
    lv_obj_set_height(header, 0);  /* 由flex_grow控制高度 */
    lv_obj_set_flex_grow(header, 1);  /* 占比1/8 */
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_style(header, &style_chat_header, 0);
    lv_obj_set_style_pad_gap(header, 10, 0);

    /* 返回按钮 */
    lv_obj_t *back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 40, 40);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x4A8AEF), 0);
    lv_obj_set_style_radius(back_btn, 8, 0);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(back_label, &lv_font_montserrat_16, 0);
    lv_obj_center(back_label);

    /* 标题 */
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "AI助手");
    lv_obj_set_style_text_font(title, SMART_FONT_CN, 0);

    /* 返回按钮事件处理 */
    lv_obj_add_event_cb(back_btn, chat_back_btn_event, LV_EVENT_CLICKED, chat_screen);
}

/**
 * 创建聊天消息区域
 */
static void ai_chat_ui_create_chat_area(lv_obj_t *parent)
{
    lv_obj_t *chat_container = lv_obj_create(parent);
    lv_obj_add_style(chat_container, &style_chat_content, 0);
    lv_obj_set_width(chat_container, LV_PCT(100));
    lv_obj_set_height(chat_container, 0);  /* 由flex_grow控制高度 */
    lv_obj_set_flex_grow(chat_container, 5);  /* 占比5/8 */
    lv_obj_set_flex_flow(chat_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(chat_container, 8, 0);
    lv_obj_set_scrollbar_mode(chat_container, LV_SCROLLBAR_MODE_AUTO);

    /* 消息列表容器 */
    message_list = lv_obj_create(chat_container);
    lv_obj_remove_style_all(message_list);
    lv_obj_set_width(message_list, LV_PCT(100));
    lv_obj_set_height(message_list, LV_SIZE_CONTENT); // 消息列表高度自适应
    lv_obj_set_flex_flow(message_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(message_list, 8, 0);

    /* 添加欢迎消息 */
    lv_obj_t *welcome_msg = lv_obj_create(message_list);
    lv_obj_add_style(welcome_msg, &style_ai_msg, 0);
    lv_obj_set_width(welcome_msg, LV_PCT(85));
    lv_obj_set_align(welcome_msg, LV_ALIGN_LEFT_MID);
    
    lv_obj_t *welcome_label = lv_label_create(welcome_msg);
    lv_label_set_text(welcome_label, "您好！我是AI语音助手\n点击下方麦克风开始说话");
    lv_obj_set_width(welcome_label, LV_PCT(100));
    lv_label_set_long_mode(welcome_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(welcome_label, SMART_FONT_CN, 0);
}

/**
 * 创建语音区域
 */
static void ai_chat_ui_create_voice_area(lv_obj_t *parent)
{
    lv_obj_t *voice_container = lv_obj_create(parent);
    lv_obj_add_style(voice_container, &style_voice_area, 0);
    lv_obj_set_width(voice_container, LV_PCT(100));
    lv_obj_set_height(voice_container, 0);  /* 由flex_grow控制高度 */
    lv_obj_set_flex_grow(voice_container, 3);  /* 占比2/8 */
    lv_obj_set_flex_flow(voice_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(voice_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(voice_container, 12, 0);

    /* 语音按钮 - 大型圆形按钮 */
    voice_btn = lv_btn_create(voice_container);
    lv_obj_add_style(voice_btn, &style_voice_btn, 0);
    lv_obj_set_size(voice_btn, 60, 60);  /* 大尺寸按钮 */
    
    /* 麦克风图标 */
    lv_obj_t *mic_icon = lv_label_create(voice_btn);
    lv_label_set_text(mic_icon, LV_SYMBOL_AUDIO);  /* 使用LVGL内置音频图标 */
    /* 使用可用的字体，优先使用较大的字体 */
    #if LV_FONT_MONTSERRAT_28
        lv_obj_set_style_text_font(mic_icon, &lv_font_montserrat_28, 0);
    #elif LV_FONT_MONTSERRAT_24
        lv_obj_set_style_text_font(mic_icon, &lv_font_montserrat_24, 0);
    #elif LV_FONT_MONTSERRAT_20
        lv_obj_set_style_text_font(mic_icon, &lv_font_montserrat_20, 0);
    #else
        lv_obj_set_style_text_font(mic_icon, &lv_font_montserrat_16, 0);
    #endif
    lv_obj_set_style_text_color(mic_icon, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(mic_icon);

    /* 语音按钮事件处理 */
    lv_obj_add_event_cb(voice_btn, voice_btn_event, LV_EVENT_CLICKED, NULL);

    /* 状态标签 */
    voice_state_label = lv_label_create(voice_container);
    lv_label_set_text(voice_state_label, "点击麦克风开始说话");
    lv_obj_set_style_text_font(voice_state_label, SMART_FONT_CN, 0);
    lv_obj_set_style_text_color(voice_state_label, lv_color_hex(0x6F7782), 0);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * 创建AI聊天控制界面
 */
lv_obj_t *ai_chat_ui_create(lv_obj_t *parent)
{
    /* 初始化样式（仅首次调用） */
    static uint8_t style_initialized = 0;
    if (!style_initialized) {
        ai_chat_ui_theme_init();
        style_initialized = 1;
    }
    
    /* 确保状态重置（防止上次退出时未正常清理） */
    current_voice_state = AI_VOICE_IDLE;

    /* 获取屏幕对象作为父容器，全屏显示 */
    lv_obj_t *screen = lv_scr_act();
    
    /* 创建AI聊天屏幕容器 - 全屏覆盖 */
    lv_obj_t *chat_screen = lv_obj_create(screen);
    lv_obj_remove_style_all(chat_screen);
    lv_obj_set_size(chat_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(chat_screen, &style_chat_screen, 0);
    lv_obj_set_flex_flow(chat_screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(chat_screen, 8, 0);
    lv_obj_set_style_pad_gap(chat_screen, 8, 0);

    /* 创建头部（返回按钮 + 标题） */
    ai_chat_ui_create_header(chat_screen, chat_screen);

    /* 创建聊天消息区域 */
    ai_chat_ui_create_chat_area(chat_screen);

    /* 创建语音区域 */
    ai_chat_ui_create_voice_area(chat_screen);

    return chat_screen;
}

/**
 * 销毁AI聊天控制界面
 */
void ai_chat_ui_destroy(lv_obj_t *chat_screen)
{
    if (chat_screen) {
        ai_chat_ui_cleanup();  /* 清理资源 */
        lv_obj_del(chat_screen);
    }
}

/**
 * 添加消息到聊天区域
 */
void ai_chat_ui_add_message(uint8_t is_user, const char *message)
{
    if (!message_list || !message) return;

    /* 创建消息气泡 */
    lv_obj_t *msg_bubble = lv_obj_create(message_list);
    lv_obj_add_style(msg_bubble, is_user ? &style_user_msg : &style_ai_msg, 0);
    lv_obj_set_width(msg_bubble, LV_PCT(85));
    lv_obj_set_align(msg_bubble, is_user ? LV_ALIGN_RIGHT_MID : LV_ALIGN_LEFT_MID);
    
    /* 消息文本 */
    lv_obj_t *msg_label = lv_label_create(msg_bubble);
    lv_label_set_text(msg_label, message);
    lv_obj_set_width(msg_label, LV_PCT(100));
    lv_label_set_long_mode(msg_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(msg_label, SMART_FONT_CN, 0);

    /* 滚动到最新消息 */
    lv_obj_scroll_to_view(msg_bubble, LV_ANIM_ON);
}

/**
 * 注册语音开始回调函数
 */
void ai_chat_ui_register_voice_start_callback(ai_voice_start_callback_t callback)
{
    voice_start_callback = callback;
}

/**
 * 注册语音停止回调函数
 */
void ai_chat_ui_register_voice_stop_callback(ai_voice_stop_callback_t callback)
{
    voice_stop_callback = callback;
}

/**
 * 注册资源清理回调函数
 */
void ai_chat_ui_register_voice_cleanup_callback(ai_voice_cleanup_callback_t callback)
{
    voice_cleanup_callback = callback;
}

/**
 * 设置语音状态
 */
void ai_chat_ui_set_voice_state(ai_voice_state_t state)
{
    current_voice_state = state;
    
    /* 更新UI显示 */
    if (voice_state_label) {
        switch (state) {
            case AI_VOICE_IDLE:
                lv_label_set_text(voice_state_label, "点击麦克风开始说话");
                if (voice_btn) {
                    lv_obj_set_style_bg_color(voice_btn, lv_color_hex(0x5B9BFF), 0);  /* 蓝色 */
                }
                break;
            case AI_VOICE_LISTENING:
                lv_label_set_text(voice_state_label, "正在监听...再次点击结束");
                if (voice_btn) {
                    lv_obj_set_style_bg_color(voice_btn, lv_color_hex(0xFF5B5B), 0);  /* 红色 */
                }
                break;
            case AI_VOICE_PROCESSING:
                lv_label_set_text(voice_state_label, "正在处理您的请求...");
                if (voice_btn) {
                    lv_obj_set_style_bg_color(voice_btn, lv_color_hex(0xFFB84D), 0);  /* 橙色 */
                }
                break;
            case AI_VOICE_SPEAKING:
                lv_label_set_text(voice_state_label, "正在播放AI回复...");
                if (voice_btn) {
                    lv_obj_set_style_bg_color(voice_btn, lv_color_hex(0x4CAF50), 0);  /* 绿色 */
                }
                break;
        }
    }
}

/**
 * 获取当前语音状态
 */
ai_voice_state_t ai_chat_ui_get_voice_state(void)
{
    return current_voice_state;
}
