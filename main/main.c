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

#include "sdkconfig.h"
#include "sensor_gpio.h"
#include "sensor_i2c.h"
#include "sensor_id.h"
#include "sensor_led.h"
#include "sensor_nvs.h"
#include "sensor_sleep.h"

#if !defined(GIT_COMMIT_HASH)
#error "GIT_COMMIT_HASH is missing"
#endif

#if defined(CONFIG_SENSOR_BME280) && defined(CONFIG_SENSOR_BME680)
#error                                                                         \
    "Configuration error: Both BME280 and BME680 sensors cannot be connected simultaneously. Please enable only one of these sensors in the configuration."
#endif

#ifdef CONFIG_SENSOR_CONNECTION_WIFI_MQTT
#include "sensor_mqtt.h"
#endif

#ifdef CONFIG_SENSOR_CONNECTION_ZIGBEE
#include "sensor_zb.h"
#endif

#ifdef CONFIG_ENABLE_BATTERY_CHECK
#include "sensor_adc.h"
#endif

#if defined(CONFIG_SENSOR_BME280) || defined(CONFIG_SENSOR_BME680)
#include "sensor_bme.h"
#endif

#ifdef CONFIG_SENSOR_SGP41
#include "sensor_sgp.h"
#endif

static const char *TAG = "sensor_main";
char sensor_id[SENSOR_ID_MAX_LEN] = {0};
double battery_voltage = 0.0;
float temperature = 0;
float humidity = 0;
float pressure = 0;
uint16_t sraw_voc = 0;
uint16_t sraw_nox = 0;
int32_t voc_index_value = 0;
int32_t nox_index_value = 0;

void app_main(void) {
#ifdef CONFIG_ENABLE_BATTERY_CHECK
  init_adc();
  get_battery_voltage(&battery_voltage);
  ESP_LOGI(TAG, "Battery Voltage: %.2f V", battery_voltage);
  ESP_LOGI(TAG, "FW version: %s", GIT_COMMIT_HASH);
  deinit_adc();
#endif

  init_nvs();
  init_gpio();
  init_led();
  resolve_sensor_id();
  init_i2c();

#ifdef CONFIG_SENSOR_SGP41
  init_sgp();
  read_sgp(&sraw_voc, &sraw_nox, &voc_index_value, &nox_index_value);
  deinit_sgp();
#endif

#if defined(CONFIG_SENSOR_BME280) || defined(CONFIG_SENSOR_BME680)
  init_bme();
  read_bme(&temperature, &humidity, &pressure);
  deinit_bme();
#endif

#ifdef CONFIG_SENSOR_CONNECTION_WIFI_MQTT
  mqtt_send_sensor_data();
  deinit_i2c();
  go_sleep();
#endif

#ifdef CONFIG_SENSOR_CONNECTION_ZIGBEE
  init_zb();
  start_zb();
#endif
}