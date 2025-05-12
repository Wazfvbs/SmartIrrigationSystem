#include <stdio.h>
#include "uart_receiver.h"
#include "wifi_manager.h"
#include "upload_manager.h"
#include "app_mqtt.h"
#include "config_manager.h"
#include "display_manager.h"
#include "lv_port_disp.h"
void app_main(void)
{
    config_manager_init();
    uart_receiver_init();
    wifi_manager_init();
    //mqtt_client_start();
    upload_manager_start();
    lv_init();
    lv_port_init();         // 初始化屏幕
    display_manager_init(); // 初始化显示内容
    display_manager_show_env(uart_receiver_get_latest_data());
    
}
