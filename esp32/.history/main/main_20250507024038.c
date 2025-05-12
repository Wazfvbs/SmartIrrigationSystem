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
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello ST7796!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    // 在 main.c 中的 app_main() 中添加
    static esp_lcd_panel_handle_t panel_handle = NULL;
    static lv_disp_draw_buf_t ;
    spi_bus_config_t buscfg = {
        .mosi_io_num = 42,
        .miso_io_num = -1,
        .sclk_io_num = 40,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = 45,
        .cs_gpio_num = 39,
        .pclk_hz = 40 * 1000 * 1000,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = 48,
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16,
    };

    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 240, 240, red_buf);
    while (1)
    {
        lv_timer_handler(); // 每 5~10ms 调用一次
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
