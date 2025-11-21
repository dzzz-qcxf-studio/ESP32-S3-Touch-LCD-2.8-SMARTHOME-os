#pragma once

#include "lvgl.h"

/**
 * 房间界面管理模块
 * Room UI Management Module
 * 
 * 功能：
 * - 创建房间详细界面
 * - 管理房间界面的显示和隐藏
 * - 处理房间界面的事件
 */

/* 房间索引定义 */
typedef enum {
    ROOM_LIVING_ROOM = 0,    /* 客厅 */
    ROOM_MASTER_BEDROOM = 1, /* 主卧 */
    ROOM_SECOND_BEDROOM = 2, /* 次卧 */
    ROOM_KITCHEN = 3,        /* 厨房 */
    ROOM_STUDY = 4,          /* 书房 */
    ROOM_GARAGE = 5,         /* 车库 */
    ROOM_COUNT = 6
} room_index_t;

/**
 * 创建房间详细界面
 * 
 * @param parent 父容器
 * @param room_idx 房间索引 (0-5)
 * @return 房间界面对象指针
 */
lv_obj_t *room_ui_create(lv_obj_t *parent, uint8_t room_idx);

/**
 * 销毁房间详细界面
 * 
 * @param room_screen 房间界面对象指针
 */
void room_ui_destroy(lv_obj_t *room_screen);

/**
 * 获取房间名称
 * 
 * @param room_idx 房间索引
 * @return 房间名称字符串
 */
const char *room_ui_get_name(uint8_t room_idx);
