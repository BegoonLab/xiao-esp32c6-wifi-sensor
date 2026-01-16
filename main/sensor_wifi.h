/**
 * @file    sensor_wifi.h
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    27 September 2024
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#pragma once

#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "sensor_led.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_DISCONNECTED_BIT BIT2

esp_err_t init_wifi_sta(void);
void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                   void *event_data);
void stop_wifi(void);
esp_err_t start_wifi(void);