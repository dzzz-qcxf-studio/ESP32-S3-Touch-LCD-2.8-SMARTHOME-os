#pragma once

#include "lvgl.h"

/**
 * AI聊天控制界面管理模块
 * AI Chat Control UI Management Module
 * 
 * 功能：
 * - 创建AI语音聊天控制界面
 * - 管理聊天消息的显示
 * - 处理语音交互
 */

/* 语音状态枚举 */
typedef enum {
    AI_VOICE_IDLE = 0,        /* 空闲状态 */
    AI_VOICE_LISTENING = 1,   /* 正在监听 */
    AI_VOICE_PROCESSING = 2,  /* 正在处理 */
    AI_VOICE_SPEAKING = 3     /* 正在播放 */
} ai_voice_state_t;

/* 语音事件回调函数类型 */
typedef void (*ai_voice_start_callback_t)(void);   /* 开始语音输入回调 */
typedef void (*ai_voice_stop_callback_t)(void);    /* 停止语音输入回调 */
typedef void (*ai_voice_cleanup_callback_t)(void); /* 资源清理回调（退出界面时） */

/**
 * 创建AI聊天控制界面
 * 
 * @param parent 父容器（通常不使用，会全屏显示）
 * @return AI聊天界面对象指针
 */
lv_obj_t *ai_chat_ui_create(lv_obj_t *parent);

/**
 * 销毁AI聊天控制界面
 * 
 * @param chat_screen AI聊天界面对象指针
 */
void ai_chat_ui_destroy(lv_obj_t *chat_screen);

/**
 * 添加消息到聊天区域
 * 
 * @param is_user 是否是用户消息（true: 用户，false: AI）
 * @param message 消息内容
 */
void ai_chat_ui_add_message(uint8_t is_user, const char *message);

/**
 * 注册语音开始回调函数
 * 当用户点击语音按钮开始说话时调用
 * 
 * @param callback 回调函数指针
 */
void ai_chat_ui_register_voice_start_callback(ai_voice_start_callback_t callback);

/**
 * 注册语音停止回调函数
 * 当用户松开语音按钮或再次点击时调用
 * 
 * @param callback 回调函数指针
 */
void ai_chat_ui_register_voice_stop_callback(ai_voice_stop_callback_t callback);

/**
 * 注册资源清理回调函数
 * 当用户退出AI界面时调用，用于释放语音资源和停止正在进行的语音任务
 * 
 * @param callback 回调函数指针
 */
void ai_chat_ui_register_voice_cleanup_callback(ai_voice_cleanup_callback_t callback);

/**
 * 设置语音状态
 * 更新UI显示当前语音交互状态
 * 
 * @param state 语音状态
 */
void ai_chat_ui_set_voice_state(ai_voice_state_t state);

/**
 * 获取当前语音状态
 * 
 * @return 当前语音状态
 */
ai_voice_state_t ai_chat_ui_get_voice_state(void);
