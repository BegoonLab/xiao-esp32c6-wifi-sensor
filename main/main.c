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

#include "sensor_nvs.h"
#include "sensor_gpio.h"
#include "esp_sleep.h"
#include "sys/time.h"

#if !defined CONFIG_SENSOR_ID
#error Define CONFIG_SENSOR_ID in idf.py menuconfig to compile source code.
#endif

#ifdef CONFIG_SENSOR_CONNECTION_WIFI_MQTT
#include "sensor_wifi.h"
#include "sensor_mqtt.h"
#endif

#ifdef CONFIG_SENSOR_CONNECTION_ZIGBEE
#include "sensor_zb.h"
#endif

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
    ESP_LOGI(TAG, "SW version: %s", GIT_COMMIT_HASH);
    deinit_adc();
#endif
    init_nvs();
    init_gpio();
    init_led();
#ifndef CONFIG_SENSOR_NO_SENSOR
    init_bme();
    read_bme(&temperature, &humidity, &pressure);
    deinit_bme();
#endif
    struct timeval start_to_connect;
    gettimeofday(&start_to_connect, NULL);

#ifdef CONFIG_SENSOR_CONNECTION_WIFI_MQTT
    init_wifi_sta();
    esp_mqtt_client_handle_t mqtt_client = init_mqtt_client();
#endif

#ifdef CONFIG_SENSOR_CONNECTION_ZIGBEE
    init_zb();

    start_zb();
#endif

    struct timeval end_to_connect;
    gettimeofday(&end_to_connect, NULL);

#ifdef CONFIG_SENSOR_CONNECTION_WIFI_MQTT
    int rssi = -100;
    esp_wifi_sta_get_rssi(&rssi);
    ESP_LOGI(TAG, "RSSI: %d", rssi);

    char *json_string = (char *)malloc(MQTT_MSG_MAX_LEN * sizeof(char));
    mqtt_prepare_json(json_string, rssi, battery_voltage, temperature, humidity, pressure, start_to_connect, end_to_connect);

    mqtt_publish(mqtt_client, json_string);

    // Cleanup
    free(json_string);
#endif

    int wakeup_time_sec = CONFIG_WAKEUP_TIME_SEC;

    if (wakeup_time_sec < 5 || wakeup_time_sec > 86400) {
        ESP_LOGW(TAG, "Invalid WAKEUP_TIME_SEC: %d. Using default 300 seconds.", wakeup_time_sec);
        wakeup_time_sec = 300;
    }

    ESP_LOGI(TAG, "Enabling timer wakeup, %ds\n", wakeup_time_sec);
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));

    // Enter deep sleep
    //esp_deep_sleep_start();
}