/**
 * @file    sensor_mqtt.c
 * @author  Alexander Begoon (<a href="mailto:alex\@begoonlab.tech">alex\@begoonlab.tech</a>)
 * @date    27 September 2024
 * @brief   //TODO
 * 
 * @details //TODO
 * 
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#include "sensor_mqtt.h"

static const char *TAG = "sensor_mqtt";

static EventGroupHandle_t s_mqtt_event_group;

#define MQTT_CONNECTED_BIT    BIT0
#define MQTT_PUBLISHED_BIT    BIT1
#define MQTT_DISCONNECTED_BIT BIT2

#define WAIT pdMS_TO_TICKS(20000)

volatile esp_mqtt_client_handle_t clientHandle;
static char mqtt_topic_full[MQTT_TOPIC_MAX_LEN];

esp_mqtt_client_handle_t init_mqtt_client() {
    snprintf(mqtt_topic_full, sizeof(mqtt_topic_full), "%s/%s/data", CONFIG_MQTT_TOPIC, CONFIG_MQTT_SENSOR_ID);

    s_mqtt_event_group = xEventGroupCreate();

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_MQTT_BROKER_URI,
        .broker.address.port = CONFIG_MQTT_BROKER_PORT,
#ifdef CONFIG_MQTT_ENABLE_AUTH
        .credentials.username = CONFIG_MQTT_USERNAME,
        .credentials.authentication.password = CONFIG_MQTT_PASSWORD,
#endif
    };

    // Initialize MQTT client
    clientHandle = esp_mqtt_client_init(&mqtt_cfg);

    // Register the event handler
    esp_mqtt_client_register_event(clientHandle, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    // Start the MQTT client
    esp_err_t ret = esp_mqtt_client_start(clientHandle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client (%s)", esp_err_to_name(ret));
    }

    // Wait for connection
    EventBits_t bits = xEventGroupWaitBits(s_mqtt_event_group,
                                           MQTT_CONNECTED_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           WAIT);

    if (bits & MQTT_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to MQTT broker");
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    xEventGroupClearBits(s_mqtt_event_group, bits);

    return clientHandle;
}

void mqtt_publish(esp_mqtt_client_handle_t client, const char *data) {
    esp_err_t publish_ret = esp_mqtt_client_publish(
            client,
            mqtt_topic_full,
            data,
            0,
            CONFIG_MQTT_QOS,
            CONFIG_MQTT_RETAIN
    );

    if (publish_ret <= 0) {
        ESP_LOGE(TAG, "Failed to publish message (%s)", esp_err_to_name(publish_ret));
    }

    // Wait for published bit
    xEventGroupWaitBits(s_mqtt_event_group,
                        MQTT_PUBLISHED_BIT,
                        pdFALSE,
                        pdFALSE,
                        WAIT);
}

void stop_mqtt(esp_mqtt_client_handle_t client) {
    esp_mqtt_client_stop(client);
    esp_mqtt_client_disconnect(client);
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            xEventGroupSetBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED");
            xEventGroupSetBits(s_mqtt_event_group, MQTT_DISCONNECTED_BIT);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED");
            xEventGroupSetBits(s_mqtt_event_group, MQTT_PUBLISHED_BIT);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            break;
    }
}