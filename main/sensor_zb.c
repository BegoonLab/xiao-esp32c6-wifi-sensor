/**
 * @file    sensor_zb.c
 * @author  Alexander Begoon (<a href="mailto:alex\@begoonlab.tech">alex\@begoonlab.tech</a>)
 * @date    18 October 2024
 * @brief   //TODO
 * 
 * @details //TODO
 * 
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#include "sensor_zb.h"

static const char *TAG = "sensor_zigbee";
static esp_timer_handle_t s_oneshot_timer;

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

static int16_t zb_value_to_s16(float value) {
    return (int16_t) (value * 100);
}

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask) {
    ESP_RETURN_ON_FALSE(esp_zb_bdb_start_top_level_commissioning(mode_mask) == ESP_OK, ,
                        TAG, "Failed to start Zigbee bdb commissioning");
}

static esp_err_t deferred_driver_init(void) {
//    temperature_sensor_config_t temp_sensor_config =
//            TEMPERATURE_SENSOR_CONFIG_DEFAULT(ESP_TEMP_SENSOR_MIN_VALUE, ESP_TEMP_SENSOR_MAX_VALUE);
//    ESP_RETURN_ON_ERROR(temp_sensor_driver_init(&temp_sensor_config, ESP_TEMP_SENSOR_UPDATE_INTERVAL, esp_app_temp_sensor_handler), TAG,
//                        "Failed to initialize temperature sensor");
//    ESP_RETURN_ON_FALSE(switch_driver_init(button_func_pair, PAIR_SIZE(button_func_pair), esp_app_buttons_handler), ESP_FAIL, TAG,
//                        "Failed to initialize switch driver");
    return ESP_OK;
}


void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct) {
    uint32_t *p_sg_p = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;
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
        case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
            if (err_status == ESP_OK) {
                trigger_breath_effect();
                ESP_LOGI(TAG, "Deferred driver initialization %s", deferred_driver_init() ? "failed" : "successful");
                ESP_LOGI(TAG, "Device started up in %s factory-reset mode", esp_zb_bdb_is_factory_new() ? "" : "non");
                if (esp_zb_bdb_is_factory_new()) {
                    ESP_LOGI(TAG, "Start network steering");
                    esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
                } else {
                    zb_deep_sleep_start();
                    ESP_LOGI(TAG, "Device rebooted");
                }
            } else {
                /* commissioning failed */
                ESP_LOGW(TAG, "Failed to initialize Zigbee stack (status: %s)", esp_err_to_name(err_status));
            }
            break;
        case ESP_ZB_BDB_SIGNAL_STEERING:
            if (err_status == ESP_OK) {
                trigger_quick_blink();
                esp_zb_ieee_addr_t extended_pan_id;
                esp_zb_get_extended_pan_id(extended_pan_id);
                ESP_LOGI(TAG,
                         "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d, Short Address: 0x%04hx)",
                         extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                         extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                         esp_zb_get_pan_id(), esp_zb_get_current_channel(), esp_zb_get_short_address());
                zb_deep_sleep_start();
            } else {
                ESP_LOGW(TAG, "Network steering was not successful (status: %s)", esp_err_to_name(err_status));
                esp_zb_scheduler_alarm((esp_zb_callback_t) bdb_start_top_level_commissioning_cb,
                                       ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
            }
            break;
        default:
            ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type,
                     esp_err_to_name(err_status));
            break;
    }
}

