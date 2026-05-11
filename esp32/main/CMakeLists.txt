idf_component_register(
  SRCS "main.c"
       "uart_receiver/uart_receiver.c"
       "wifi_manager/wifi_manager.c"
       "app_mqtt/app_mqtt.c"
       "upload_manager/upload_manager.c"
       "command_manager/command_manager.c"
       "stm32_protocol/stm32_protocol.c"
       "led_manager/led_manager.c"
       "config_manager/config_manager.c"
       "display_manager/display_manager.c"
       "lv_port/lv_port_disp.c"
       "voice_manager/voice_manager_stub.c"
       "ui/ui.c"
        "ui/default_ui.c"
        "ui/menu_ui.c"
        "ui/history_ui.c"
       "../components/fonts/cjk_22.c"
       "../components/fonts/st5.c"

  INCLUDE_DIRS "."

  REQUIRES
    esp_driver_uart
    esp_driver_ledc
    esp_wifi
    esp_netif
    esp_event
    esp_http_client
    esp_http_server
    nvs_flash
    driver
    mqtt
    json
    esp_driver_gpio
    esp_lcd
    atanisoft__esp_lcd_ili9488
    lvgl
    esp_driver_ledc
    esp_driver_spi
    freertos
    heap
    esp_psram
)
