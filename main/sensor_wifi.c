/**
 * @file    sensor_wifi.c
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    27 September 2024
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#include "sensor_wifi.h"
#ifdef CONFIG_SENSOR_CONNECTION_WIFI_MQTT

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

static const char *TAG = "sensor_wifi";
static int s_retry_num = 0;
bool disconnect_wifi = false;

esp_err_t init_wifi_sta(void) {
  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id = NULL;
  esp_event_handler_instance_t instance_got_ip = NULL;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = CONFIG_ESP_WIFI_SSID,
              .password = CONFIG_ESP_WIFI_PASSWORD,
              // WPA2-PSK only
              .threshold.authmode = WIFI_AUTH_WPA2_PSK,
          },
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(
      esp_wifi_set_country_code(CONFIG_ESP_WIFI_COUNTRY_CODE, true));
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MAX_MODEM));
  ESP_LOGI(TAG, "init_wifi_sta finished.");

  return ESP_OK;
}

esp_err_t start_wifi(void) {
  ESP_ERROR_CHECK(esp_wifi_start());

  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or
   * connection failed for the maximum number of re-tries (WIFI_FAIL_BIT). The
   * bits are set by event_handler() (see above) */
  EventBits_t bits = xEventGroupWaitBits(
      s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE,
      pdMS_TO_TICKS(CONFIG_ESP_MAXIMUM_RETRY * 10000));

  /* xEventGroupWaitBits() returns the bits before the call returned, hence we
   * can test which event actually happened. */
  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "Connected to AP SSID:%s password:%s", CONFIG_ESP_WIFI_SSID,
             CONFIG_ESP_WIFI_PASSWORD);
    return ESP_OK;
  }
  if (bits & WIFI_FAIL_BIT) {
    ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
             CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
  } else {
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
  }

  xEventGroupClearBits(s_wifi_event_group, bits);

  return ESP_ERR_WIFI_NOT_CONNECT;
}

void stop_wifi(void) {
  disconnect_wifi = true;

  esp_wifi_disconnect();
  esp_wifi_stop();

  EventBits_t bits =
      xEventGroupWaitBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT, pdFALSE,
                          pdFALSE, pdMS_TO_TICKS(3000));

  ESP_LOGI(TAG, "WiFi stopped");

  xEventGroupClearBits(s_wifi_event_group, bits);
}

void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                   void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    xEventGroupSetBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT);
    if (disconnect_wifi) {
      return;
    }
    if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(TAG, "retry to connect to the AP");
    } else {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    ESP_LOGI(TAG, "connect to the AP fail");
    trigger_slow_blink();
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    trigger_quick_blink();
  }
}
#endif