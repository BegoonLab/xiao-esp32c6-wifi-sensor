/**
 * @file    sensor_bme.c
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    30 September 2024
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#include "sensor_bme.h"

#define HIGH 1
#define LOW 0

#ifdef CONFIG_SENSOR_BME680
static const char *TAG = "sensor_bme680";

static bme680_t sensor;

bme680_values_float_t values;
#endif

#ifdef CONFIG_SENSOR_BME280
static const char *TAG = "sensor_bme280";

static bmp280_t sensor;

static bmp280_params_t params;
#endif

void init_bme(void) {
#ifdef CONFIG_SENSOR_BME680
  esp_rom_gpio_pad_select_gpio(CONFIG_BME680_POWER_PIN);
  gpio_reset_pin(CONFIG_BME680_POWER_PIN);
  gpio_set_direction(CONFIG_BME680_POWER_PIN, GPIO_MODE_OUTPUT);

  gpio_set_level(CONFIG_BME680_POWER_PIN, HIGH);
  ESP_LOGI(TAG, "BME680 powered on");

  vTaskDelay(pdMS_TO_TICKS(25));

  ESP_ERROR_CHECK(i2cdev_init());

  memset(&sensor, 0, sizeof(sensor));
  ESP_ERROR_CHECK(
      bme680_init_desc(&sensor, CONFIG_BME680_I2C_ADDR, CONFIG_BME680_I2C_PORT,
                       CONFIG_BME680_I2C_SDA, CONFIG_BME680_I2C_SCL));
  ESP_ERROR_CHECK(bme680_init_sensor(&sensor));
  ESP_ERROR_CHECK(bme680_use_heater_profile(&sensor, BME680_HEATER_NOT_USED));
  ESP_ERROR_CHECK(bme680_set_oversampling_rates(
      &sensor, BME680_OSR_16X, BME680_OSR_16X, BME680_OSR_16X));
  ESP_ERROR_CHECK(bme680_set_filter_size(&sensor, BME680_IIR_SIZE_127));
#endif

#ifdef CONFIG_SENSOR_BME280
  esp_rom_gpio_pad_select_gpio(CONFIG_BME280_POWER_PIN);
  gpio_reset_pin(CONFIG_BME280_POWER_PIN);
  gpio_set_direction(CONFIG_BME280_POWER_PIN, GPIO_MODE_OUTPUT);

  gpio_set_level(CONFIG_BME280_POWER_PIN, HIGH);
  vTaskDelay(pdMS_TO_TICKS(100));
  gpio_set_level(CONFIG_BME280_POWER_PIN, LOW);
  vTaskDelay(pdMS_TO_TICKS(100));
  gpio_set_level(CONFIG_BME280_POWER_PIN, HIGH);
  ESP_LOGI(TAG, "BME280 powered on");

  vTaskDelay(pdMS_TO_TICKS(25));

  ESP_ERROR_CHECK(i2cdev_init());

  memset(&sensor, 0, sizeof(sensor));
  ESP_ERROR_CHECK(
      bmp280_init_desc(&sensor, CONFIG_BME280_I2C_ADDR, CONFIG_BME280_I2C_PORT,
                       CONFIG_BME280_I2C_SDA, CONFIG_BME280_I2C_SCL));

  bmp280_init_default_params(&params);
  ESP_ERROR_CHECK(bmp280_init(&sensor, &params));
#endif
}

void read_bme(float *temperature, float *humidity, float *pressure) {
#ifdef CONFIG_SENSOR_BME680
  if (bme680_force_measurement(&sensor) == ESP_OK) {
    bool busy;
    do {
      ESP_ERROR_CHECK(bme680_is_measuring(&sensor, &busy));
    } while (busy);
    if (bme680_get_results_float(&sensor, &values) == ESP_OK) {
      ESP_LOGE(TAG, "Unable to read from BME680");
    }
  } else {
    ESP_LOGE(TAG, "Unable to read from BME680");
  }

  *temperature = values.temperature;
  *humidity = values.humidity;
  *pressure = values.pressure;
#endif

#ifdef CONFIG_SENSOR_BME280
  if (bmp280_force_measurement(&sensor) == ESP_OK) {
    bool busy;
    do {
      ESP_ERROR_CHECK(bmp280_is_measuring(&sensor, &busy));
    } while (busy);
    if (bmp280_read_float(&sensor, temperature, pressure, humidity) != ESP_OK) {
      ESP_LOGE(TAG, "Unable to read from BME280");
    }
  } else {
    ESP_LOGE(TAG, "Unable to read from BME280");
  }
#endif

#ifndef CONFIG_SENSOR_NO_SENSOR
  convert_values(temperature, humidity, pressure);
#endif
}

void deinit_bme(void) {
#ifndef CONFIG_SENSOR_NO_SENSOR
  ESP_ERROR_CHECK(i2cdev_done());
#endif

#ifdef CONFIG_SENSOR_BME680
  gpio_set_level(CONFIG_BME680_POWER_PIN, LOW);
#endif

#ifdef CONFIG_SENSOR_BME280
  gpio_set_level(CONFIG_BME280_POWER_PIN, LOW);
#endif
}

void convert_values(float *temperature, float *humidity, float *pressure) {
  // Round temperature and humidity to two decimal places
  *temperature = roundf(*temperature * 100.0f) / 100.0f;
  *humidity = roundf(*humidity * 100.0f) / 100.0f;

  // Convert pressure from Pa to hPa and round to two decimal places
  *pressure = roundf((*pressure / 100.0f) * 100.0f) / 100.0f;
}