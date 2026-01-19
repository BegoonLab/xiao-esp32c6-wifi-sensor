/**
 * @file    sensor_led.c
 * @author  Alexander Begoon <alex@begoonlab.tech>
 * @date    20 October 2024
 * @brief   //TODO
 *
 * @details //TODO
 *
 * @copyright Copyright (c) 2024 <a href="https://begoonlab.tech">BegoonLab</a>.
 *            All rights reserved.
 */

#include "sensor_led.h"

// LED effects
typedef enum {
  LED_EFFECT_BREATH,
  LED_EFFECT_QUICK_BLINK,
  LED_EFFECT_SLOW_BLINK
} led_effect_t;

QueueHandle_t led_effect_queue;

void init_led(void) {
  // Prepare and then apply the LEDC PWM timer configuration
  ledc_timer_config_t ledc_timer = {
      .speed_mode = LEDC_MODE,
      .duty_resolution = LEDC_DUTY_RES,
      .timer_num = LEDC_TIMER,
      .freq_hz = LEDC_FREQUENCY, // Set output frequency at 4 kHz
      .clk_cfg = LEDC_AUTO_CLK};
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  // Prepare and then apply the LEDC PWM channel configuration
  ledc_channel_config_t ledc_channel = {.speed_mode = LEDC_MODE,
                                        .channel = LEDC_CHANNEL,
                                        .timer_sel = LEDC_TIMER,
                                        .intr_type = LEDC_INTR_DISABLE,
                                        .gpio_num = LEDC_OUTPUT_IO,
                                        .duty = 0, // Set duty to 0%
                                        .hpoint = 0};
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  // Create the queue for LED effects
  led_effect_queue = xQueueCreate(10, sizeof(led_effect_t));

  xTaskCreate(led_effect_task, "LED_Effect_Task", 512, NULL, 5, NULL);
}

// Task to handle different LED effects
void led_effect_task(void *pvParameters __attribute__((unused))) {
  led_effect_t current_effect = LED_EFFECT_BREATH;

  while (1) {
    // Wait for an effect to be queued
    turn_off();
    if (xQueueReceive(led_effect_queue, &current_effect, portMAX_DELAY)) {
      switch (current_effect) {
      case LED_EFFECT_BREATH:
        led_breath_effect();
        break;
      case LED_EFFECT_QUICK_BLINK:
        led_quick_blink();
        break;
      case LED_EFFECT_SLOW_BLINK:
        led_slow_blink();
        break;
      default:
        break;
      }
    }
  }
}

// Function to simulate a breathing effect
void led_breath_effect() {
  for (int i = 0; i < 5; ++i) {
    for (int duty = 8191; duty >= 0; duty -= 256) {
      ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
      ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
      vTaskDelay(pdMS_TO_TICKS(20));
    }
    for (int duty = 0; duty <= 8191; duty += 256) {
      ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
      ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
      vTaskDelay(pdMS_TO_TICKS(20));
    }
  }
}

// Function to blink the LED quickly 5 times
void led_quick_blink() {
  for (int i = 0; i < 10; i++) {
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 8191);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    vTaskDelay(pdMS_TO_TICKS(100));
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// Function to blink the LED slowly 5 times
void led_slow_blink() {
  for (int i = 0; i < 3; i++) {
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 8191);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    vTaskDelay(pdMS_TO_TICKS(1000));
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void turn_off() {
  ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 8191);
  ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

void trigger_breath_effect() {
  led_effect_t effect = LED_EFFECT_BREATH;
  xQueueSendToBack(led_effect_queue, &effect, 0);
}

void trigger_quick_blink() {
  led_effect_t effect = LED_EFFECT_QUICK_BLINK;
  xQueueSendToBack(led_effect_queue, &effect, 0);
}

void trigger_slow_blink() {
  led_effect_t effect = LED_EFFECT_SLOW_BLINK;
  xQueueSendToBack(led_effect_queue, &effect, 0);
}