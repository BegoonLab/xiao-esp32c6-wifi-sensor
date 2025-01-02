/**
 * @file    sensor_sgp.c
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    20 November 2024
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#include "sensor_sgp.h"

#ifdef CONFIG_SENSOR_SGP41

static const char *TAG = "sensor_sgp41";

extern SensorData sensor_data;

GasIndexAlgorithmParams voc_params;
GasIndexAlgorithmParams nox_params;

uint16_t DEFAULT_COMPENSATION_RH = 0x8000; // in ticks as defined by SGP41
uint16_t DEFAULT_COMPENSATION_T = 0x6666;  // in ticks as defined by SGP41

TaskHandle_t send_data_task_handle = NULL;

static i2c_master_dev_handle_t sgp41_i2c_handle;

const float sampling_interval = 10.f;
int32_t send_data_after = CONFIG_WAKEUP_TIME_SEC + 60;
volatile bool can_go_sleep = true;

void init_sgp(void) {
  GasIndexAlgorithm_init_with_sampling_interval(
      &voc_params, GasIndexAlgorithm_ALGORITHM_TYPE_VOC, sampling_interval);
  GasIndexAlgorithm_init_with_sampling_interval(
      &nox_params, GasIndexAlgorithm_ALGORITHM_TYPE_NOX, sampling_interval);

  sensirion_i2c_hal_init();

#if defined(CONFIG_SENSOR_BME280) || defined(CONFIG_SENSOR_BME680)
  init_bme();
#endif

  xTaskCreate(send_data_task, "Send_Data_Task", 8192, NULL, 5,
              &send_data_task_handle);
}

void read_sgp() {

  int16_t error = 0;
  uint16_t serial_number[3];
  uint16_t compensation_rh = DEFAULT_COMPENSATION_RH;
  uint16_t compensation_t = DEFAULT_COMPENSATION_T;

  error = sgp41_get_serial_number(serial_number);
  if (error) {
    ESP_LOGE(TAG, "Error executing sgp41_get_serial_number(): %i", error);
  } else {
    ESP_LOGI(TAG, "serial: 0x%04x%04x%04x", serial_number[0], serial_number[1],
             serial_number[2]);
  }

  uint16_t test_result = 0;

  error = sgp41_execute_self_test(&test_result);
  if (error) {
    ESP_LOGE(TAG, "Error executing sgp41_execute_self_test(): %i", error);
  } else {
    ESP_LOGI(TAG, "Self Test: 0x%x", test_result);
  }

  uint16_t nox_conditioning_s = 8;

  while (nox_conditioning_s > 0) {
    sensirion_i2c_hal_sleep_usec(1000000);

    error = sgp41_execute_conditioning(
        DEFAULT_COMPENSATION_RH, DEFAULT_COMPENSATION_T, &sensor_data.sraw_voc);
    nox_conditioning_s--;
    ESP_LOGI(TAG, "NOx conditioning remaining time: %i s", nox_conditioning_s);

    if (error) {
      ESP_LOGE(TAG, "Error executing sgp41_execute_conditioning(): %i", error);
    }
  }

  while (1) {
    send_data_after -= sampling_interval;

    if (send_data_after < 0 && can_go_sleep == true) {
      can_go_sleep = false;
      xTaskNotify(send_data_task_handle, 1, eSetValueWithoutOverwrite);
    }

    sensirion_i2c_hal_sleep_usec(((uint32_t)(sampling_interval * 1000) - 240) *
                                 1000);

    // Measure RH and T signals and convert to SGP40 ticks
    read_compensation_values(&compensation_rh, &compensation_t);

    // Request a first measurement to turn the heater on
    if (xSemaphoreTake(sensor_data.mutex, portMAX_DELAY) == pdTRUE) {
      error = sgp41_measure_raw_signals(compensation_rh, compensation_t,
                                        &sensor_data.sraw_voc,
                                        &sensor_data.sraw_nox);
      xSemaphoreGive(sensor_data.mutex);
    }

    if (error) {
      ESP_LOGE(TAG, "Error executing sgp41_measure_raw_signals(): %i", error);
      continue;
    }

    // Wait for the heater to reach temperature
    sensirion_i2c_hal_sleep_usec(170000);

    // Request the actual measurement
    if (xSemaphoreTake(sensor_data.mutex, portMAX_DELAY) == pdTRUE) {
      error = sgp41_measure_raw_signals(compensation_rh, compensation_t,
                                        &sensor_data.sraw_voc,
                                        &sensor_data.sraw_nox);
      xSemaphoreGive(sensor_data.mutex);
    }

    if (error) {
      ESP_LOGE(TAG, "Error executing sgp41_measure_raw_signals(): %i", error);
      continue;
    }

    // Turn the heater off
    error = sgp41_turn_heater_off();

    if (error) {
      ESP_LOGE(TAG, "Error executing sgp41_turn_heater_off(): %i", error);
      continue;
    }

    // Process raw signals by Gas Index Algorithm
    // to get the VOC and NOx index values
    if (xSemaphoreTake(sensor_data.mutex, portMAX_DELAY) == pdTRUE) {
      GasIndexAlgorithm_process(&voc_params, sensor_data.sraw_voc,
                                &sensor_data.voc_index_value);
      GasIndexAlgorithm_process(&nox_params, sensor_data.sraw_nox,
                                &sensor_data.nox_index_value);
      ESP_LOGI(TAG, "VOC Raw: %i\tVOC Index: %li\tNOx Raw: %i\tNOx Index: %li",
               sensor_data.sraw_voc, sensor_data.voc_index_value,
               sensor_data.sraw_nox, sensor_data.nox_index_value);
      xSemaphoreGive(sensor_data.mutex);
    }
  }
}

void deinit_sgp(void) {
#if defined(CONFIG_SENSOR_BME280) || defined(CONFIG_SENSOR_BME680)
  deinit_bme();
#endif
  sensirion_i2c_hal_free();
}

void sensirion_i2c_hal_init(void) {
  sgp41_i2c_handle = get_i2c_device(CONFIG_SGP41_I2C_ADDR);
}

void sensirion_i2c_hal_free(void) { release_i2c_device(sgp41_i2c_handle); }

int8_t sensirion_i2c_hal_read(uint8_t address, uint8_t *data, uint16_t count) {
  esp_err_t err = ESP_FAIL;
  static uint8_t attempt = 0;
  do {
    err = i2c_master_receive(sgp41_i2c_handle, data, count, -1);
    attempt++;
  } while (err != ESP_OK && attempt < 5);

  ESP_ERROR_CHECK(err);

  return (int8_t)err;
}

int8_t sensirion_i2c_hal_write(uint8_t address, const uint8_t *data,
                               uint16_t count) {
  esp_err_t err = ESP_FAIL;
  static uint8_t attempt = 0;

  do {
    err = i2c_master_transmit(sgp41_i2c_handle, data, count, -1);
    attempt++;
  } while (err != ESP_OK && attempt < 5);

  ESP_ERROR_CHECK(err);

  return (int8_t)err;
}

void sensirion_i2c_hal_sleep_usec(uint32_t useconds) {
  if (can_go_sleep && useconds > 100000) {
    sensirion_i2c_hal_free();
    esp_sleep_enable_timer_wakeup(useconds);
    esp_light_sleep_start();
    sensirion_i2c_hal_init();
  } else {
    uint32_t msec = useconds / 1000;
    msec += 10;
    vTaskDelay(pdMS_TO_TICKS(msec));
  }
}

// Task to handle data sending
void send_data_task(void *pvParameters) {
  uint32_t notification = 0;
  while (1) {
    // Wait indefinitely for a notification to send data
    if (xTaskNotifyWait(0, ULONG_MAX, &notification,
                        pdMS_TO_TICKS(CONFIG_WAKEUP_TIME_SEC * 1000)) ==
        pdTRUE) {
      if (notification == 1) { // Command to send data
#ifdef CONFIG_ENABLE_BATTERY_CHECK
        init_adc();
        check_battery(&sensor_data);
        deinit_adc();
#endif
        mqtt_send_sensor_data();
        can_go_sleep = true;
        send_data_after = CONFIG_WAKEUP_TIME_SEC;
      }
    }
  }
}

/*
 * Get the humidity and temperature values to use for compensating SGP41
 * measurement values. The returned humidity and temperature is in ticks
 * as defined by SGP41 interface.
 *
 * @param compensation_rh: out variable for humidity
 * @param compensation_t: out variable for temperature
 */
void read_compensation_values(uint16_t *compensation_rh,
                              uint16_t *compensation_t) {

  *compensation_rh = DEFAULT_COMPENSATION_RH;
  *compensation_t = DEFAULT_COMPENSATION_T;

#if defined(CONFIG_SENSOR_BME280) || defined(CONFIG_SENSOR_BME680)

  read_bme(&sensor_data);

  if (xSemaphoreTake(sensor_data.mutex, portMAX_DELAY) == pdTRUE) {
    ESP_LOGI(TAG, "T: %.2f\tRH: %.2f", sensor_data.temperature,
             sensor_data.humidity);

    // convert temperature and humidity to ticks as defined by SGP40
    // interface
    // NOTE: in case you read RH and T raw signals check out the
    // ticks specification in the datasheet, as they can be different for
    // different sensors
    *compensation_rh = (uint16_t)sensor_data.humidity * 65535 / 100;
    *compensation_t = (uint16_t)(sensor_data.temperature + 45) * 65535 / 175;

    xSemaphoreGive(sensor_data.mutex);
  }

#endif
}
#endif