/**
 * @file    sensor_i2c.c
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    15 December 2024
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#include "sensor_i2c.h"

static i2c_master_bus_handle_t bus_handle;

void init_i2c(void) {
  i2c_master_bus_config_t i2c_mst_config = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port = MASTER_I2C_PORT,
      .scl_io_num = MASTER_I2C_SCL,
      .sda_io_num = MASTER_I2C_SDA,
      .glitch_ignore_cnt = 7,
  };

  ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));
}

// Get or create a device handle for the given address
i2c_master_dev_handle_t get_i2c_device(uint8_t device_address) {
  static i2c_master_dev_handle_t dev_handle;

  i2c_device_config_t i2c_dev_conf = {
      .scl_speed_hz = I2C_FREQ_HZ,
      .device_address = device_address,
  };

  ESP_ERROR_CHECK(
      i2c_master_bus_add_device(bus_handle, &i2c_dev_conf, &dev_handle));

  return dev_handle;
}

// Remove a device handle
void release_i2c_device(i2c_master_dev_handle_t dev_handle) {
  ESP_ERROR_CHECK(i2c_master_bus_rm_device(dev_handle));
}

void deinit_i2c(void) { ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle)); }