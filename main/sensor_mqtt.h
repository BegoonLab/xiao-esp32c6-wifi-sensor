/**
 * @file    sensor_mqtt.h
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

#include "esp_event.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "mqtt_client.h"
#include "sensor_data.h"
#include "sensor_id.h"
#include "sensor_led.h"
#include "sensor_wifi.h"
#include "sys/time.h"
#include <cJSON.h>

#ifdef CONFIG_MQTT_QOS_0
#define CONFIG_MQTT_QOS 0
#elif CONFIG_MQTT_QOS_1
#define CONFIG_MQTT_QOS 1
#elif CONFIG_MQTT_QOS_2
#define CONFIG_MQTT_QOS 2
#endif

#define MQTT_TOPIC_MAX_LEN 256
#define MQTT_MSG_MAX_LEN 512

#define MQTT_CONNECTED_BIT BIT0
#define MQTT_PUBLISHED_BIT BIT1
#define MQTT_DISCONNECTED_BIT BIT2

#define WAIT pdMS_TO_TICKS(20000)

void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                        int32_t event_id, void *event_data);

esp_mqtt_client_handle_t init_mqtt_client();

void mqtt_publish(esp_mqtt_client_handle_t client, const char *data);
void stop_mqtt(esp_mqtt_client_handle_t client);
void mqtt_prepare_json(char *json_string, int rssi,
                       struct timeval start_to_connect,
                       struct timeval end_to_connect);
void mqtt_send_sensor_data(void);