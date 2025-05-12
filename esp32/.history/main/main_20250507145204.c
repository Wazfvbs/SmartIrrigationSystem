#include <stdio.h>
#include "uart_receiver.h"
#include "wifi_manager.h"
#include "upload_manager.h"
#include "app_mqtt.h"
#include "config_manager.h"
#include "display_manager.h"
#include "lv_port_disp.h"
#include "lvgl.h"

void app_main(void)
{
    config_manager_init();
    uart_receiver_init();
    wifi_manager_init();
    upload_manager_start();

    lv_init();
    display_manager_init();

    int counter = 0;

    while (1)
    {
        lv_timer_handler(); // 必须调用 LVGL 心跳

        // 每秒刷新一次显示内容
        if (++counter >= 100)
        {
            display_manager_show_env(uart_receiver_get_latest_data());
            counter = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}