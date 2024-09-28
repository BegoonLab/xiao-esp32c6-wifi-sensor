/**
 * @file    sensor_mqtt.h
 * @author  Alexander Begoon (<a href="mailto:alex\@begoonlab.tech">alex\@begoonlab.tech</a>)
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

#include "mqtt_client.h"

#include "freertos/event_groups.h"

#ifdef CONFIG_MQTT_QOS_0
#define CONFIG_MQTT_QOS 0
#elif CONFIG_MQTT_QOS_1
#define CONFIG_MQTT_QOS 1
#elif CONFIG_MQTT_QOS_2
#define CONFIG_MQTT_QOS 2
#endif

#define MQTT_TOPIC_MAX_LEN 256

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

esp_mqtt_client_handle_t init_mqtt_client();

void mqtt_publish(esp_mqtt_client_handle_t client, const char *data);
void stop_mqtt(esp_mqtt_client_handle_t client);