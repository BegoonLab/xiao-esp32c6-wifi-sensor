/**
 * @file    main.c
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    27 September 2024
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#include "esp_mac.h"
#include "sdkconfig.h"
#include "sensor_gpio.h"
#include "sensor_id.h"
#include "sensor_led.h"
#include "sensor_nvs.h"
#include "sys/time.h"

#ifdef CONFIG_SENSOR_CONNECTION_WIFI_MQTT
#include "esp_sleep.h"
#include "sensor_mqtt.h"
#include "sensor_wifi.h"
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

#include <stdio.h>
#include <string.h>

static const char *TAG = "sensor_main";
char sensor_id[SENSOR_ID_MAX_LEN] = {0};
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
  // Resolve SENSOR_ID
  if (strlen(CONFIG_SENSOR_ID) == 0) {
    uint8_t mac[6];
    if (esp_base_mac_addr_get(mac) != ESP_OK) {
      ESP_LOGE(TAG, "Failed to get base MAC address");
      // Handle error accordingly, possibly using a fallback ID
      strlcpy(sensor_id, "0x000000000000", SENSOR_ID_MAX_LEN);
    } else {
      // Use snprintf_s if available, otherwise ensure snprintf usage is secure
      int ret =
          snprintf(sensor_id, SENSOR_ID_MAX_LEN, "0x%02X%02X%02X%02X%02X%02X",
                   mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      if (ret < 0 || ret >= SENSOR_ID_MAX_LEN) {
        ESP_LOGE(TAG, "Error or truncation occurred while setting SENSOR_ID");
        strlcpy(sensor_id, "0x000000000000", SENSOR_ID_MAX_LEN);
      } else {
        ESP_LOGI(TAG, "Using MAC Address as SENSOR_ID: %s", sensor_id);
      }
    }
  } else {
    strlcpy(sensor_id, CONFIG_SENSOR_ID, SENSOR_ID_MAX_LEN);
    ESP_LOGI(TAG, "Using Configured SENSOR_ID: %s", sensor_id);
  }

#ifdef CONFIG_SENSOR_CONNECTION_WIFI_MQTT
  struct timeval start_to_connect;
  gettimeofday(&start_to_connect, NULL);
  init_wifi_sta();
  esp_mqtt_client_handle_t mqtt_client = init_mqtt_client();
#endif

#ifdef CONFIG_SENSOR_CONNECTION_ZIGBEE
  init_zb();
  start_zb();
#endif

#ifdef CONFIG_SENSOR_CONNECTION_WIFI_MQTT
  struct timeval end_to_connect;
  gettimeofday(&end_to_connect, NULL);

  int rssi = -100;
  esp_wifi_sta_get_rssi(&rssi);
  ESP_LOGI(TAG, "RSSI: %d", rssi);

  char *json_string = (char *)malloc(MQTT_MSG_MAX_LEN * sizeof(char));
  if (json_string == NULL) {
    ESP_LOGE(TAG, "Failed to allocate memory for JSON string");
    // Handle error accordingly
  } else {
    mqtt_prepare_json(json_string, rssi, battery_voltage, temperature, humidity,
                      pressure, start_to_connect, end_to_connect);

    mqtt_publish(mqtt_client, json_string);

    // Cleanup
    free(json_string);
  }

  int wakeup_time_sec = CONFIG_WAKEUP_TIME_SEC;

  if (wakeup_time_sec < 5 || wakeup_time_sec > 86400) {
    ESP_LOGW(TAG, "Invalid WAKEUP_TIME_SEC: %d. Using default 300 seconds.",
             wakeup_time_sec);
    wakeup_time_sec = 300;
  }

  ESP_LOGI(TAG, "Enabling timer wakeup, %ds\n", wakeup_time_sec);
  ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));

  // Enter deep sleep
  esp_deep_sleep_start();
#endif
}