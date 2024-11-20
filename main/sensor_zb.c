/**
 * @file    sensor_zb.c
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    18 October 2024
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#include "sensor_zb.h"
#ifdef CONFIG_SENSOR_CONNECTION_ZIGBEE

static const char *TAG = "sensor_zigbee";
static esp_timer_handle_t s_oneshot_timer;

bool go_sleep = false;

extern double battery_voltage;
extern float temperature;
extern float humidity;
extern float pressure;

void init_zb(void) {
  esp_zb_platform_config_t config = {
      .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
      .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),
  };

  ESP_ERROR_CHECK(esp_zb_platform_config(&config));
}

int16_t zb_value_to_s16(float value) { return (int16_t)(value * 100); }

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask) {
  ESP_RETURN_ON_FALSE(esp_zb_bdb_start_top_level_commissioning(mode_mask) ==
                          ESP_OK,
                      , TAG, "Failed to start Zigbee bdb commissioning");
}

static esp_err_t
zb_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *message) {
  esp_err_t ret = ESP_OK;

  ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
  ESP_RETURN_ON_FALSE(
      message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG,
      TAG, "Received message: error status(%d)", message->info.status);
  ESP_LOGI(TAG,
           "Received message: endpoint(%d), cluster(0x%x), attribute(0x%x), "
           "data size(%d)",
           message->info.dst_endpoint, message->info.cluster,
           message->attribute.id, message->attribute.data.size);

  if (message->info.dst_endpoint == HA_ESP_SENSOR_ENDPOINT) {
    if (message->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_IDENTIFY) {
      if (message->attribute.id == ESP_ZB_ZCL_ATTR_IDENTIFY_IDENTIFY_TIME_ID) {
        trigger_breath_effect();
      }

      if (message->attribute.id == ESP_ZB_ZCL_IDENTIFY_EFFECT_ID_OKAY) {
        trigger_quick_blink();
      }
    }
  }

  return ret;
}

static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id,
                                   const void *message) {
  esp_err_t ret = ESP_OK;
  switch (callback_id) {
  case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
  case ESP_ZB_CORE_IDENTIFY_EFFECT_CB_ID:
    ret = zb_attribute_handler((esp_zb_zcl_set_attr_value_message_t *)message);
    break;
  default:
    ESP_LOGW(TAG, "Receive ZigBee action(0x%x) callback", callback_id);
    break;
  }
  return ret;
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct) {
  uint32_t *p_sg_p = signal_struct->p_app_signal;
  esp_err_t err_status = signal_struct->esp_err_status;
  esp_zb_app_signal_type_t sig_type = *p_sg_p;
  int before_deep_sleep_time_sec = 3;
  switch (sig_type) {
  case ESP_ZB_ZDO_SIGNAL_LEAVE:
    ESP_LOGI(TAG, "ZDO signal: Leave");
    trigger_quick_blink();
    break;
  case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
    ESP_LOGI(TAG, "Initialize Zigbee stack");
    esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
    break;
  case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    // Wait 2 min to join a Zigbee network
    before_deep_sleep_time_sec = 120;
  case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
    if (err_status == ESP_OK) {
      trigger_breath_effect();
      ESP_LOGI(TAG, "Device started up in %s factory-reset mode",
               esp_zb_bdb_is_factory_new() ? "" : "non");
      if (esp_zb_bdb_is_factory_new()) {
        ESP_LOGI(TAG, "Start network steering");
        esp_zb_bdb_start_top_level_commissioning(
            ESP_ZB_BDB_MODE_NETWORK_STEERING);
      } else {
        sensor_zb_update_reporting_info();
        sensor_zb_update_clusters();
        ESP_LOGI(TAG, "Device rebooted");
      }
    } else {
      trigger_slow_blink();
      /* commissioning failed */
      ESP_LOGW(TAG, "Failed to initialize Zigbee stack (status: %s)",
               esp_err_to_name(err_status));
    }
    zb_deep_sleep_start(before_deep_sleep_time_sec);
    break;
  case ESP_ZB_BDB_SIGNAL_STEERING:
    if (err_status == ESP_OK) {
      trigger_quick_blink();
      esp_zb_ieee_addr_t extended_pan_id;
      esp_zb_get_extended_pan_id(extended_pan_id);
      ESP_LOGI(TAG,
               "Joined network successfully (Extended PAN ID: "
               "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, "
               "Channel:%d, Short Address: 0x%04hx)",
               extended_pan_id[7], extended_pan_id[6], extended_pan_id[5],
               extended_pan_id[4], extended_pan_id[3], extended_pan_id[2],
               extended_pan_id[1], extended_pan_id[0], esp_zb_get_pan_id(),
               esp_zb_get_current_channel(), esp_zb_get_short_address());
    } else {
      ESP_LOGW(TAG, "Network steering was not successful (status: %s)",
               esp_err_to_name(err_status));
      esp_zb_scheduler_alarm(
          (esp_zb_callback_t)bdb_start_top_level_commissioning_cb,
          ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
    }
    break;
  case ESP_ZB_COMMON_SIGNAL_CAN_SLEEP:
    esp_zb_sleep_now();
    break;
  default:
    ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s",
             esp_zb_zdo_signal_to_string(sig_type), sig_type,
             esp_err_to_name(err_status));
    break;
  }
}

