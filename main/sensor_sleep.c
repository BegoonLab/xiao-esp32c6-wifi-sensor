/**
 * @file    sensor_sleep.c
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    13 December 2024
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#include "sensor_sleep.h"

static const char *TAG = "sensor_sleep";

void go_sleep(void) {
  int wakeup_time_sec = CONFIG_WAKEUP_TIME_SEC;

  if (wakeup_time_sec < 5 || wakeup_time_sec > 86400) {
    ESP_LOGW(TAG, "Invalid WAKEUP_TIME_SEC: %d. Using default 300 seconds.",
             wakeup_time_sec);
    wakeup_time_sec = 300;
  }

  ESP_LOGI(TAG, "Enabling timer wakeup, %ds", wakeup_time_sec);
  ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));

  // Enter deep sleep
  esp_deep_sleep_start();
}

void go_sleep_for(uint64_t time_in_us) {
  ESP_LOGI(TAG, "Enabling timer wakeup, %lluus", time_in_us);
  ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(time_in_us));

  // Enter deep sleep
  esp_deep_sleep_start();
}