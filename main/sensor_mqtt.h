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

void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                        int32_t event_id, void *event_data);

esp_mqtt_client_handle_t init_mqtt_client();

void mqtt_publish(esp_mqtt_client_handle_t client, const char *data);
void stop_mqtt(esp_mqtt_client_handle_t client);
void mqtt_prepare_json(char *json_string, int rssi, double battery_voltage,
                       double temperature, double humidity, double pressure,
                       struct timeval start_to_connect,
                       struct timeval end_to_connect);