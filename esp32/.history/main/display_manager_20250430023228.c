// display_manager.c
#include "display_manager.h"
#include "esp_log.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_st7796.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/spi_common.h"

#define TAG "DisplayManager"

// 定义硬件连接
/*
#define LCD_HOST SPI2_HOST
#define PIN_NUM_MOSI 11
#define PIN_NUM_CLK 12
#define PIN_NUM_CS 10
#define PIN_NUM_DC 13
#define PIN_NUM_RST 14
#define PIN_NUM_BK 21

#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_42
#define DISPLAY_MOSI_PIN      GPIO_NUM_47
#define DISPLAY_CLK_PIN       GPIO_NUM_21
#define DISPLAY_DC_PIN        GPIO_NUM_40
#define DISPLAY_RST_PIN       GPIO_NUM_45
#define DISPLAY_CS_PIN        GPIO_NUM_41
*/
#define LCD_HOST SPI2_HOST
#define PIN_NUM_MOSI 47
#define PIN_NUM_CLK 21
#define PIN_NUM_CS 41
#define PIN_NUM_DC 40
#define PIN_NUM_RST 45
#define PIN_NUM_BK_LIGHT 42

#define LCD_H_RES 480
#define LCD_V_RES 320

static esp_lcd_panel_handle_t panel_handle = NULL;

esp_err_t display_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing ST7796 LCD panel...");

    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_BK,
    };
    gpio_config(&bk_gpio_config);
    gpio_set_level(PIN_NUM_BK, 1);

    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 80 * 2 + 8,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = 40 * 1000 * 1000,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, true));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));

    ESP_LOGI(TAG, "ST7796 LCD panel initialization complete");
    return ESP_OK;
}

esp_err_t display_manager_show_env(const sensor_data_t *data)
{
    ESP_LOGI(TAG, "Showing environment data");
    char line[128];
    snprintf(line, sizeof(line),
             "Temp: %.1fC\nHumi: %.1f%%\nSoil: %.1f%%\nLight: %.1flux\nWater: %.1f%%\nBatt: %.1f%%",
             data->env_temperature,
             data->env_humidity,
             data->soil_moisture,
             data->light_intensity,
             data->water_level,
             data->battery_level);
    ESP_LOGI(TAG, "%s", line);
    // 后续集成 lvgl 或字模绘制文本显示
    return ESP_OK;
}

esp_err_t display_manager_show_message(const char *message)
{
    ESP_LOGI(TAG, "Showing message: %s", message);
    // 后续添加文字显示或帧动画
    return ESP_OK;
}

esp_err_t display_manager_show_watering(void)
{
    ESP_LOGI(TAG, "Showing watering screen");
    // 后续添加动画或符号显示
    return ESP_OK;
}