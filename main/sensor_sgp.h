/**
 * @file    sensor_sgp.h
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    20 November 2024
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#pragma once

#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "memory.h"
#include "sensirion_common.h"
#include "sensirion_gas_index_algorithm.h"
#include "sensirion_i2c_hal.h"
#include "sensor_data.h"
#include "sensor_mqtt.h"
#include "sgp41_i2c.h"
#ifdef CONFIG_ENABLE_BATTERY_CHECK
#include "sensor_adc.h"
#endif
#if defined(CONFIG_SENSOR_BME280) || defined(CONFIG_SENSOR_BME680)
#include "sensor_bme.h"
#else
#warning                                                                       \
    "A sensor for temperature and humidity compensation is not defined. Measurements may be unreliable."
#endif
#include "sensor_i2c.h"

#define HIGH 1
#define LOW 0

void init_sgp(void);

void read_sgp(SensorData *sensor_data);

void deinit_sgp(void);

void send_data_task(void *pvParameters);

void read_compensation_values(uint16_t *compensation_rh,
                              uint16_t *compensation_t,
                              SensorData *sensor_data);