static esp_zb_cluster_list_t *
custom_sensor_clusters_create(esp_zb_configuration_tool_cfg_t *sensor) {
  char *manufacturer_name = build_zcl_string(MANUFACTURER_NAME);
  char *model_identifier = build_zcl_string(CONFIG_IDF_TARGET);
  char *sensor_id_identifier = build_zcl_string(sensor_id);
  char *version = build_zcl_string(GIT_COMMIT_HASH);

  if (!manufacturer_name || !model_identifier || !sensor_id_identifier ||
      !version) {
    ESP_LOGE(TAG, "Failed to build ZCL strings.");
    return NULL;
  }

  esp_zb_cluster_list_t *cluster_list = esp_zb_zcl_cluster_list_create();

  // Create basic cluster
  esp_zb_attribute_list_t *basic_cluster =
      esp_zb_basic_cluster_create(&(sensor->basic_cfg));

  ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(
      basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID,
      manufacturer_name));
  ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(
      basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_SERIAL_NUMBER_ID,
      sensor_id_identifier));
  ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(
      basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID,
      model_identifier));

  ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(
      basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_SW_BUILD_ID, version));

  // Add Basic cluster
  ESP_ERROR_CHECK(esp_zb_cluster_list_add_basic_cluster(
      cluster_list, basic_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

  // Add Temperature Measurement cluster
  esp_zb_temperature_meas_cluster_cfg_t temp_meas_cfg = {
      .measured_value = zb_value_to_s16(temperature),
      .min_value = zb_value_to_s16(ESP_TEMP_SENSOR_MIN_VALUE),
      .max_value = zb_value_to_s16(ESP_TEMP_SENSOR_MAX_VALUE)};
  ESP_ERROR_CHECK(esp_zb_cluster_list_add_temperature_meas_cluster(
      cluster_list, esp_zb_temperature_meas_cluster_create(&temp_meas_cfg),
      ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

  // Add Humidity Measurement cluster
  esp_zb_humidity_meas_cluster_cfg_t humidity_cfg = {
      .measured_value = zb_value_to_s16(humidity),
      .min_value = zb_value_to_s16(ESP_HUMIDITY_SENSOR_MIN_VALUE),
      .max_value = zb_value_to_s16(ESP_HUMIDITY_SENSOR_MAX_VALUE)};
  ESP_ERROR_CHECK(esp_zb_cluster_list_add_humidity_meas_cluster(
      cluster_list, esp_zb_humidity_meas_cluster_create(&humidity_cfg),
      ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

  // Add Pressure Measurement cluster
  esp_zb_pressure_meas_cluster_cfg_t pressure_cfg = {
      .measured_value = (int16_t)(pressure),
      .min_value = zb_value_to_s16(ESP_PRESSURE_SENSOR_MIN_VALUE),
      .max_value = zb_value_to_s16(ESP_PRESSURE_SENSOR_MAX_VALUE)};
  ESP_ERROR_CHECK(esp_zb_cluster_list_add_pressure_meas_cluster(
      cluster_list, esp_zb_pressure_meas_cluster_create(&pressure_cfg),
      ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

  // Add Battery Diagnostic cluster
  esp_zb_power_config_cluster_cfg_t power_cfg = {
      .main_voltage = (uint16_t)(battery_voltage * 10),
      .main_voltage_min = 33,
      .main_voltage_max = 42};
  uint16_t battery_voltage_measured = (uint16_t)(battery_voltage * 10);
  uint16_t battery_quantity = 1;
  uint16_t battery_percentage =
      (uint16_t)(calculate_battery_percentage(battery_voltage) * 2);
  uint16_t battery_size = ESP_ZB_ZCL_POWER_CONFIG_BATTERY_SIZE_BUILT_IN;
  esp_zb_attribute_list_t *power_config_cluster =
      esp_zb_power_config_cluster_create(&power_cfg);
  ESP_ERROR_CHECK(esp_zb_power_config_cluster_add_attr(
      power_config_cluster, ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID,
      &battery_voltage_measured));
  ESP_ERROR_CHECK(esp_zb_power_config_cluster_add_attr(
      power_config_cluster,
      ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING_ID,
      &battery_percentage));
  ESP_ERROR_CHECK(esp_zb_power_config_cluster_add_attr(
      power_config_cluster, ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_QUANTITY_ID,
      &battery_quantity));
  ESP_ERROR_CHECK(esp_zb_power_config_cluster_add_attr(
      power_config_cluster, ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_SIZE_ID,
      &battery_size));
  ESP_ERROR_CHECK(esp_zb_cluster_list_add_power_config_cluster(
      cluster_list, power_config_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
  ESP_ERROR_CHECK(esp_zb_cluster_list_add_identify_cluster(
      cluster_list, esp_zb_identify_cluster_create(&(sensor->identify_cfg)),
      ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
  ESP_ERROR_CHECK(esp_zb_cluster_list_add_identify_cluster(
      cluster_list, esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_IDENTIFY),
      ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE));

  // Free allocated memory after use
  free(manufacturer_name);
  free(sensor_id_identifier);
  free(model_identifier);
  free(version);

  return cluster_list;
}

esp_zb_ep_list_t *
custom_sensor_ep_create(uint8_t endpoint_id,
                        esp_zb_configuration_tool_cfg_t *sensor) {
  esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
  esp_zb_endpoint_config_t endpoint_config = {
      .endpoint = endpoint_id,
      .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
      .app_device_id = ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID,
      .app_device_version = 0};
  esp_zb_ep_list_add_ep(ep_list, custom_sensor_clusters_create(sensor),
                        endpoint_config);
  return ep_list;
}

void s_oneshot_timer_callback(void *arg) {
  /* Enter deep sleep */
  ESP_LOGI(TAG, "Enter deep sleep");
  esp_deep_sleep_start();
}

void sensor_zb_task(void *pvParameters) {
  const esp_timer_create_args_t s_oneshot_timer_args = {
      .callback = &s_oneshot_timer_callback, .name = "one-shot"};

  ESP_ERROR_CHECK(esp_timer_create(&s_oneshot_timer_args, &s_oneshot_timer));

  /* Initialize Zigbee stack */
  esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZED_CONFIG();
  esp_zb_sleep_enable(true);
  esp_zb_init(&zb_nwk_cfg);

  /* Create customized temperature sensor endpoint */
  esp_zb_configuration_tool_cfg_t sensor_cfg = {
      .basic_cfg =
          {
              .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
              .power_source = (uint8_t)0x03, // Battery
          },
      .identify_cfg = {
          .identify_time = ESP_ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE,
      }};
  esp_zb_ep_list_t *esp_zb_sensor_ep =
      custom_sensor_ep_create(HA_ESP_SENSOR_ENDPOINT, &sensor_cfg);

  /* Register the device */
  esp_zb_device_register(esp_zb_sensor_ep);

  esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);

  esp_zb_core_action_handler_register(zb_action_handler);

  ESP_ERROR_CHECK(esp_zb_start(false));

  esp_zb_stack_main_loop();
}

void sensor_zb_update_reporting_info(void) {
  // Create reporting info for temperature
  esp_zb_zcl_reporting_info_t temperature_reporting_info = {
      .direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_SRV,
      .ep = HA_ESP_SENSOR_ENDPOINT,
      .cluster_id = ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
      .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
      .u.send_info.min_interval = 1,
      .u.send_info.max_interval = CONFIG_WAKEUP_TIME_SEC,
      .u.send_info.def_min_interval = 1,
      .u.send_info.def_max_interval = CONFIG_WAKEUP_TIME_SEC,
      .u.send_info.delta.u16 = 10,
      .attr_id = ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
      .dst.profile_id = ESP_ZB_AF_HA_PROFILE_ID,
      .manuf_code = ESP_ZB_ZCL_ATTR_NON_MANUFACTURER_SPECIFIC,
  };
  esp_zb_lock_acquire(portMAX_DELAY);
  ESP_ERROR_CHECK(
      esp_zb_zcl_update_reporting_info(&temperature_reporting_info));
  esp_zb_lock_release();

  // Create reporting info for humidity
  esp_zb_zcl_reporting_info_t humidity_reporting_info = {
      .direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_SRV,
      .ep = HA_ESP_SENSOR_ENDPOINT,
      .cluster_id = ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT,
      .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
      .u.send_info.min_interval = 1,
      .u.send_info.max_interval = CONFIG_WAKEUP_TIME_SEC,
      .u.send_info.def_min_interval = 1,
      .u.send_info.def_max_interval = CONFIG_WAKEUP_TIME_SEC,
      .u.send_info.delta.u16 = 10,
      .attr_id = ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID,
      .dst.profile_id = ESP_ZB_AF_HA_PROFILE_ID,
      .manuf_code = ESP_ZB_ZCL_ATTR_NON_MANUFACTURER_SPECIFIC,
  };
  esp_zb_lock_acquire(portMAX_DELAY);
  ESP_ERROR_CHECK(esp_zb_zcl_update_reporting_info(&humidity_reporting_info));
  esp_zb_lock_release();

  // Create reporting info for pressure
  esp_zb_zcl_reporting_info_t pressure_reporting_info = {
      .direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_SRV,
      .ep = HA_ESP_SENSOR_ENDPOINT,
      .cluster_id = ESP_ZB_ZCL_CLUSTER_ID_PRESSURE_MEASUREMENT,
      .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
      .u.send_info.min_interval = 1,
      .u.send_info.max_interval = CONFIG_WAKEUP_TIME_SEC,
      .u.send_info.def_min_interval = 1,
      .u.send_info.def_max_interval = CONFIG_WAKEUP_TIME_SEC,
      .u.send_info.delta.u16 = 1,
      .attr_id = ESP_ZB_ZCL_ATTR_PRESSURE_MEASUREMENT_VALUE_ID,
      .dst.profile_id = ESP_ZB_AF_HA_PROFILE_ID,
      .manuf_code = ESP_ZB_ZCL_ATTR_NON_MANUFACTURER_SPECIFIC,
  };
  esp_zb_lock_acquire(portMAX_DELAY);
  ESP_ERROR_CHECK(esp_zb_zcl_update_reporting_info(&pressure_reporting_info));
  esp_zb_lock_release();
}

void zb_deep_sleep_init(void) {
  const esp_timer_create_args_t s_oneshot_timer_args = {
      .callback = &s_oneshot_timer_callback, .name = "one-shot"};

  ESP_ERROR_CHECK(esp_timer_create(&s_oneshot_timer_args, &s_oneshot_timer));

  int wakeup_time_sec = CONFIG_WAKEUP_TIME_SEC;

  if (wakeup_time_sec < 5 || wakeup_time_sec > 86400) {
    ESP_LOGW(TAG, "Invalid WAKEUP_TIME_SEC: %d. Using default 300 seconds.",
             wakeup_time_sec);
    wakeup_time_sec = 300;
  }

  ESP_LOGI(TAG, "Enabling timer wakeup, %ds\n", wakeup_time_sec);
  ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));
}

void sensor_zb_update_clusters() {
  esp_zb_zcl_status_t status;
  int16_t measured_temperature = zb_value_to_s16(temperature);
  int16_t measured_humidity = zb_value_to_s16(humidity);
  int16_t measured_pressure = (int16_t)(pressure);
  uint16_t battery_voltage_measured = (uint16_t)(battery_voltage * 10);
  uint16_t battery_percentage =
      (uint16_t)(calculate_battery_percentage(battery_voltage) * 2);

  esp_zb_lock_acquire(portMAX_DELAY);
  status = esp_zb_zcl_set_attribute_val(
      HA_ESP_SENSOR_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
      ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
      &measured_temperature, false);
  esp_zb_lock_release();
  if (status != ESP_ZB_ZCL_STATUS_SUCCESS) {
    ESP_LOGE(TAG, "Unable to set attr. Err 0x%x", status);
  }

  esp_zb_lock_acquire(portMAX_DELAY);
  status = esp_zb_zcl_set_attribute_val(
      HA_ESP_SENSOR_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT,
      ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
      ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID, &measured_humidity,
      false);
  esp_zb_lock_release();
  if (status != ESP_ZB_ZCL_STATUS_SUCCESS) {
    ESP_LOGE(TAG, "Unable to set attr. Err 0x%x", status);
  }

  esp_zb_lock_acquire(portMAX_DELAY);
  status = esp_zb_zcl_set_attribute_val(
      HA_ESP_SENSOR_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_PRESSURE_MEASUREMENT,
      ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
      ESP_ZB_ZCL_ATTR_PRESSURE_MEASUREMENT_VALUE_ID, &measured_pressure, false);
  esp_zb_lock_release();
  if (status != ESP_ZB_ZCL_STATUS_SUCCESS) {
    ESP_LOGE(TAG, "Unable to set attr. Err 0x%x", status);
  }

  esp_zb_lock_acquire(portMAX_DELAY);
  status = esp_zb_zcl_set_attribute_val(
      HA_ESP_SENSOR_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
      ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
      ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID,
      &battery_voltage_measured, false);
  esp_zb_lock_release();
  if (status != ESP_ZB_ZCL_STATUS_SUCCESS) {
    ESP_LOGE(TAG, "Unable to set attr. Err 0x%x", status);
  }

  esp_zb_lock_acquire(portMAX_DELAY);
  status = esp_zb_zcl_set_attribute_val(
      HA_ESP_SENSOR_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
      ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
      ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING_ID,
      &battery_percentage, false);
  esp_zb_lock_release();
  if (status != ESP_ZB_ZCL_STATUS_SUCCESS) {
    ESP_LOGE(TAG, "Unable to set attr. Err 0x%x", status);
  }
}

void start_zb(void) {
  zb_deep_sleep_init();

  /* Start Zigbee stack task */
  xTaskCreate(sensor_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);
}

void zb_deep_sleep_start(int before_deep_sleep_time_sec) {
  if (!go_sleep) {
    go_sleep = true;
  } else {
    return;
  }

  ESP_LOGI(TAG, "Start one-shot timer for %ds to enter the deep sleep",
           before_deep_sleep_time_sec);
  ESP_ERROR_CHECK(esp_timer_start_once(s_oneshot_timer,
                                       before_deep_sleep_time_sec * 1000000));
}

char *build_zcl_string(const char *input_string) {
  size_t len = strlen(input_string);

  if (len > MAX_ZCL_STRING_LEN) {
    ESP_LOGE(TAG, "Input string length exceeds maximum allowed length.");
    return NULL;
  }

  // Allocate memory for the ZCL string (including the length byte)
  char *zcl_string = (char *)malloc(len + 1);
  if (!zcl_string) {
    ESP_LOGE(TAG, "Failed to allocate memory for ZCL string.");
    return NULL;
  }

  // Prepend the length byte
  zcl_string[0] = (char)len;

  // Copy the actual string after the length byte
  memcpy(&zcl_string[1], input_string, len);

  return zcl_string;
}

double calculate_battery_percentage(double voltage) {
  if (voltage >= 4.2) {
    return 100.0;
  } else if (voltage >= 4.1) {
    return 90 + (voltage - 4.1) * 100; // Linear between 4.1V and 4.2V
  } else if (voltage >= 4.0) {
    return 80 + (voltage - 4.0) * 100;
  } else if (voltage >= 3.9) {
    return 70 + (voltage - 3.9) * 100;
  } else if (voltage >= 3.8) {
    return 60 + (voltage - 3.8) * 100;
  } else if (voltage >= 3.7) {
    return 50 + (voltage - 3.7) * 100;
  } else if (voltage >= 3.6) {
    return 40 + (voltage - 3.6) * 100;
  } else if (voltage >= 3.5) {
    return 30 + (voltage - 3.5) * 100;
  } else if (voltage >= 3.4) {
    return 20 + (voltage - 3.4) * 100;
  } else if (voltage >= 3.3) {
    return 10 + (voltage - 3.3) * 100;
  } else {
    return 0.0;
  }
}
#endif