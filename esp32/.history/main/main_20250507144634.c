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
    float last_temperature = -999;

    while (1)
    {
        lv_timer_handler(); // 必须周期调用

        if (++counter >= 100) // 每秒刷新一次
        {
            const sensor_data_t *data = uart_receiver_get_latest_data();

            // 仅当温度有更新时才刷新（避免重复）
            if (data->env_temperature != last_temperature && data->env_temperature > 0)
            {
                display_manager_show_env(data);
                last_temperature = data->env_temperature;
            }

            counter = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // 每 10ms 一次
    }
}
