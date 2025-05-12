#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_st7796.h"
#include "lvgl.h"
// === 配置区域 ===
#define LCD_HOST SPI2_HOST
#define PIN_NUM_MOSI 42
#define PIN_NUM_CLK 40
#define PIN_NUM_CS 39
#define PIN_NUM_DC 45
#define PIN_NUM_RST 48
#define PIN_NUM_BK 47

#define LCD_H_RES 
#define LCD_V_RES 320480

static const char *TAG = "ST7796_TEST";

void app_main(void)
{
    ESP_LOGI(TAG, "初始化 SPI 总线...");
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "配置 ST7796 面板 IO...");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = 20 * 1000 * 1000, // SPI时钟频率
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "创建 ST7796 面板...");
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

    // 开启背光
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_BK,
    };
    gpio_config(&bk_gpio_config);
    gpio_set_level(PIN_NUM_BK, 1); // 打开背光

    // 分配显示缓冲区
    static lv_color_t *buf = NULL;
    size_t buf_size = LCD_H_RES * LCD_V_RES * sizeof(lv_color_t);
    buf = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    assert(buf != NULL);

    // 填充红色
    for (int i = 0; i < LCD_H_RES * LCD_V_RES; i++)
    {
        buf[i].full = 0xF800; // RGB565 红色
    }

    ESP_LOGI(TAG, "刷屏...");
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_H_RES, LCD_V_RES, buf));
    ESP_LOGI(TAG, "完成！红色背景应当已显示。");

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
