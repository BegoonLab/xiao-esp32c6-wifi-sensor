idf_component_register(
  SRCS
  "main.c"
  "sensor_adc.c"
  "sensor_bme.c"
  "sensor_data.c"
  "sensor_gpio.c"
  "sensor_i2c.c"
  "sensor_id.c"
  "sensor_led.c"
  "sensor_matter.cpp"
  "sensor_mqtt.c"
  "sensor_nvs.c"
  "sensor_sgp.c"
  "sensor_sleep.c"
  "sensor_wifi.c"
  "sensor_zb.c"
  "../vendor/embedded-i2c-sgp41/sgp41_i2c.c"
  "../vendor/embedded-i2c-sgp41/sensirion_i2c.c"
  "../vendor/embedded-i2c-sgp41/sensirion_common.c"
  "../vendor/gas-index-algorithm/sensirion_gas_index_algorithm/sensirion_gas_index_algorithm.c"
  "../vendor/boschsensortec/BME280_SensorAPI/bme280.c"
  "../vendor/boschsensortec/BME68x_SensorAPI/bme68x.c"
  INCLUDE_DIRS
  "."
  "../vendor/embedded-i2c-sgp41"
  "../vendor/gas-index-algorithm/sensirion_gas_index_algorithm"
  "../vendor/boschsensortec/BME280_SensorAPI"
  "../vendor/boschsensortec/BME68x_SensorAPI")

set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")
