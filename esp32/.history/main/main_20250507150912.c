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
    // lv_port_init();         // 正确初始化显示（只调这一次！）

    display_manager_init(); // 初始化 LVGL 画面和内容
    display_manager_show_env(uart_receiver_get_latest_data());

    while (1)
    {
            lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
