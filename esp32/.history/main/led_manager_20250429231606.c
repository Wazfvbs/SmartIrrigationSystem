#include "led_manager.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include <stdbool.h>
static const char *TAG = "LED_Manager";

void led_manager_init(void)
{
    // 初始化LED（使用简单PWM控制）
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 5000,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = 2, // 修改成你的LED控制脚
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER_0};
    ledc_channel_config(&ledc_channel);

    ESP_LOGI(TAG, "LED Manager Initialized");
}

void led_manager_set(bool on)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, on ? 255 : 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ESP_LOGI(TAG, "LED is now %s", on ? "ON" : "OFF");
}
