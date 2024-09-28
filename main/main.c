/**
 * @file    main.c
 * @author  Alexander Begoon (<a href="mailto:alex\@begoonlab.tech">alex\@begoonlab.tech</a>)
 * @date    27 September 2024
 * @brief   //TODO
 *
 * @details //TODO
 * 
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#include "sdkconfig.h"
#include <cJSON.h>
#include "sensor_nvs.h"
#include "sensor_wifi.h"
#include "sensor_mqtt.h"
#include "sensor_gpio.h"
#include "esp_sleep.h"

static const char *TAG = "sensor_main";

void app_main(void) {
    init_nvs();
    init_gpio();

    init_wifi_sta();
    esp_mqtt_client_handle_t mqtt_client = init_mqtt_client();

    while (true) {
        int rssi;
        esp_wifi_sta_get_rssi(&rssi);
        ESP_LOGI(TAG, "RSSI: %d", rssi);

        // Test
        // Publish MQTT message
        // Create cJSON object
        cJSON *json = cJSON_CreateObject();
        cJSON_AddStringToObject(json, "ID", CONFIG_MQTT_SENSOR_ID);
        cJSON_AddNumberToObject(json, "RSSI", rssi);
        cJSON_AddNumberToObject(json, "battery_voltage", 3.7);
        cJSON_AddNumberToObject(json, "temperature", 26.3);
        cJSON_AddNumberToObject(json, "humidity", 46.3);
        cJSON_AddNumberToObject(json, "pressure", 1002.5);

        // Convert cJSON to string
        char *json_string = cJSON_PrintUnformatted(json);

        mqtt_publish(mqtt_client, json_string);

        // Cleanup
        free(json_string);
        cJSON_Delete(json);

        stop_mqtt(mqtt_client);
        stop_wifi();

        const int wakeup_time_sec = 300;
        ESP_LOGI(TAG, "Enabling timer wakeup, %ds\n", wakeup_time_sec);
        ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));

        // enter deep sleep
        esp_deep_sleep_start();
    }
}