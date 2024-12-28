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

static struct bme68x_dev dev;
static struct bme68x_conf conf;
static struct bme68x_heatr_conf heatr_conf;
static struct bme68x_data data;
static uint32_t del_period;
static uint8_t n_fields;
static uint16_t sample_count = 1;

static i2c_master_dev_handle_t bme68x_i2c_handle;
#endif

#ifdef CONFIG_SENSOR_BME280
static const char *TAG = "sensor_bme280";

static uint32_t period;
static struct bme280_dev dev;
static struct bme280_settings settings;

static i2c_master_dev_handle_t bme280_i2c_handle;
#endif

void bme280_init_after_sleep() {
  int8_t rslt = bme280_init(&dev);
  bme280_error_codes_print_result("bme280_init", rslt);

  rslt = bme280_get_sensor_settings(&settings, &dev);
  bme280_error_codes_print_result("bme280_get_sensor_settings", rslt);

  /* Configuring the over-sampling rate, filter coefficient and standby time */
  /* Overwrite the desired settings */
  settings.filter = BME280_FILTER_COEFF_2;

  /* Over-sampling rate for humidity, temperature and pressure */
  settings.osr_h = BME280_OVERSAMPLING_1X;
  settings.osr_p = BME280_OVERSAMPLING_1X;
  settings.osr_t = BME280_OVERSAMPLING_1X;

  /* Setting the standby time */
  settings.standby_time = BME280_STANDBY_TIME_0_5_MS;

  rslt = bme280_set_sensor_settings(BME280_SEL_ALL_SETTINGS, &settings, &dev);
  bme280_error_codes_print_result("bme280_set_sensor_settings", rslt);
}

void init_bme(void) {

#ifdef CONFIG_SENSOR_BME680
  esp_rom_gpio_pad_select_gpio(CONFIG_BME680_POWER_PIN);
  gpio_reset_pin(CONFIG_BME680_POWER_PIN);
  gpio_set_direction(CONFIG_BME680_POWER_PIN, GPIO_MODE_OUTPUT);

  gpio_set_level(CONFIG_BME680_POWER_PIN, HIGH);
  ESP_LOGI(TAG, "BME680 powered on");

  vTaskDelay(pdMS_TO_TICKS(25));

  bme68x_i2c_handle = get_i2c_device(CONFIG_BME680_I2C_ADDR);

  dev.read = bme68x_i2c_read;
  dev.write = bme68x_i2c_write;
  dev.delay_us = bme68x_delay_us;
  dev.intf = BME68X_I2C_INTF;

  int8_t rslt = bme68x_init(&dev);
  bme68x_error_codes_print_result("bme68x_init", rslt);

  conf.filter = BME68X_FILTER_OFF;
  conf.odr = BME68X_ODR_NONE;
  conf.os_hum = BME68X_OS_16X;
  conf.os_pres = BME68X_OS_1X;
  conf.os_temp = BME68X_OS_2X;
  rslt = bme68x_set_conf(&conf, &dev);
  bme68x_error_codes_print_result("bme68x_set_conf", rslt);

  heatr_conf.enable = BME68X_ENABLE;
  heatr_conf.heatr_temp = 300;
  heatr_conf.heatr_dur = 100;
  rslt = bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heatr_conf, &dev);
  bme68x_error_codes_print_result("bme68x_set_heatr_conf", rslt);
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

  bme280_i2c_handle = get_i2c_device(CONFIG_BME280_I2C_ADDR);

  dev.read = bme280_i2c_read;
  dev.write = bme280_i2c_write;
  dev.delay_us = bme280_delay_us;
  dev.intf = BME280_I2C_INTF;

  int8_t rslt = 0;
  bme280_init_after_sleep();

  /* Always set the power mode after setting the configuration */
  rslt = bme280_set_sensor_mode(BME280_POWERMODE_NORMAL, &dev);
  bme280_error_codes_print_result("bme280_set_power_mode", rslt);

  /* Calculate measurement time in microseconds */
  rslt = bme280_cal_meas_delay(&period, &settings);
  bme280_error_codes_print_result("bme280_cal_meas_delay", rslt);

  ESP_LOGI(TAG, "Measurement time : %lu us", (long unsigned int)period);
