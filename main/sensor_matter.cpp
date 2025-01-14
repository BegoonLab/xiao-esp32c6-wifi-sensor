/**
 * @file    sensor_matter.cpp
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    03 January 2025
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#include "sensor_matter.h"
#include "esp_matter.h"
#include "esp_matter_ota.h"
#include "sensor_openthread.h"
#include <algorithm>
#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>

static const char *TAG = "sensor_matter";

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

static void open_commissioning_window_if_necessary() {
  VerifyOrReturn(chip::Server::GetInstance().GetFabricTable().FabricCount() ==
                 0);

  chip::CommissioningWindowManager &commissionMgr =
      chip::Server::GetInstance().GetCommissioningWindowManager();
  VerifyOrReturn(commissionMgr.IsCommissioningWindowOpen() == false);

  // After removing last fabric, this example does not remove the Wi-Fi
  // credentials and still has IP connectivity so, only advertising on DNS-SD.
  CHIP_ERROR err = commissionMgr.OpenBasicCommissioningWindow(
      chip::System::Clock::Seconds16(300),
      chip::CommissioningWindowAdvertisement::kAllSupported);
  if (err != CHIP_NO_ERROR) {
    ESP_LOGE(TAG, "Failed to open commissioning window, err:%s",
             err.AsString());
  }
}

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg) {
  switch (event->Type) {
  case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
    ESP_LOGI(TAG, "Commissioning complete");
    break;

  case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
    ESP_LOGI(TAG, "Commissioning failed, fail safe timer expired");
    break;

  case chip::DeviceLayer::DeviceEventType::kFabricRemoved:
    ESP_LOGI(TAG, "Fabric removed successfully");
    open_commissioning_window_if_necessary();
    break;

  case chip::DeviceLayer::DeviceEventType::kBLEDeinitialized:
    ESP_LOGI(TAG, "BLE deinitialized and memory reclaimed");
    break;

  default:
    break;
  }
}

esp_err_t start_matter(void) {
  /* Matter start */
  esp_err_t err = esp_matter::start(app_event_cb);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start Matter, err:%d", err);
  }

  return err;
}

// This callback is invoked when clients interact with the Identify Cluster.
// In the callback implementation, an endpoint can identify itself. (e.g., by
// flashing an LED or light).
static esp_err_t app_identification_cb(identification::callback_type_t type,
                                       uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant,
                                       void *priv_data) {
  ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u",
           type, effect_id, effect_variant);
  return ESP_OK;
}

// This callback is called for every attribute update. The callback
// implementation shall handle the desired attributes and return an appropriate
// error code. If the attribute is not of your interest, please do not return an
// error code and strictly return ESP_OK.
static esp_err_t
app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id,
                        uint32_t cluster_id, uint32_t attribute_id,
                        esp_matter_attr_val_t *val, void *priv_data) {
  // Since this is just a sensor and we don't expect any writes on our
  // temperature sensor, so, return success.
  return ESP_OK;
}

esp_err_t init_matter(void) {
  /* Create a Matter node and add the mandatory Root Node device type on
   * endpoint 0 */
  node::config_t node_config;
  node_t *node = node::create(&node_config, app_attribute_update_cb,
                              app_identification_cb);

  if (node == nullptr) {
    ESP_LOGE(TAG, "Failed to create Matter node");
  }

  // add temperature sensor device
  temperature_sensor::config_t temp_sensor_config;
  endpoint_t *temp_sensor_ep = temperature_sensor::create(
      node, &temp_sensor_config, ENDPOINT_FLAG_NONE, NULL);
  if (temp_sensor_ep == nullptr) {
    ESP_LOGE(TAG, "Failed to create temperature_sensor endpoint");
  }

  // add the humidity sensor device
  humidity_sensor::config_t humidity_sensor_config;
  endpoint_t *humidity_sensor_ep = humidity_sensor::create(
      node, &humidity_sensor_config, ENDPOINT_FLAG_NONE, NULL);
  if (humidity_sensor_ep == nullptr) {
    ESP_LOGE(TAG, "Failed to create humidity_sensor endpoint");
  }

  pressure_sensor::config_t pressure_sensor_config;
  endpoint_t *pressure_sensor_ep = pressure_sensor::create(
      node, &pressure_sensor_config, ENDPOINT_FLAG_NONE, NULL);
  if (pressure_sensor_ep == nullptr) {
    ESP_LOGE(TAG, "Failed to create pressure_sensor endpoint");
  }

  esp_openthread_platform_config_t config = {
      .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
      .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
      .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
  };
  set_openthread_platform_config(&config);

  return ESP_OK;
}
