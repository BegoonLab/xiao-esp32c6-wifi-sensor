/**
 * @file    sensor_bme.h
 * @author  Alexander Begoon (<a href="mailto:alex\@begoonlab.tech">alex\@begoonlab.tech</a>)
 * @date    30 September 2024
 * @brief   //TODO
 * 
 * @details //TODO
 * 
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#pragma once

#ifndef CONFIG_SENSOR_NO_SENSOR
#include "esp_rom_gpio.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "memory.h"
#include "math.h"
#endif

#ifdef CONFIG_SENSOR_BME680
#include "bme680.h"
#endif

#ifdef CONFIG_SENSOR_BME280
#include "bmp280.h"
#endif

void init_bme(void);
void deinit_bme(void);
void read_bme(float *temperature, float *humidity, float *pressure);
void convert_values(float *temperature, float *humidity, float *pressure);