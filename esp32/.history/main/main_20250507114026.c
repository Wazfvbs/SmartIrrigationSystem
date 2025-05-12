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

#include "lvgl.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_st7796.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LCD_HOST SPI2_HOST
#define PIN_NUM_MOSI 42
#define PIN_NUM_CLK 40
#define PIN_NUM_CS 39
#define PIN_NUM_DC 45
#define PIN_NUM_RST 48
#define PIN_NUM_BK 47
#define LCD_H_RES 480
#define LCD_V_RES 320

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

    static const char *TAG = "main";

    // 1. 初始化 SPI 总线
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // 2. 初始化 IO 接口
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = 10 * 1000 * 1000, // 降频测试
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_HOST, &io_config, &io_handle));

    // 3. 初始化屏幕面板
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // 4. 填充整屏为红色
    test_buf = heap_caps_malloc(LCD_H_RES * LCD_V_RES * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(test_buf != NULL); // 防止分配失败
    static lv_color_t test_buf[LCD_H_RES * LCD_V_RES];
    for (int i = 0; i < LCD_H_RES * LCD_V_RES; i++)
    {
        test_buf[i].full = 0xF800; // 红色
    }
    ESP_LOGI(TAG, "刷红屏测试...");
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_H_RES, LCD_V_RES, test_buf));

    // 5. 暂停等待
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    while (1)
    {
        lv_timer_handler(); // 每 5~10ms 调用一次
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
