/**
 * @file    sensor_data.c
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    28 December 2024
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#include "sensor_data.h"

static const char *TAG = "sensor_data";

void init_sensor_data(SensorData *data) {
  data->mutex = xSemaphoreCreateMutex();
  if (data->mutex == NULL) {
    ESP_LOGE(TAG, "Failed to create mutex");
  }

  data->battery.voltage = 0;
  data->battery.remaining_charge = 0;
  data->temperature = 0;
  data->humidity = 0;
  data->pressure = 0;
  data->sraw_voc = 0;
  data->sraw_nox = 0;
  data->voc_index_value = 0;
  data->nox_index_value = 0;
}

void deinit_sensor_data(SensorData *data) {
  if (data->mutex != NULL) {
    vSemaphoreDelete(data->mutex);
    data->mutex = NULL;
  }
}