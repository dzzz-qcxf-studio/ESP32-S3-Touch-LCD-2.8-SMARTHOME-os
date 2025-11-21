#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "BAT_Driver.h"
#include "room_ui.h"

/**********************
 *   DATA STRUCTURES
 **********************/

/**
 * 环境数据结构
 */
typedef struct {
    float temperature;      /**< 温度 (°C) */
    uint8_t humidity;       /**< 湿度 (%) */
    bool is_valid;          /**< 数据是否有效 */
} smart_ui_env_data_t;

/**
 * 能耗数据结构
 */
typedef struct {
    float daily_energy;     /**< 今日能耗 (kWh) */
    bool is_valid;          /**< 数据是否有效 */
} smart_ui_energy_data_t;

/**
 * 安防数据结构
 */
typedef struct {
    char status[32];        /**< 安防状态文本 */
    bool is_valid;          /**< 数据是否有效 */
} smart_ui_security_data_t;

/**
 * 房间设备状态结构
 */
typedef struct {
    uint8_t total_devices;  /**< 总设备数 */
    uint8_t online_devices; /**< 在线设备数 */
    bool is_valid;          /**< 数据是否有效 */
} smart_ui_room_status_t;

/**
 * 系统状态数据结构
 */
typedef struct {
    char wifi_status[48];   /**< Wi-Fi 状态文本 */
    int8_t wifi_rssi;       /**< Wi-Fi 信号强度 (dBm) */
    char firmware_version[16]; /**< 固件版本 */
    float power_voltage;    /**< 电源电压 (V) */
    bool wifi_valid;        /**< Wi-Fi 数据是否有效 */
    bool fw_valid;          /**< 固件数据是否有效 */
    bool power_valid;       /**< 电源数据是否有效 */
} smart_ui_system_data_t;



/**********************
 *   CALLBACK TYPES
 **********************/

/**
 * 数据更新回调函数类型
 */
typedef void (*smart_ui_data_callback_t)(void);

/**********************
 *   PUBLIC FUNCTIONS
 **********************/

/**
 * 初始化 UI 数据模块
 */
void smart_ui_data_init(void);

/**
 * 注册数据更新回调（在数据改变时调用）
 * @param callback 回调函数指针
 */
void smart_ui_register_update_callback(smart_ui_data_callback_t callback);

/**
 * 更新环境数据
 * @param data 环境数据指针
 */
void smart_ui_update_env_data(const smart_ui_env_data_t *data);

/**
 * 更新能耗数据
 * @param data 能耗数据指针
 */
void smart_ui_update_energy_data(const smart_ui_energy_data_t *data);

/**
 * 更新安防数据
 * @param data 安防数据指针
 */
void smart_ui_update_security_data(const smart_ui_security_data_t *data);

/**
 * 更新房间设备状态
 * @param room_index 房间索引 (0-5)
 * @param status 房间状态指针
 */
void smart_ui_update_room_status(uint8_t room_index, const smart_ui_room_status_t *status);

/**
 * 更新系统状态数据
 * @param data 系统状态数据指针
 */
void smart_ui_update_system_data(const smart_ui_system_data_t *data);

/**
 * 获取环境数据
 * @return 环境数据指针
 */
const smart_ui_env_data_t* smart_ui_get_env_data(void);

/**
 * 获取能耗数据
 * @return 能耗数据指针
 */
const smart_ui_energy_data_t* smart_ui_get_energy_data(void);

/**
 * 获取安防数据
 * @return 安防数据指针
 */
const smart_ui_security_data_t* smart_ui_get_security_data(void);

/**
 * 获取房间设备状态
 * @param room_index 房间索引 (0-5)
 * @return 房间状态指针
 */
const smart_ui_room_status_t* smart_ui_get_room_status(uint8_t room_index);

/**
 * 获取系统状态数据
 * @return 系统状态数据指针
 */
const smart_ui_system_data_t* smart_ui_get_system_data(void);



/**********************
 *  全局变量声明 / GLOBAL VARIABLES
 **********************/

/* 环境数据存储 - 温度、湿度 */
extern smart_ui_env_data_t g_env_data;

/* 能耗数据存储 - 今日能耗 */
extern smart_ui_energy_data_t g_energy_data;

/* 安防数据存储 - 安防状态 */
extern smart_ui_security_data_t g_security_data;

/* 房间设备状态存储 - 6个房间 */
extern smart_ui_room_status_t g_room_status[ROOM_COUNT];

/* 系统数据存储 - Wi-Fi、固件、电源 */
extern smart_ui_system_data_t g_system_data;

/* 数据更新回调函数指针 - 当数据改变时调用 */
extern smart_ui_data_callback_t g_update_callback;