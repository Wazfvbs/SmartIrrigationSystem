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

// UI 渲染任务
static void lvgl_task(void *arg)
{
    while (1)
    {
        lvgl_acquire();
        lv_timer_handler(); // 必须周期性调用
        lvgl_release();
        vTaskDelay(pdMS_TO_TICKS(5)); // 建议 5~10ms 周期
    }
}

// 数据更新任务
static void ui_update_task(void *arg)
{
    while (1)
    {
        const sensor_data_t *data = uart_receiver_get_latest_data();
        if (data)
        {
            display_manager_show_env(data); // 会自动刷新标签
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // 每秒更新一次
    }
}

void app_main(void)
{
    config_manager_init();
    uart_receiver_init();
    wifi_manager_init();
    upload_manager_start();
    lv_init();
    display_manager_init();

    xTaskCreate(lvgl_task, "lvgl_task", 4096, NULL, 5, NULL);
    xTaskCreate(ui_update_task, "ui_update_task", 4096, NULL, 5, NULL);
}