#endif
}

void read_bme(SensorData *sensor_data) {
  if (xSemaphoreTake(sensor_data->mutex, portMAX_DELAY) == pdTRUE) {
#ifdef CONFIG_SENSOR_BME680
    int8_t rslt = 0;
    while (sample_count <= SAMPLE_COUNT) {
      rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, &dev);
      bme68x_error_codes_print_result("bme68x_set_op_mode", rslt);

      /* Calculate delay period in microseconds */
      del_period = bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, &dev) +
                   (heatr_conf.heatr_dur * 1000);
      dev.delay_us(del_period, dev.intf_ptr);

      /* Check if rslt == BME68X_OK, report or handle if otherwise */
      rslt = bme68x_get_data(BME68X_FORCED_MODE, &data, &n_fields, &dev);
      bme68x_error_codes_print_result("bme68x_get_data", rslt);

      if (n_fields) {
        sensor_data->temperature = data.temperature;
        sensor_data->pressure = data.pressure;
        sensor_data->humidity = data.humidity;
        sample_count++;
      }
    }
#endif

#ifdef CONFIG_SENSOR_BME280
    bme280_init_after_sleep();

    int8_t rslt = bme280_set_sensor_mode(BME280_POWERMODE_NORMAL, &dev);
    bme280_error_codes_print_result("bme280_set_sensor_mode", rslt);

    rslt = bme280_get_temperature(&sensor_data->temperature);
    bme280_error_codes_print_result("bme280_get_temperature", rslt);

    rslt = bme280_get_humidity(&sensor_data->humidity);
    bme280_error_codes_print_result("bme280_get_humidity", rslt);

    rslt = bme280_get_pressure(&sensor_data->pressure);
    bme280_error_codes_print_result("bme280_get_pressure", rslt);

    bme280_set_sensor_mode(BME280_POWERMODE_SLEEP, &dev);
    bme280_error_codes_print_result("bme280_set_sensor_mode", rslt);
#endif

#if defined(CONFIG_SENSOR_BME280) || defined(CONFIG_SENSOR_BME680)
    convert_values(&sensor_data->temperature, &sensor_data->humidity,
                   &sensor_data->pressure);
#endif
    xSemaphoreGive(sensor_data->mutex);
  }
}

void deinit_bme(void) {
#ifdef CONFIG_SENSOR_BME680
  release_i2c_device(bme68x_i2c_handle);
  gpio_set_level(CONFIG_BME680_POWER_PIN, LOW);
#endif

#ifdef CONFIG_SENSOR_BME280
  release_i2c_device(bme280_i2c_handle);
  gpio_set_level(CONFIG_BME280_POWER_PIN, LOW);
#endif
}

void convert_values(double *temperature, double *humidity, double *pressure) {
  // Round temperature and humidity to two decimal places
  *temperature = round(*temperature * 100.0) / 100.0;
  *humidity = round(*humidity * 100.0) / 100.0;

  // Convert pressure from Pa to hPa and round to two decimal places
  *pressure = round((*pressure / 100.0) * 100.0) / 100.0;
}

#ifdef CONFIG_SENSOR_BME280

/*!
 *  @brief Prints the execution status of the APIs.
 */
void bme280_error_codes_print_result(const char api_name[], int8_t rslt) {
  if (rslt != BME280_OK) {
    ESP_LOGE(TAG, "%s\t", api_name);

    switch (rslt) {
    case BME280_E_NULL_PTR:
      ESP_LOGE(TAG, "Error [%d] : Null pointer error.", rslt);
      ESP_LOGE(TAG,
               "It occurs when the user tries to assign value (not address) to "
               "a pointer, which has been initialized to NULL.");
      break;

    case BME280_E_COMM_FAIL:
      ESP_LOGE(TAG, "Error [%d] : Communication failure error.", rslt);
      ESP_LOGE(TAG, "It occurs due to read/write operation failure and also "
                    "due to power failure during communication");
      break;

    case BME280_E_DEV_NOT_FOUND:
      ESP_LOGE(TAG,
               "Error [%d] : Device not found error. It occurs when the device "
               "chip id is incorrectly read",
               rslt);
      break;

    case BME280_E_INVALID_LEN:
      ESP_LOGE(TAG,
               "Error [%d] : Invalid length error. It occurs when write is "
               "done with invalid length",
               rslt);
      break;

    default:
      ESP_LOGE(TAG, "Error [%d] : Unknown error code", rslt);
      break;
    }
  }
}

