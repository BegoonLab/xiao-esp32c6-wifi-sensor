/**
 * @file    sensor_bme.h
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    30 September 2024
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#pragma once

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_gpio.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "math.h"
#include "memory.h"
#include "sensor_i2c.h"

#include "bme280.h"
#include "bme68x.h"
#include "freertos/task.h"

#define SAMPLE_COUNT UINT8_C(5)

void init_bme(void);
void deinit_bme(void);
void read_bme(float *temperature, float *humidity, float *pressure);
void convert_values(float *temperature, float *humidity, float *pressure);
void bme280_init_after_sleep();

int8_t bme280_get_temperature(float *temperature);
int8_t bme280_get_humidity(float *humidity);
int8_t bme280_get_pressure(float *pressure);

/*!
 *  @brief Function for reading the sensor's registers through I2C bus.
 *
 * @param[in] reg_addr       : Register address from which data is read.
 * @param[out] reg_data      : Pointer to data buffer where read data is stored.
 * @param[in] length         : Number of bytes of data to be read.
 * @param[in, out] intf_ptr  : Void pointer that can enable the linking of
 * descriptors for interface related call backs.
 *
 *  @return Status of execution
 *
 *  @retval BME280_INTF_RET_SUCCESS -> Success.
 *  @retval != BME280_INTF_RET_SUCCESS -> Failure.
 *
 */
BME280_INTF_RET_TYPE bme280_i2c_read(uint8_t reg_addr, uint8_t *reg_data,
                                     uint32_t length, void *intf_ptr);

/*!
 *  @brief Function for writing the sensor's registers through I2C bus.
 *
 * @param[in] reg_addr      : Register address to which the data is written.
 * @param[in] reg_data      : Pointer to data buffer in which data to be written
 *                            is stored.
 * @param[in] length        : Number of bytes of data to be written.
 * @param[in, out] intf_ptr : Void pointer that can enable the linking of
 * descriptors for interface related call backs
 *
 *  @return Status of execution
 *
 *  @retval BME280_INTF_RET_SUCCESS -> Success.
 *  @retval != BME280_INTF_RET_SUCCESS -> Failure.
 *
 */
BME280_INTF_RET_TYPE bme280_i2c_write(uint8_t reg_addr, const uint8_t *reg_data,
                                      uint32_t length, void *intf_ptr);

/*!
 *  @brief This function provides the delay for required time (Microsecond) as
 * per the input provided in some of the APIs.
 *
 *  @param[in] period_us  : The required wait time in microsecond.
 *  @param[in] intf_ptr   : Interface pointer
 *
 *  @return void.
 */
void bme280_delay_us(uint32_t period_us, void *intf_ptr);

/*!
 *  @brief This API is used to print the execution status.
 *
 *  @param[in] api_name : Name of the API whose execution status has to be
 * printed.
 *  @param[in] rslt     : Error code returned by the API whose execution status
 * has to be printed.
 *
 *  @return void.
 */
void bme280_error_codes_print_result(const char api_name[], int8_t rslt);

/*!
 *  @brief Function for reading the sensor's registers through I2C bus.
 *
 *  @param[in] reg_addr     : Register address.
 *  @param[out] reg_data    : Pointer to the data buffer to store the read data.
 *  @param[in] len          : No of bytes to read.
 *  @param[in] intf_ptr     : Interface pointer
 *
 *  @return Status of execution
 *  @retval = BME68X_INTF_RET_SUCCESS -> Success
 *  @retval != BME68X_INTF_RET_SUCCESS  -> Failure Info
 *
 */
BME68X_INTF_RET_TYPE bme68x_i2c_read(uint8_t reg_addr, uint8_t *reg_data,
                                     uint32_t len, void *intf_ptr);

/*!
 *  @brief Function for writing the sensor's registers through I2C bus.
 *
 *  @param[in] reg_addr     : Register address.
 *  @param[in] reg_data     : Pointer to the data buffer whose value is to be
 * written.
 *  @param[in] len          : No of bytes to write.
 *  @param[in] intf_ptr     : Interface pointer
 *
 *  @return Status of execution
 *  @retval = BME68X_INTF_RET_SUCCESS -> Success
 *  @retval != BME68X_INTF_RET_SUCCESS  -> Failure Info
 *
 */
BME68X_INTF_RET_TYPE bme68x_i2c_write(uint8_t reg_addr, const uint8_t *reg_data,
                                      uint32_t len, void *intf_ptr);

/*!
 * @brief This function provides the delay for required time (Microsecond) as
 * per the input provided in some of the APIs.
 *
 *  @param[in] period       : The required wait time in microsecond.
 *  @param[in] intf_ptr     : Interface pointer
 *
 *  @return void.
 *
 */
void bme68x_delay_us(uint32_t period, void *intf_ptr);

/*!
 *  @brief Prints the execution status of the APIs.
 *
 *  @param[in] api_name : Name of the API whose execution status has to be
 * printed.
 *  @param[in] rslt     : Error code returned by the API whose execution status
 * has to be printed.
 *
 *  @return void.
 */
void bme68x_error_codes_print_result(const char api_name[], int8_t rslt);
