/**
 * @file    sensor_data.h
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    28 December 2024
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#pragma once

#include "esp_log.h"
#include "freertos/FreeRTOS.h"

typedef struct {
  double voltage;          // Voltage in volts
  double remaining_charge; // Remaining charge percentage
} Battery;

typedef struct {
  SemaphoreHandle_t mutex;
  Battery battery;
  double temperature;
  double humidity;
  double pressure;
  uint16_t sraw_voc;
  uint16_t sraw_nox;
  int32_t voc_index_value;
  int32_t nox_index_value;
} SensorData;

void init_sensor_data(SensorData *data);
void deinit_sensor_data(SensorData *data);