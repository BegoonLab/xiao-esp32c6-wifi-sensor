/**
 * @file    sensor_led.h
 * @author  Alexander Begoon (<a href="mailto:alex\@begoonlab.tech">alex\@begoonlab.tech</a>)
 * @date    20 October 2024
 * @brief   //TODO
 * 
 * @details //TODO
 * 
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#pragma once
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (15) // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (4096) // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LEDC_FREQUENCY          (4000) // Frequency in Hertz. Set frequency at 4 kHz

void init_led(void);
void led_effect_task(void *pvParameters);
void led_breath_effect(void);
void led_quick_blink(void);
void led_slow_blink(void);
void turn_off(void);
void trigger_breath_effect(void);
void trigger_quick_blink(void);
void trigger_slow_blink(void);