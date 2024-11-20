/**
 * @file    sensor_sleep.h
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    13 December 2024
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#pragma once

#include "esp_log.h"
#include "esp_sleep.h"

void go_sleep(void);
void go_sleep_for(uint64_t time_in_us);