/*!
 *  @brief This internal API is used to get compensated temperature data.
 */
int8_t bme280_get_temperature(double *temperature) {
  int8_t rslt = BME280_E_NULL_PTR;
  int8_t idx = 0;
  uint8_t status_reg = 0;
  struct bme280_data comp_data;

  while (idx < SAMPLE_COUNT) {
    rslt = bme280_get_regs(BME280_REG_STATUS, &status_reg, 1, &dev);
    bme280_error_codes_print_result("bme280_get_regs", rslt);

    if (status_reg & BME280_STATUS_MEAS_DONE) {
      /* Measurement time delay given to read sample */
      dev.delay_us(period, dev.intf_ptr);

      /* Read compensated data */
      rslt = bme280_get_sensor_data(BME280_TEMP, &comp_data, &dev);
      bme280_error_codes_print_result("bme280_get_sensor_data", rslt);

      *temperature = comp_data.temperature;
      idx++;
    }
  }

  return rslt;
}

/*!
 *  @brief This internal API is used to get compensated humidity data.
 */
int8_t bme280_get_humidity(double *humidity) {
  int8_t rslt = BME280_E_NULL_PTR;
  int8_t idx = 0;
  uint8_t status_reg = 0;
  struct bme280_data comp_data;

  while (idx < SAMPLE_COUNT) {
    rslt = bme280_get_regs(BME280_REG_STATUS, &status_reg, 1, &dev);
    bme280_error_codes_print_result("bme280_get_regs", rslt);

    if (status_reg & BME280_STATUS_MEAS_DONE) {
      /* Measurement time delay given to read sample */
      dev.delay_us(period, dev.intf_ptr);

      /* Read compensated data */
      rslt = bme280_get_sensor_data(BME280_HUM, &comp_data, &dev);
      bme280_error_codes_print_result("bme280_get_sensor_data", rslt);

      *humidity = comp_data.humidity;
      idx++;
    }
  }

  return rslt;
}

/*!
 *  @brief This internal API is used to get compensated pressure data.
 */
int8_t bme280_get_pressure(double *pressure) {
  int8_t rslt = BME280_E_NULL_PTR;
  int8_t idx = 0;
  uint8_t status_reg = 0;
  struct bme280_data comp_data;

  while (idx < SAMPLE_COUNT) {
    rslt = bme280_get_regs(BME280_REG_STATUS, &status_reg, 1, &dev);
    bme280_error_codes_print_result("bme280_get_regs", rslt);

    if (status_reg & BME280_STATUS_MEAS_DONE) {
      /* Measurement time delay given to read sample */
      dev.delay_us(period, dev.intf_ptr);

      /* Read compensated data */
      rslt = bme280_get_sensor_data(BME280_PRESS, &comp_data, &dev);
      bme280_error_codes_print_result("bme280_get_sensor_data", rslt);

      *pressure = comp_data.pressure;

      idx++;
    }
  }

  return rslt;
}

BME280_INTF_RET_TYPE bme280_i2c_read(uint8_t reg_addr, uint8_t *reg_data,
                                     uint32_t length, void *intf_ptr) {
  esp_err_t err = ESP_FAIL;

  if (reg_addr) {
    err = i2c_master_transmit_receive(bme280_i2c_handle, &reg_addr, 1, reg_data,
                                      length, -1);
  } else {
    err = i2c_master_transmit(bme280_i2c_handle, reg_data, length, -1);
  }

  ESP_ERROR_CHECK(err);

  return (BME280_INTF_RET_TYPE)err;
}

