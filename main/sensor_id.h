/**
 * @file    sensor_id.h
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    20 November 2024
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#pragma once

#define SENSOR_ID_MAX_LEN 18 // "0x" + 12 hex digits + null terminator

extern char sensor_id[SENSOR_ID_MAX_LEN];