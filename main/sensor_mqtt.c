/**
 * @file    sensor_mqtt.c
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    27 September 2024
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#include "sensor_mqtt.h"

#ifdef CONFIG_SENSOR_CONNECTION_WIFI_MQTT

static const char *TAG = "sensor_mqtt";

static EventGroupHandle_t s_mqtt_event_group;

static esp_mqtt_client_handle_t clientHandle;
static char mqtt_topic_full[MQTT_TOPIC_MAX_LEN];

extern SensorData sensor_data;

void init_mqtt_client() {

  if (init_wifi_sta() != ESP_OK) {
    ESP_LOGE(TAG, "WiFi is not initiated");
    return;
  }

  snprintf(mqtt_topic_full, sizeof(mqtt_topic_full), "%s/%s/data",
           CONFIG_MQTT_TOPIC, sensor_id);

  s_mqtt_event_group = xEventGroupCreate();

  esp_mqtt_client_config_t mqtt_cfg = {
      .broker.address.uri = CONFIG_MQTT_BROKER_URI,
      .broker.address.port = CONFIG_MQTT_BROKER_PORT,
      .session.disable_keepalive = true,
#ifdef CONFIG_MQTT_ENABLE_AUTH
      .credentials.username = CONFIG_MQTT_USERNAME,
      .credentials.authentication.password = CONFIG_MQTT_PASSWORD,
#endif
  };

  // Initialize MQTT client
  ESP_LOGI(TAG, "Init MQTT client");
  clientHandle = esp_mqtt_client_init(&mqtt_cfg);

  // Register the event handler
  esp_mqtt_client_register_event(clientHandle, ESP_EVENT_ANY_ID,
                                 mqtt_event_handler, NULL);
}

void mqtt_publish(const char *data) {
  for (int i = 0; i < 3; ++i) {
    esp_err_t publish_ret =
        esp_mqtt_client_publish(clientHandle, mqtt_topic_full, data, 0,
                                CONFIG_MQTT_QOS, CONFIG_MQTT_RETAIN);

    if (publish_ret <= 0) {
      ESP_LOGE(TAG, "Failed to publish message (%s)",
               esp_err_to_name(publish_ret));
    }

    // Wait for published bit
    EventBits_t bits = xEventGroupWaitBits(
        s_mqtt_event_group, MQTT_PUBLISHED_BIT, pdFALSE, pdFALSE, WAIT);

    if (bits & MQTT_PUBLISHED_BIT) {
      trigger_quick_blink();
      xEventGroupClearBits(s_mqtt_event_group, bits);
      break;
    }

    trigger_slow_blink();

    xEventGroupClearBits(s_mqtt_event_group, bits);
  }
}

void start_mqtt() {
  // Start the MQTT client
  esp_err_t ret = esp_mqtt_client_start(clientHandle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start MQTT client (%s)", esp_err_to_name(ret));
  }

  for (int i = 0; i < 3; ++i) {
    // Wait for connection
    ESP_LOGI(TAG, "Wait for MQTT connection");
    EventBits_t bits = xEventGroupWaitBits(
        s_mqtt_event_group, MQTT_CONNECTED_BIT, pdFALSE, pdFALSE, WAIT);

    if (bits & MQTT_CONNECTED_BIT) {
      ESP_LOGI(TAG, "Connected to MQTT broker");
      trigger_quick_blink();
      xEventGroupClearBits(s_mqtt_event_group, bits);
      break;
    }
    ESP_LOGE(TAG, "MQTT connect failed");

    trigger_slow_blink();

    xEventGroupClearBits(s_mqtt_event_group, bits);

    esp_mqtt_client_reconnect(clientHandle);
  }
}

void stop_mqtt() {
  ESP_LOGI(TAG, "Stop MQTT client");
  esp_err_t err = esp_mqtt_client_stop(clientHandle);
  if (err) {
    ESP_LOGE(TAG, "Failed to stop MQTT client: (%i)", err);
    esp_mqtt_client_disconnect(clientHandle);
  }
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                        int32_t event_id, void *event_data) {
  switch (event_id) {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    xEventGroupSetBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED");
    xEventGroupSetBits(s_mqtt_event_group, MQTT_DISCONNECTED_BIT);
    break;
  case MQTT_EVENT_PUBLISHED:
    ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED");
    xEventGroupSetBits(s_mqtt_event_group, MQTT_PUBLISHED_BIT);
    break;
  case MQTT_EVENT_ERROR:
    ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
    break;
  default:
    break;
  }
}

void mqtt_prepare_json(char *json_string, int rssi,
                       struct timeval start_to_connect,
                       struct timeval end_to_connect) {
  // Create cJSON object
  cJSON *json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "ID", sensor_id);
  cJSON_AddNumberToObject(json, "RSSI", rssi);

  if (xSemaphoreTake(sensor_data.mutex, portMAX_DELAY) == pdTRUE) {
    cJSON_AddNumberToObject(json, "battery_voltage",
                            sensor_data.battery.voltage);
    cJSON_AddNumberToObject(json, "battery_charge",
                            sensor_data.battery.remaining_charge);

#if defined(CONFIG_SENSOR_BME280) || defined(CONFIG_SENSOR_BME680)
    cJSON_AddNumberToObject(json, "temperature", sensor_data.temperature);
    cJSON_AddNumberToObject(json, "humidity", sensor_data.humidity);
    cJSON_AddNumberToObject(json, "pressure", sensor_data.pressure);
#endif

#if defined(CONFIG_SENSOR_SGP41)
    cJSON_AddNumberToObject(json, "voc_raw", sensor_data.sraw_voc);
    cJSON_AddNumberToObject(json, "nox_raw", sensor_data.sraw_nox);
    cJSON_AddNumberToObject(json, "voc_index", sensor_data.voc_index_value);
    cJSON_AddNumberToObject(json, "nox_index", sensor_data.nox_index_value);
#endif

    xSemaphoreGive(sensor_data.mutex);
  }

  cJSON_AddNumberToObject(
      json, "connection_duration_ms",
      (end_to_connect.tv_sec - start_to_connect.tv_sec) * 1000 +
          (end_to_connect.tv_usec - start_to_connect.tv_usec) / 1000);

  // Convert cJSON to string
  if (!cJSON_PrintPreallocated(json, json_string, MQTT_MSG_MAX_LEN, false)) {
    ESP_LOGE(TAG, "Can't marshal JSON data");
  }

  cJSON_Delete(json);
  json = NULL;
}

void mqtt_send_sensor_data(void) {
  struct timeval start_to_connect;
  gettimeofday(&start_to_connect, NULL);
  if (start_wifi() == ESP_OK) {
    start_mqtt();

    struct timeval end_to_connect;
    gettimeofday(&end_to_connect, NULL);

    int rssi = -100;
    esp_wifi_sta_get_rssi(&rssi);
    ESP_LOGI(TAG, "RSSI: %d", rssi);

    char *json_string = (char *)malloc(MQTT_MSG_MAX_LEN * sizeof(char));
    if (json_string == NULL) {
      ESP_LOGE(TAG, "Failed to allocate memory for JSON string");
    } else {
      mqtt_prepare_json(json_string, rssi, start_to_connect, end_to_connect);

      mqtt_publish(json_string);

      // Cleanup
      free(json_string);
      json_string = NULL;
    }

    stop_mqtt();
  }

  stop_wifi();
}
#endif