static esp_zb_cluster_list_t *custom_sensor_clusters_create(esp_zb_configuration_tool_cfg_t *sensor) {
    char *manufacturer_name = build_zcl_string(MANUFACTURER_NAME);
    char *model_identifier = build_zcl_string(CONFIG_IDF_TARGET);
    char *sensor_id = build_zcl_string(CONFIG_SENSOR_ID);
    char *version = build_zcl_string(GIT_COMMIT_HASH);

    if (!manufacturer_name || !model_identifier || !sensor_id || !version) {
        ESP_LOGE(TAG, "Failed to build ZCL strings.");
        return NULL;
    }

    esp_zb_cluster_list_t *cluster_list = esp_zb_zcl_cluster_list_create();

    // Create basic cluster
    esp_zb_attribute_list_t *basic_cluster = esp_zb_basic_cluster_create(&(sensor->basic_cfg));

    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID,
                                                  manufacturer_name));
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_SERIAL_NUMBER_ID, sensor_id));
    ESP_ERROR_CHECK(
            esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, model_identifier));

    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_SW_BUILD_ID, version));

    // Add Basic cluster
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_basic_cluster(cluster_list, basic_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

    // Add Temperature Measurement cluster
    esp_zb_temperature_meas_cluster_cfg_t temp_meas_cfg = {
            .measured_value = zb_value_to_s16(temperature),
            .min_value = zb_value_to_s16(ESP_TEMP_SENSOR_MIN_VALUE),
            .max_value = zb_value_to_s16(ESP_TEMP_SENSOR_MAX_VALUE)
    };
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_temperature_meas_cluster(cluster_list,
                                                                     esp_zb_temperature_meas_cluster_create(
                                                                             &temp_meas_cfg),
                                                                     ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

    // Add Humidity Measurement cluster
    esp_zb_humidity_meas_cluster_cfg_t humidity_cfg = {
            .measured_value = zb_value_to_s16(humidity),
            .min_value = zb_value_to_s16(ESP_HUMIDITY_SENSOR_MIN_VALUE),
            .max_value = zb_value_to_s16(ESP_HUMIDITY_SENSOR_MAX_VALUE)
    };
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_humidity_meas_cluster(cluster_list,
                                                                  esp_zb_humidity_meas_cluster_create(&humidity_cfg),
                                                                  ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

    // Add Pressure Measurement cluster
    esp_zb_pressure_meas_cluster_cfg_t pressure_cfg = {
            .measured_value = zb_value_to_s16(humidity),
            .min_value = zb_value_to_s16(ESP_PRESSURE_SENSOR_MIN_VALUE),
            .max_value = zb_value_to_s16(ESP_PRESSURE_SENSOR_MAX_VALUE)
    };
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_pressure_meas_cluster(cluster_list,
                                                                  esp_zb_pressure_meas_cluster_create(&pressure_cfg),
                                                                  ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

    // Add Battery Diagnostic cluster
    esp_zb_power_config_cluster_cfg_t power_cfg = {
            .main_voltage = 36,
            .main_voltage_min = 33,
            .main_voltage_max = 42
    };
    uint16_t battery_voltage = 38;
    esp_zb_attribute_list_t *power_config_cluster = esp_zb_power_config_cluster_create(&power_cfg);
    ESP_ERROR_CHECK(
            esp_zb_power_config_cluster_add_attr(power_config_cluster, ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID,
                                                 &battery_voltage));
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_power_config_cluster(cluster_list, power_config_cluster,
                                                                 ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_identify_cluster(cluster_list, esp_zb_identify_cluster_create(
            &(sensor->identify_cfg)), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_identify_cluster(cluster_list, esp_zb_zcl_attr_list_create(
            ESP_ZB_ZCL_CLUSTER_ID_IDENTIFY), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE));

    //

    // Free allocated memory after use
    free(manufacturer_name);
    free(sensor_id);
    free(model_identifier);
    free(version);

    return cluster_list;
}

static esp_zb_ep_list_t *custom_sensor_ep_create(
        uint8_t endpoint_id, esp_zb_configuration_tool_cfg_t *sensor) {
    esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
    esp_zb_endpoint_config_t endpoint_config = {
            .endpoint = endpoint_id,
            .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
            .app_device_id = ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID,
            .app_device_version = 0
    };
    esp_zb_ep_list_add_ep(ep_list, custom_sensor_clusters_create(sensor), endpoint_config);
    return ep_list;
}

static void s_oneshot_timer_callback(void *arg) {
    /* Enter deep sleep */
    ESP_LOGI(TAG, "Enter deep sleep");
    esp_deep_sleep_start();
}

static void sensor_zb_task(void *pvParameters) {
    const esp_timer_create_args_t s_oneshot_timer_args = {
            .callback = &s_oneshot_timer_callback,
            .name = "one-shot"
    };

    ESP_ERROR_CHECK(esp_timer_create(&s_oneshot_timer_args, &s_oneshot_timer));

    /* Initialize Zigbee stack */
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZED_CONFIG();
    esp_zb_init(&zb_nwk_cfg);

    /* Create customized temperature sensor endpoint */
    esp_zb_configuration_tool_cfg_t sensor_cfg = {
            .basic_cfg = {
                    .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
                    .power_source = (uint8_t) 0x03, // Battery
            },
            .identify_cfg = {
                    .identify_time = ESP_ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE,
            }
    };
    esp_zb_ep_list_t *esp_zb_sensor_ep = custom_sensor_ep_create(HA_ESP_SENSOR_ENDPOINT, &sensor_cfg);

    /* Register the device */
    esp_zb_device_register(esp_zb_sensor_ep);

    esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);
    ESP_ERROR_CHECK(esp_zb_start(false));

    esp_zb_stack_main_loop();
}

static void zb_deep_sleep_init(void) {
    const esp_timer_create_args_t s_oneshot_timer_args = {
            .callback = &s_oneshot_timer_callback,
            .name = "one-shot"
    };

    ESP_ERROR_CHECK(esp_timer_create(&s_oneshot_timer_args, &s_oneshot_timer));
}

void start_zb(void) {
    zb_deep_sleep_init();

    /* Start Zigbee stack task */
    xTaskCreate(sensor_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);
}

static void zb_deep_sleep_start(void) {
    /* Start the one-shot timer */
    int wakeup_time_sec = CONFIG_WAKEUP_TIME_SEC;

    if (wakeup_time_sec < 5 || wakeup_time_sec > 86400) {
        ESP_LOGW(TAG, "Invalid WAKEUP_TIME_SEC: %d. Using default 300 seconds.", wakeup_time_sec);
        wakeup_time_sec = 300;
    }
    ESP_LOGI(TAG, "Start one-shot timer for %ds to enter the deep sleep", wakeup_time_sec);
    ESP_ERROR_CHECK(esp_timer_start_once(s_oneshot_timer, wakeup_time_sec * 1000000));
}

static char *build_zcl_string(const char *input_string) {
    size_t len = strlen(input_string);

    if (len > MAX_ZCL_STRING_LEN) {
        ESP_LOGE(TAG, "Input string length exceeds maximum allowed length.");
        return NULL;
    }

    // Allocate memory for the ZCL string (including the length byte)
    char *zcl_string = (char *) malloc(len + 1);
    if (!zcl_string) {
        ESP_LOGE(TAG, "Failed to allocate memory for ZCL string.");
        return NULL;
    }

    // Prepend the length byte
    zcl_string[0] = (char) len;

    // Copy the actual string after the length byte
    memcpy(&zcl_string[1], input_string, len);

    return zcl_string;
}
