#pragma once

#include <string.h>
#include "lvgl.h"
#include "LVGL_Driver.h"
#include "ST7789.h"
#include "smart_ui_data.h"

void smart_ui_main(void);
void LVGL_Backlight_adjustment(uint8_t brightness);

/* 数据接口函数 - 用于外部模块更新 UI 数据 */
/* Data interface functions - for external modules to update UI data */

/**
 * 更新环境数据并刷新 UI
 * Update environment data and refresh UI
 */
static inline void ui_update_environment(float temp, uint8_t humidity)
{
    smart_ui_env_data_t data = {
        .temperature = temp,
        .humidity = humidity,
        .is_valid = 1
    };
    smart_ui_update_env_data(&data);
}

/**
 * 更新能耗数据并刷新 UI
 * Update energy data and refresh UI
 */
static inline void ui_update_energy(float daily_kwh)
{
    smart_ui_energy_data_t data = {
        .daily_energy = daily_kwh,
        .is_valid = 1
    };
    smart_ui_update_energy_data(&data);
}

/**
 * 更新安防状态并刷新 UI
 * Update security status and refresh UI
 */
static inline void ui_update_security(const char *status)
{
    smart_ui_security_data_t data = {0};
    if (status) {
        strncpy(data.status, status, sizeof(data.status) - 1);
        data.is_valid = 1;
    }
    smart_ui_update_security_data(&data);
}

/**
 * 更新房间设备状态并刷新 UI
 * Update room device status and refresh UI
 */
static inline void ui_update_room(uint8_t room_idx, uint8_t total, uint8_t online)
{
    smart_ui_room_status_t data = {
        .total_devices = total,
        .online_devices = online,
        .is_valid = 1
    };
    smart_ui_update_room_status(room_idx, &data);
}

/**
 * 更新系统状态并刷新 UI
 * Update system status and refresh UI
 */
static inline void ui_update_system(const char *wifi_status, int8_t rssi,
                                    const char *fw_version, float voltage)
{
    smart_ui_system_data_t data = {0};
    
    if (wifi_status) {
        strncpy(data.wifi_status, wifi_status, sizeof(data.wifi_status) - 1);
        data.wifi_valid = 1;
    }
    data.wifi_rssi = rssi;
    
    if (fw_version) {
        strncpy(data.firmware_version, fw_version, sizeof(data.firmware_version) - 1);
        data.fw_valid = 1;
    }
    
    data.power_voltage = voltage;
    data.power_valid = 1;
    
    smart_ui_update_system_data(&data);
}