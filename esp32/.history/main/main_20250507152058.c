#include <stdio.h>
#include "uart_receiver.h"
#include "wifi_manager.h"
#include "upload_manager.h"
#include "app_mqtt.h"
#include "config_manager.h"
#include "display_manager.h"
#include "lv_port_disp.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "main"

// LVGL 10ms 刷新任务
void lvgl_task(void *arg)
{
    while (1)
    {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// 每1秒刷新一次数据显示
void env_display_task(void *arg)
{
    while (1)
    {
        const sensor_data_t *data = uart_receiver_get_latest_data();
        if (data)
        {
            ESP_LOGI(TAG, "刷新界面数据: T=%.1f", data->env_temperature);
            display_manager_show_env(data);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    config_manager_init();
    uart_receiver_init();
    wifi_manager_init();
    upload_manager_start();

    lv_init();                 // 初始化 LVGL 核心
    display_manager_init();   // 初始化屏幕和显示内容

    // 创建 LVGL 渲染任务
    xTaskCreate(lvgl_task, "lvgl_task", 4096, NULL, 2, NULL);

    // 创建环境数据显示任务
    xTaskCreate(env_display_task, "env_display_task", 4096, NULL, 2, NULL);
}
