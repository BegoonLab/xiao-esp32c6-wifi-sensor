/**
 * @file    sensor_openthread.h
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    03 January 2025
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#pragma once

#ifdef CONFIG_SENSOR_CONNECTION_MATTER_OVER_THREAD
#include "esp_openthread_types.h"
#include <platform/ESP32/OpenthreadLauncher.h>

#if SOC_IEEE802154_SUPPORTED
#define ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG()                                  \
  { .radio_mode = RADIO_MODE_NATIVE, }
#endif

#define ESP_OPENTHREAD_DEFAULT_HOST_CONFIG()                                   \
  { .host_connection_mode = HOST_CONNECTION_MODE_NONE, }

#define ESP_OPENTHREAD_DEFAULT_PORT_CONFIG()                                   \
  {                                                                            \
    .storage_partition_name = "nvs", .netif_queue_size = 10,                   \
    .task_queue_size = 10,                                                     \
  }
#endif // CONFIG_SENSOR_CONNECTION_MATTER_OVER_THREAD