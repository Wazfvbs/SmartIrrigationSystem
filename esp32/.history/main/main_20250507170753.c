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

    while (1)
    {
        const sensor_data_t *data = uart_receiver_get_latest_data();
        if (data) {
            display_manager_show_env(data);
        }

        lv_timer_handler();  // 保持 LVGL 渲染
        vTaskDelay(pdMS_TO_TICKS(1000));  // 每秒更新一次显示
    }
}

