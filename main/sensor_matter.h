/**
 * @file    sensor_matter.h
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

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "esp_log.h"

esp_err_t init_matter(void);
esp_err_t start_matter(void);
esp_err_t stop_matter(void);

#ifdef __cplusplus
}
#endif
