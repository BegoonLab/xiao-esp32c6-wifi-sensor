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
#include "sys/time.h"

#ifdef CONFIG_ENABLE_BATTERY_CHECK
#include "sensor_adc.h"
#endif

#ifndef CONFIG_SENSOR_NO_SENSOR
#include "sensor_bme.h"
#endif

static const char *TAG = "sensor_main";
double battery_voltage = 0.0;
float temperature = 0;
float humidity = 0;
float pressure = 0;

void app_main(void) {

#ifdef CONFIG_ENABLE_BATTERY_CHECK
    init_adc();
    get_battery_voltage(&battery_voltage);
    ESP_LOGI(TAG, "Battery Voltage: %.2f V", battery_voltage);
    deinit_adc();
#endif
    init_nvs();
    init_gpio();
#ifndef CONFIG_SENSOR_NO_SENSOR
    init_bme();
    read_bme(&temperature, &humidity, &pressure);
    deinit_bme();
#endif
    struct timeval start_to_connect;
    gettimeofday(&start_to_connect, NULL);

    init_wifi_sta();
    esp_mqtt_client_handle_t mqtt_client = init_mqtt_client();

    struct timeval end_to_connect;
    gettimeofday(&end_to_connect, NULL);

    int rssi = -100;
    esp_wifi_sta_get_rssi(&rssi);
    ESP_LOGI(TAG, "RSSI: %d", rssi);

    // Create cJSON object
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "ID", CONFIG_MQTT_SENSOR_ID);
    cJSON_AddNumberToObject(json, "RSSI", rssi);
    cJSON_AddNumberToObject(json, "battery_voltage", battery_voltage);

    char temperature_str[16];
    snprintf(temperature_str, sizeof(temperature_str), "%.2f", temperature);
    cJSON_AddStringToObject(json, "temperature", temperature_str);

    char humidity_str[16];
    snprintf(humidity_str, sizeof(humidity_str), "%.2f", humidity);
    cJSON_AddStringToObject(json, "humidity", humidity_str);

    char pressure_str[16];
    snprintf(pressure_str, sizeof(pressure_str), "%.2f", pressure);
    cJSON_AddStringToObject(json, "pressure", pressure_str);

    cJSON_AddNumberToObject(json, "connection_duration_ms",
                            (end_to_connect.tv_sec - start_to_connect.tv_sec) * 1000 +
                            (end_to_connect.tv_usec - start_to_connect.tv_usec) / 1000);

    // Convert cJSON to string
    char *json_string = cJSON_PrintUnformatted(json);

    while (true) {
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