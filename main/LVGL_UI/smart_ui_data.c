#include "smart_ui_data.h"

/**********************
 *  全局变量定义 / GLOBAL VARIABLES
 **********************/


/* 环境数据存储 - 温度、湿度 */
smart_ui_env_data_t g_env_data = {0};

/* 能耗数据存储 - 今日能耗 */
smart_ui_energy_data_t g_energy_data = {0};

/* 安防数据存储 - 安防状态 */
smart_ui_security_data_t g_security_data = {0};

/* 房间设备状态存储 - 6个房间 */
smart_ui_room_status_t g_room_status[ROOM_COUNT] = {0};

/* 系统数据存储 - Wi-Fi、固件、电源 */
smart_ui_system_data_t g_system_data = {0};

/* 数据更新回调函数指针 - 当数据改变时调用 */
smart_ui_data_callback_t g_update_callback = NULL;




/**********************
 *   公共函数 / PUBLIC FUNCTIONS
 **********************/

/**
 * 初始化 UI 数据模块
 * 清空所有数据并重置回调函数
 */
void smart_ui_data_init(void)
{
    /* 清空所有数据结构 */
    memset(&g_env_data, 0, sizeof(g_env_data));
    memset(&g_energy_data, 0, sizeof(g_energy_data));
    memset(&g_security_data, 0, sizeof(g_security_data));
    memset(g_room_status, 0, sizeof(g_room_status));
    memset(&g_system_data, 0, sizeof(g_system_data));
    
    /* 清空回调函数指针 */
    g_update_callback = NULL;
    
    /* 初始化默认版本号 */
    strncpy(g_system_data.firmware_version, "v1.0.0", 
    sizeof(g_system_data.firmware_version) - 1);
    g_system_data.fw_valid = 1;  /* 标记固件版本有效 */
    
    /* 初始化默认电源电压 - 4.2V */
    g_system_data.power_voltage = BAT_Get_Volts();
    g_system_data.power_valid = 1;  /* 标记电源数据有效 */
    
    /* 初始化房间设备状态 - 每个房间默认5个设备，3个在线 */
    for (uint8_t i = 0; i < 6; i++) {
        g_room_status[i].total_devices = 5;
        g_room_status[i].online_devices = 3;
        g_room_status[i].is_valid = 1;  /* 标记房间数据有效 */
    }

    g_energy_data.daily_energy = 0.0;  /* 今日能耗 */
    g_energy_data.is_valid = 1;

    strncpy(g_security_data.status, "未连接", /* 安防状态 */
        sizeof(g_security_data.status) - 1);
    g_security_data.is_valid = 1;

    strncpy(g_system_data.wifi_status, "未连接", /* Wi-Fi 状态 */
        sizeof(g_system_data.wifi_status) - 1);
    g_system_data.wifi_valid = 1;

    g_system_data.fw_valid = 1;
    g_system_data.power_valid = 1;
}

/**
 * 注册数据更新回调函数
 * 当任何数据被更新时，会自动调用此回调函数
 * @param callback 回调函数指针
 */
void smart_ui_register_update_callback(smart_ui_data_callback_t callback)
{
    g_update_callback = callback;
}

/**
 * 更新环境数据（温度、湿度）
 * @param data 指向环境数据的指针
 */
void smart_ui_update_env_data(const smart_ui_env_data_t *data)
{
    if (data) {
        /* 复制数据到全局存储 */
        memcpy(&g_env_data, data, sizeof(smart_ui_env_data_t));
        /* 触发回调函数更新 UI */
        if (g_update_callback) {
            g_update_callback();
        }
    }
}

/**
 * 更新能耗数据（今日能耗）
 * @param data 指向能耗数据的指针
 */
void smart_ui_update_energy_data(const smart_ui_energy_data_t *data)
{
    if (data) {
        memcpy(&g_energy_data, data, sizeof(smart_ui_energy_data_t));
        if (g_update_callback) {
            g_update_callback();
        }
    }
}

/**
 * 更新安防数据（安防状态）
 * @param data 指向安防数据的指针
 */
void smart_ui_update_security_data(const smart_ui_security_data_t *data)
{
    if (data) {
        memcpy(&g_security_data, data, sizeof(smart_ui_security_data_t));
        if (g_update_callback) {
            g_update_callback();
        }
    }
}

/**
 * 更新房间设备状态
 * @param room_index 房间索引 (0-5)
 * @param status 指向房间状态的指针
 */
void smart_ui_update_room_status(uint8_t room_index, const smart_ui_room_status_t *status)
{
    if (room_index < 6 && status) {
        memcpy(&g_room_status[room_index], status, sizeof(smart_ui_room_status_t));
        if (g_update_callback) {
            g_update_callback();
        }
    }
}

/**
 * 更新系统数据（Wi-Fi、固件、电源）
 * @param data 指向系统数据的指针
 */
void smart_ui_update_system_data(const smart_ui_system_data_t *data)
{
    if (data) {
        memcpy(&g_system_data, data, sizeof(smart_ui_system_data_t));
        if (g_update_callback) {
            g_update_callback();
        }
    }
}

/**
 * 获取环境数据
 * @return 指向环境数据的指针
 */
const smart_ui_env_data_t* smart_ui_get_env_data(void)
{
    return &g_env_data;
}

/**
 * 获取能耗数据
 * @return 指向能耗数据的指针
 */
const smart_ui_energy_data_t* smart_ui_get_energy_data(void)
{
    return &g_energy_data;
}

/**
 * 获取安防数据
 * @return 指向安防数据的指针
 */
const smart_ui_security_data_t* smart_ui_get_security_data(void)
{
    return &g_security_data;
}

/**
 * 获取房间设备状态
 * @param room_index 房间索引 (0-5)
 * @return 指向房间状态的指针，如果索引无效则返回 NULL
 */
const smart_ui_room_status_t* smart_ui_get_room_status(uint8_t room_index)
{
    if (room_index < 6) {
        return &g_room_status[room_index];
    }
    return NULL;
}

/**
 * 获取系统数据
 * @return 指向系统数据的指针
 */
const smart_ui_system_data_t* smart_ui_get_system_data(void)
{
    return &g_system_data;
}
