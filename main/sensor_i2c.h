/**
 * @file    sensor_i2c.h
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    15 December 2024
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#pragma once

#include "driver/i2c_master.h"

#ifdef CONFIG_SENSOR_BME680
#ifndef MASTER_I2C_PORT
#define MASTER_I2C_PORT CONFIG_BME680_I2C_PORT
#define MASTER_I2C_SCL CONFIG_BME680_I2C_SCL
#define MASTER_I2C_SDA CONFIG_BME680_I2C_SDA
#endif
#endif

#ifdef CONFIG_SENSOR_BME280
#ifndef MASTER_I2C_PORT
#define MASTER_I2C_PORT CONFIG_BME280_I2C_PORT
#define MASTER_I2C_SCL CONFIG_BME280_I2C_SCL
#define MASTER_I2C_SDA CONFIG_BME280_I2C_SDA
#endif
#endif

#ifdef CONFIG_SENSOR_SGP41
#ifndef MASTER_I2C_PORT
#define MASTER_I2C_PORT CONFIG_SGP41_I2C_PORT
#define MASTER_I2C_SCL CONFIG_SGP41_I2C_SCL
#define MASTER_I2C_SDA CONFIG_SGP41_I2C_SDA
#endif
#endif

#ifndef I2C_FREQ_HZ
#define I2C_FREQ_HZ 100000
#endif

void init_i2c(void);
void deinit_i2c(void);
i2c_master_dev_handle_t get_i2c_device(uint8_t device_address);
void release_i2c_device(i2c_master_dev_handle_t dev_handle);