/**
 * @file    sensor_id.c
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    13 December 2024
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#include "sensor_id.h"

static const char *TAG = "sensor_id";

void resolve_sensor_id(void) {
  // Resolve SENSOR_ID
  if (strlen(CONFIG_SENSOR_ID) == 0) {
    uint8_t mac[6];
    if (esp_base_mac_addr_get(mac) != ESP_OK) {
      ESP_LOGE(TAG, "Failed to get base MAC address");
      strlcpy(sensor_id, "0x000000000000", SENSOR_ID_MAX_LEN);
    } else {
      int ret =
          snprintf(sensor_id, SENSOR_ID_MAX_LEN, "0x%02X%02X%02X%02X%02X%02X",
                   mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      if (ret < 0 || ret >= SENSOR_ID_MAX_LEN) {
        ESP_LOGE(TAG, "Error or truncation occurred while setting SENSOR_ID");
        strlcpy(sensor_id, "0x000000000000", SENSOR_ID_MAX_LEN);
      } else {
        ESP_LOGI(TAG, "Using MAC Address as SENSOR_ID: %s", sensor_id);
      }
    }
  } else {
    strlcpy(sensor_id, CONFIG_SENSOR_ID, SENSOR_ID_MAX_LEN);
    ESP_LOGI(TAG, "Using Configured SENSOR_ID: %s", sensor_id);
  }
}