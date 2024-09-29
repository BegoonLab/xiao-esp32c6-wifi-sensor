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
#include "sensor_adc.h"
#include "esp_sleep.h"
#include "sys/time.h"

static const char *TAG = "sensor_main";

void app_main(void) {
    init_nvs();
    init_gpio();
    init_adc();

    double battery_voltage = 0.0;

//    while (true) {
//        battery_voltage = 0.0;
        get_battery_voltage(&battery_voltage);

        ESP_LOGI(TAG, "Battery Voltage: %.2f V", battery_voltage);

//        vTaskDelay(pdMS_TO_TICKS(1000));
//    }

    deinit_adc();

    struct timeval start_to_connect;
    gettimeofday(&start_to_connect, NULL);

    init_wifi_sta();
    esp_mqtt_client_handle_t mqtt_client = init_mqtt_client();

    struct timeval end_to_connect;
    gettimeofday(&end_to_connect, NULL);

    while (true) {
        int rssi = -100;
        esp_wifi_sta_get_rssi(&rssi);
        ESP_LOGI(TAG, "RSSI: %d", rssi);

        // Test
        // Publish MQTT message
        // Create cJSON object
        cJSON *json = cJSON_CreateObject();
        cJSON_AddStringToObject(json, "ID", CONFIG_MQTT_SENSOR_ID);
        cJSON_AddNumberToObject(json, "RSSI", rssi);
        cJSON_AddNumberToObject(json, "battery_voltage", battery_voltage);
        cJSON_AddNumberToObject(json, "temperature", 26.3);
        cJSON_AddNumberToObject(json, "humidity", 46.3);
        cJSON_AddNumberToObject(json, "pressure", 1002.5);
        cJSON_AddNumberToObject(json, "connection_duration_ms",
                                (end_to_connect.tv_sec - start_to_connect.tv_sec) * 1000 +
                                (end_to_connect.tv_usec - start_to_connect.tv_usec) / 1000);

        // Convert cJSON to string
        char *json_string = cJSON_PrintUnformatted(json);

        mqtt_publish(mqtt_client, json_string);

        // Cleanup
        free(json_string);
        cJSON_Delete(json);

        int wakeup_time_sec = CONFIG_WAKEUP_TIME_SEC;

        if (wakeup_time_sec < 5 || wakeup_time_sec > 86400) {
            ESP_LOGW(TAG, "Invalid WAKEUP_TIME_SEC: %d. Using default 300 seconds.", wakeup_time_sec);
            wakeup_time_sec = 300;
        }

        ESP_LOGI(TAG, "Enabling timer wakeup, %ds\n", wakeup_time_sec);
        ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));

        // enter deep sleep
        esp_deep_sleep_start();
    }
}