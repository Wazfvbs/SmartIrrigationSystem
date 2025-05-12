// lv_port_disp.c
#include "lv_port_disp.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_st7796.h"
#include "esp_log.h"
#include "lvgl.h"
#include "driver/gpio.h"

#define TAG "lv_port_disp"

#define LCD_HOST SPI2_HOST
#define PIN_NUM_MOSI 42 // 对应 ST7796 的 SDI/DIN/MOSI
#define PIN_NUM_CLK 40  // 对应 ST7796 的 SCK
#define PIN_NUM_CS 39   // 对应 ST7796 的 CS，部分模块接地也可，但建议接控制口
#define PIN_NUM_DC 45   // 对应 ST7796 的 D/C 或 A0
#define PIN_NUM_RST 48  // 对应 ST7796 的 RESET（可接或不接，但建议接）
#define PIN_NUM_BK 47   // 背光控制引脚（接屏幕背光 EN 或 LED）
#define LCD_H_RES 480
#define LCD_V_RES 320



static lv_disp_drv_t disp_drv;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;
static esp_lcd_panel_handle_t lcd_handle = NULL;

static void flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_map)
{
    int x1 = area->x1;
    int y1 = area->y1;
    int x2 = area->x2;
    int y2 = area->y2;

    esp_lcd_panel_draw_bitmap(lcd_handle, x1, y1, x2 + 1, y2 + 1, color_map);
    lv_disp_flush_ready(disp_drv);
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x0000FF), LV_PART_MAIN); // 显示蓝色背景
    ESP_LOGI("lv_flush", "Flush area: (%d,%d) ~ (%d,%d)", area->x1, area->y1, area->x2, area->y2);
}

void lv_port_init(void)
{
    ESP_LOGI(TAG, "Initializing LVGL display port...");

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
        .max_transfer_sz = LCD_H_RES * 80 * sizeof(lv_color_t) + 8,
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
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(io_handle, &panel_config, &lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(lcd_handle, false, true));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(lcd_handle, true));

    buf1 = heap_caps_malloc(LCD_H_RES * 40 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    buf2 = heap_caps_malloc(LCD_H_RES * 40 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1 && buf2);

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LCD_H_RES * 40);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    ESP_LOGI(TAG, "LVGL display port initialized.");
}