/**
 * @file    sensor_zb.h
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    18 October 2024
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#pragma once

#include "esp_check.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_zigbee_core.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ha/esp_zigbee_ha_standard.h"
#if defined(CONFIG_SENSOR_BME280) || defined(CONFIG_SENSOR_BME680)
#include "sensor_bme.h"
#endif
#include "sensor_id.h"
#include "sensor_led.h"
#include "sys/time.h"
#include "time.h"
#include "zcl/esp_zigbee_zcl_power_config.h"
#include <stdio.h>
#include <string.h>

/* ZigBee configuration */
#define INSTALLCODE_POLICY_ENABLE                                              \
  false /* enable the installation code policy for security */
#define ED_AGING_TIMEOUT ESP_ZB_ED_AGING_TIMEOUT_64MIN
#define ED_KEEP_ALIVE 3000 /* 3000 millisecond */
#define HA_ESP_SENSOR_ENDPOINT                                                 \
  10 /* esp temperature sensor device endpoint, used for temperature           \
        measurement */
#define ESP_ZB_PRIMARY_CHANNEL_MASK                                            \
  ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK /* Zigbee primary channel mask */

#define ESP_TEMP_SENSOR_UPDATE_INTERVAL                                        \
  (1) /* Local sensor update interval (second) */
#define ESP_TEMP_SENSOR_MIN_VALUE                                              \
  (-10) /* Local sensor min measured value (degree Celsius) */
#define ESP_TEMP_SENSOR_MAX_VALUE                                              \
  (80) /* Local sensor max measured value (degree Celsius) */

#define ESP_HUMIDITY_SENSOR_MIN_VALUE 0
#define ESP_HUMIDITY_SENSOR_MAX_VALUE 100

#define ESP_PRESSURE_SENSOR_MIN_VALUE 500
#define ESP_PRESSURE_SENSOR_MAX_VALUE 1500

#define MAX_ZCL_STRING_LEN 64

/* Attribute values in ZCL string format
 * The string should be started with the length of its own.
 */
#define MANUFACTURER_NAME "ESPRESSIF"

#define ESP_ZB_ZED_CONFIG()                                                    \
  {                                                                            \
    .esp_zb_role = ESP_ZB_DEVICE_TYPE_ED,                                      \
    .install_code_policy = INSTALLCODE_POLICY_ENABLE,                          \
    .nwk_cfg.zed_cfg = {                                                       \
        .ed_timeout = ED_AGING_TIMEOUT,                                        \
        .keep_alive = ED_KEEP_ALIVE,                                           \
    },                                                                         \
  }

#define ESP_ZB_DEFAULT_RADIO_CONFIG()                                          \
  { .radio_mode = ZB_RADIO_MODE_NATIVE, }

#define ESP_ZB_DEFAULT_HOST_CONFIG()                                           \
  { .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE, }

void sensor_zb_task(void *pvParameters);

void init_zb(void);

void start_zb(void);

int16_t zb_value_to_s16(float value);

char *build_zcl_string(const char *input_string);

void zb_deep_sleep_start(int before_deep_sleep_time_sec);

void zb_deep_sleep_init(void);

void s_oneshot_timer_callback(void *arg);

esp_zb_ep_list_t *
custom_sensor_ep_create(uint8_t endpoint_id,
                        esp_zb_configuration_tool_cfg_t *sensor);

double calculate_battery_percentage(double voltage);

void sensor_zb_update_clusters();

void sensor_zb_update_reporting_info(void);