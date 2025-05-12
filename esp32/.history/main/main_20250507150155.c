#include <stdio.h>
#include "uart_receiver.h"
#include "wifi_manager.h"
#include "upload_manager.h"
#include "app_mqtt.h"
#include "config_manager.h"
#include "display_manager.h"
#include "lv_port_disp.h"
#include "lvgl.h"

// 全局刷新标志
static volatile bool g_display_needs_update = false;

// 提供给 uart_receiver 调用的设置函数
void uart_receiver_set_display_update_flag(void)
{
    g_display_needs_update = true;
}

void app_main(void)
{
    config_manager_init();
    uart_receiver_init();
    wifi_manager_init();
    upload_manager_start();

    lv_init();
    display_manager_init();

    while (1)
    {
        lv_timer_handler();

        if (g_display_needs_update)
        {
            display_manager_show_env(uart_receiver_get_latest_data());
            g_display_needs_update = false;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
