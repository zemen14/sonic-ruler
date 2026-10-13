#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "driver/temperature_sensor.h"

#define TRIG_PIN        GPIO_NUM_4
#define ECHO_PIN        GPIO_NUM_5

#define TRIG_PULSE_US   10
#define NUM_SAMPLES     9
#define SAMPLE_DELAY_MS 20
#define ECHO_TIMEOUT_US 30000UL

static float speed_of_sound_cm_per_us(float temp_c) {
    float v_m_per_s = 331.3f + 0.606f * temp_c;
    return v_m_per_s / 10000.0f;
}

static uint32_t measure_echo_us(void) {
    gpio_set_level(TRIG_PIN, 0);
    esp_rom_delay_us(4);
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(TRIG_PULSE_US);
    gpio_set_level(TRIG_PIN, 0);

    int64_t t_start = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 0) {
        if ((esp_timer_get_time() - t_start) > ECHO_TIMEOUT_US) return 0;
    }

    int64_t echo_start = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 1) {
        if ((esp_timer_get_time() - echo_start) > ECHO_TIMEOUT_US) return 0;
    }
    return (uint32_t)(esp_timer_get_time() - echo_start);
}

static float measure_distance_cm(float temp_c) {
    float v = speed_of_sound_cm_per_us(temp_c);
    float samples[NUM_SAMPLES];
    int valid = 0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        uint32_t echo_us = measure_echo_us();
        if (echo_us > 0) {
            float dist = (echo_us * v) / 2.0f;
            if (dist < 400.0f) samples[valid++] = dist;
        }
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_DELAY_MS));
    }

    if (valid == 0) return -1.0f;

    for (int i = 0; i < valid - 1; i++)
        for (int j = i + 1; j < valid; j++)
            if (samples[j] < samples[i]) {
                float tmp = samples[i];
                samples[i] = samples[j];
                samples[j] = tmp;
            }

    return samples[valid / 2];
}

void app_main(void) {
    gpio_config_t trig_cfg = {
        .pin_bit_mask = (1ULL << TRIG_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&trig_cfg);
    gpio_set_level(TRIG_PIN, 0);

    gpio_config_t echo_cfg = {
        .pin_bit_mask = (1ULL << ECHO_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&echo_cfg);

    temperature_sensor_handle_t temp_handle = NULL;
    temperature_sensor_config_t temp_cfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(0, 50);
    temperature_sensor_install(&temp_cfg, &temp_handle);
    temperature_sensor_enable(temp_handle);

    printf("SonicRuler - Ultrasonic Range Finder starting...\n");

    while (1) {
        float temp_c = 25.0f;
        temperature_sensor_get_celsius(temp_handle, &temp_c);
        float dist_cm = measure_distance_cm(temp_c);

        if (dist_cm < 0.0f) {
            printf("Distance: OUT OF RANGE at %.0fC\n", temp_c);
        } else {
            printf("Distance: %.1fcm at %.0fC\n", dist_cm, temp_c);
        }

        vTaskDelay(pdMS_TO_TICKS(1000 - (NUM_SAMPLES * SAMPLE_DELAY_MS)));
    }
}
