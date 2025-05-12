#include <stdio.h>
#include "uart_receiver.h"
#include "wifi_manager.h"

void app_main(void)
{
    uart_receiver_init();
    wifi_manager_init();
}