BME280_INTF_RET_TYPE bme280_i2c_write(uint8_t reg_addr, const uint8_t *reg_data,
                                      uint32_t length, void *intf_ptr) {
  esp_err_t err = ESP_FAIL;

  if (reg_addr) {
    uint8_t *data_addr = malloc(length + 1);
    if (data_addr == NULL) {
      ESP_LOGE(TAG, "data_addr memory alloc fail");
      return (BME280_INTF_RET_TYPE)ESP_ERR_NO_MEM;
    }
    data_addr[0] = reg_addr;
    for (int i = 0; i < length; i++) {
      data_addr[i + 1] = reg_data[i];
    }
    err = i2c_master_transmit(bme280_i2c_handle, data_addr, length + 1, -1);
    free(data_addr);
  } else {
    err = i2c_master_transmit(bme280_i2c_handle, reg_data, length, -1);
  }

  ESP_ERROR_CHECK(err);

  return (BME280_INTF_RET_TYPE)err;
}

void bme280_delay_us(uint32_t period_us, void *intf_ptr) {
  uint32_t msec = period_us / 1000;
  if (period_us % 1000 > 0) {
    msec++;
  }
  vTaskDelay(pdMS_TO_TICKS(msec));
}
#endif

#ifdef CONFIG_SENSOR_BME680
BME68X_INTF_RET_TYPE bme68x_i2c_read(uint8_t reg_addr, uint8_t *reg_data,
                                     uint32_t len, void *intf_ptr) {
  esp_err_t err = ESP_FAIL;

  if (reg_addr) {
    err = i2c_master_transmit_receive(bme68x_i2c_handle, &reg_addr, 1, reg_data,
                                      len, -1);
  } else {
    err = i2c_master_transmit(bme68x_i2c_handle, reg_data, len, -1);
  }

  ESP_ERROR_CHECK(err);

  return (BME68X_INTF_RET_TYPE)err;
}

BME68X_INTF_RET_TYPE bme68x_i2c_write(uint8_t reg_addr, const uint8_t *reg_data,
                                      uint32_t len, void *intf_ptr) {
  esp_err_t err = ESP_FAIL;

  if (reg_addr) {
    uint8_t *data_addr = malloc(len + 1);
    if (data_addr == NULL) {
      ESP_LOGE(TAG, "data_addr memory alloc fail");
      return (BME68X_INTF_RET_TYPE)ESP_ERR_NO_MEM;
    }
    data_addr[0] = reg_addr;
    for (int i = 0; i < len; i++) {
      data_addr[i + 1] = reg_data[i];
    }
    err = i2c_master_transmit(bme68x_i2c_handle, data_addr, len + 1, -1);
    free(data_addr);
  } else {
    err = i2c_master_transmit(bme68x_i2c_handle, reg_data, len, -1);
  }

  ESP_ERROR_CHECK(err);

  return (BME68X_INTF_RET_TYPE)err;
}

/*!
 *  @brief Prints the execution status of the APIs.
 */
void bme68x_error_codes_print_result(const char api_name[], int8_t rslt) {
  switch (rslt) {
  case BME68X_OK:

    /* Do nothing */
    break;
  case BME68X_E_NULL_PTR:
    ESP_LOGE(TAG, "API name [%s]  Error [%d] : Null pointer", api_name, rslt);
    break;
  case BME68X_E_COM_FAIL:
    ESP_LOGE(TAG, "API name [%s]  Error [%d] : Communication failure", api_name,
             rslt);
    break;
  case BME68X_E_INVALID_LENGTH:
    ESP_LOGE(TAG, "API name [%s]  Error [%d] : Incorrect length parameter",
             api_name, rslt);
    break;
  case BME68X_E_DEV_NOT_FOUND:
    ESP_LOGE(TAG, "API name [%s]  Error [%d] : Device not found", api_name,
             rslt);
    break;
  case BME68X_E_SELF_TEST:
    ESP_LOGE(TAG, "API name [%s]  Error [%d] : Self test error", api_name,
             rslt);
    break;
  case BME68X_W_NO_NEW_DATA:
    ESP_LOGE(TAG, "API name [%s]  Warning [%d] : No new data found", api_name,
             rslt);
    break;
  default:
    ESP_LOGE(TAG, "API name [%s]  Error [%d] : Unknown error code", api_name,
             rslt);
    break;
  }
}

void bme68x_delay_us(uint32_t period_us, void *intf_ptr) {
  uint32_t msec = period_us / 1000;
  if (period_us % 1000 > 0) {
    msec++;
  }
  vTaskDelay(pdMS_TO_TICKS(msec));
}
#endif