#include <stdio.h>
#include "uart_receiver.h"
#include "wifi_manager.h"
#include "upload_manager.h"
#include "app_mqtt.h"
#include "config_manager.h"
#include "display_manager.h"

void app_main(void)
{
    config_manager_init();
    uart_receiver_init();
    wifi_manager_init();
    //mqtt_client_start();
    upload_manager_start();
    display_manager_init();
    extern sensor_data_t latest_data;
    display_manager_show_env(&latest_data);
}
