/**
 * @file    sensor_zb.h
 * @author  Alexander Begoon (<a href="mailto:alex\@begoonlab.tech">alex\@begoonlab.tech</a>)
 * @date    18 October 2024
 * @brief   //TODO
 * 
 * @details //TODO
 * 
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#pragma once

#include "esp_zigbee_core.h"
#include "esp_check.h"
#include "esp_log.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ZigBee configuration */
#define INSTALLCODE_POLICY_ENABLE       false   /* enable the installation code policy for security */
#define ED_AGING_TIMEOUT                ESP_ZB_ED_AGING_TIMEOUT_64MIN
#define ED_KEEP_ALIVE                   3000    /* 3000 millisecond */
#define HA_ESP_SENSOR_ENDPOINT          10      /* esp temperature sensor device endpoint, used for temperature measurement */
#define ESP_ZB_PRIMARY_CHANNEL_MASK     ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK    /* Zigbee primary channel mask */

#define ESP_TEMP_SENSOR_UPDATE_INTERVAL (1)     /* Local sensor update interval (second) */
#define ESP_TEMP_SENSOR_MIN_VALUE       (-10)   /* Local sensor min measured value (degree Celsius) */
#define ESP_TEMP_SENSOR_MAX_VALUE       (80)    /* Local sensor max measured value (degree Celsius) */

/* Attribute values in ZCL string format
 * The string should be started with the length of its own.
 */
#define MANUFACTURER_NAME               "\x09""ESPRESSIF"
#define MODEL_IDENTIFIER                "\x07"CONFIG_IDF_TARGET

#define ESP_ZB_ZED_CONFIG()                                         \
    {                                                               \
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_ED,                       \
        .install_code_policy = INSTALLCODE_POLICY_ENABLE,           \
        .nwk_cfg.zed_cfg = {                                        \
            .ed_timeout = ED_AGING_TIMEOUT,                         \
            .keep_alive = ED_KEEP_ALIVE,                            \
        },                                                          \
    }

#define ESP_ZB_DEFAULT_RADIO_CONFIG()                           \
    {                                                           \
        .radio_mode = ZB_RADIO_MODE_NATIVE,                     \
    }

#define ESP_ZB_DEFAULT_HOST_CONFIG()                            \
    {                                                           \
        .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE,   \
    }

static void sensor_zb_task(void *pvParameters);

void init_zb(void);

void start_zb(void);


static int16_t zb_temperature_to_s16(float temp);

static esp_zb_ep_list_t *
custom_temperature_sensor_ep_create(uint8_t endpoint_id, esp_zb_temperature_sensor_cfg_t *temperature_sensor);

static esp_zb_cluster_list_t *
custom_temperature_sensor_clusters_create(esp_zb_temperature_sensor_cfg_t *temperature_sensor);