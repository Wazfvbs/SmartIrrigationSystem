#include <stdio.h>
#include "uart_receiver.h"
#include "wifi_manager.h"
#include "upload_manager.h"
#include "app_mqtt.h"
#include "config_manager.h"
#include "display_manager.h"
#include "lv_port_disp.h"
#include "lvgl.h"

#include "lv_port_disp.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_st7796.h"
#include "esp_log.h"
#include "lvgl.h"
#include "driver/gpio.h"

void app_main(void)
{
    config_manager_init();
    uart_receiver_init();
    wifi_manager_init();
    //mqtt_client_start();
    upload_manager_start();
    lv_init();
    //lv_port_init();         // 初始化屏幕
    display_manager_init(); // 初始化显示内容
    display_manager_show_env(uart_receiver_get_latest_data());
    

    
    while (1)
    {
        lv_timer_handler(); // 每 5~10ms 调用一次
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
