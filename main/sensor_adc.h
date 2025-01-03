/**
 * @file    sensor_adc.h
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    29 September 2024
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#pragma once
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "math.h"
#include "sensor_data.h"
#include "stdbool.h"

void init_adc(void);
void deinit_adc(void);
bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel,
                          adc_atten_t atten, adc_cali_handle_t *out_handle);
void adc_calibration_deinit(adc_cali_handle_t handle);
void check_battery(SensorData *sensor_data);
double calculate_battery_percentage(double voltage);