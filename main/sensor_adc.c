/**
 * @file    sensor_adc.c
 * @author  Alexander Begoon (<a href="mailto:alex\@begoonlab.tech">alex\@begoonlab.tech</a>)
 * @date    29 September 2024
 * @brief   //TODO
 * 
 * @details //TODO
 * 
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#include "sensor_adc.h"

//ADC1 Channels
#if CONFIG_IDF_TARGET_ESP32
#define ADC1_CHAN0          ADC_CHANNEL_4
#define ADC1_CHAN1          ADC_CHANNEL_5
#else
#define ADC1_CHAN0          ADC_CHANNEL_2
#define ADC1_CHAN1          ADC_CHANNEL_3
#endif

static const char *TAG = "sensor_adc";

bool do_calibration1_chan0;

adc_cali_handle_t adc1_cali_chan0_handle = NULL;

adc_oneshot_unit_handle_t adc1_handle;

static int adc_raw;
static int voltage;

void init_adc(void){
    adc_oneshot_unit_init_cfg_t init_config1 = {
            .unit_id = ADC_UNIT_1,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
            .bitwidth = ADC_BITWIDTH_DEFAULT,
            .atten = ADC_ATTEN_DB_12,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_CHAN0, &config));

    do_calibration1_chan0 = adc_calibration_init(ADC_UNIT_1, ADC1_CHAN0, ADC_ATTEN_DB_12, &adc1_cali_chan0_handle);

    vTaskDelay(pdMS_TO_TICKS(5));

    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC1_CHAN0, &adc_raw));
    ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, ADC1_CHAN0, adc_raw);
    if (do_calibration1_chan0) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_raw, &voltage));
        ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, ADC1_CHAN0, voltage);
    }

    vTaskDelay(pdMS_TO_TICKS(5));
}

bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

void adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}

void deinit_adc(void) {
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
    if (do_calibration1_chan0) {
        adc_calibration_deinit(adc1_cali_chan0_handle);
    }
}

void get_battery_voltage(double *battery_voltage) {

    // Retrieve and convert the divider ratio
    double voltage_divider_ratio = CONFIG_BATTERY_VOLTAGE_DIVIDER_RATIO_INT / 100.0;

    // Validate the voltage divider ratio
    if (voltage_divider_ratio <= 0.0 || voltage_divider_ratio > 100.0) {
        ESP_LOGE(TAG, "Invalid BATTERY_VOLTAGE_DIVIDER_RATIO: %.2f. Using default 2.0", voltage_divider_ratio);
        voltage_divider_ratio = 2;
    }

    int battery_voltage_mv = 0;
    int probe_battery_voltage_mv = 0;

    for (int i = 0; i < 8; ++i) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC1_CHAN0, &adc_raw));
        if (do_calibration1_chan0) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_raw, &probe_battery_voltage_mv));
        }

        battery_voltage_mv += probe_battery_voltage_mv;
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    battery_voltage_mv /= 8;

    // Convert milli-volts to volts with two decimal places
    *battery_voltage = round((battery_voltage_mv * 3.3 * voltage_divider_ratio / 1000.0) * 100.0) / 100.0;
}