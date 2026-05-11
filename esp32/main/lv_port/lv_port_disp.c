#include "lv_port_disp.h"

#include <stdlib.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9488.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include "lvgl.h"

#include "../config.h"

#define TAG "lv_port_disp"

#define LCD_HOST SPI2_HOST
#define PIN_NUM_MOSI DISPLAY_MOSI_PIN
#define PIN_NUM_CLK DISPLAY_CLK_PIN
#define PIN_NUM_CS DISPLAY_CS_PIN
#define PIN_NUM_DC DISPLAY_DC_PIN
#define PIN_NUM_RST DISPLAY_RST_PIN
#define PIN_NUM_BK DISPLAY_BACKLIGHT_PIN

#define LCD_H_RES DISPLAY_WIDTH
#define LCD_V_RES DISPLAY_HEIGHT
#define LCD_SPI_PCLK_HZ (20 * 1000 * 1000)
#define LCD_DRAW_BUF_LINES 40
#define LCD_DRAW_BUF_MIN_LINES 4

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;
static esp_lcd_panel_handle_t lcd_handle = NULL;
static SemaphoreHandle_t lvgl_mutex = NULL;
static lv_disp_drv_t s_disp_drv;
static bool s_lv_port_ready = false;

void lvgl_acquire(void);
void lvgl_release(void);

static bool alloc_draw_buffers_with_fallback(size_t max_lines, size_t *out_lines, bool *out_double_buffer)
{
    static const uint16_t candidate_lines[] = {40, 32, 24, 20, 16, 12, 8, 6, 4};

    for (size_t i = 0; i < sizeof(candidate_lines) / sizeof(candidate_lines[0]); ++i)
    {
        const size_t lines = candidate_lines[i];
        if (lines < LCD_DRAW_BUF_MIN_LINES || lines > max_lines)
        {
            continue;
        }

        const size_t buf_bytes = LCD_H_RES * lines * sizeof(lv_color_t);
        lv_color_t *a = (lv_color_t *)heap_caps_malloc(buf_bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        lv_color_t *b = (lv_color_t *)heap_caps_malloc(buf_bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

        if (a != NULL && b != NULL)
        {
            buf1 = a;
            buf2 = b;
            *out_lines = lines;
            *out_double_buffer = true;
            return true;
        }

        if (b != NULL)
        {
            free(b);
            b = NULL;
        }

        if (a != NULL)
        {
            // Fallback to single DMA buffer when memory is tight.
            buf1 = a;
            buf2 = NULL;
            *out_lines = lines;
            *out_double_buffer = false;
            return true;
        }
    }

    return false;
}

static bool create_ili9488_panel_with_fallback(esp_lcd_panel_io_handle_t io_handle,
                                                const esp_lcd_panel_dev_config_t *panel_config,
                                                size_t *out_conv_lines)
{
    static const uint16_t candidate_lines[] = {40, 32, 24, 20, 16, 12, 8, 6, 4, 2, 1};

    for (size_t i = 0; i < sizeof(candidate_lines) / sizeof(candidate_lines[0]); ++i)
    {
        const size_t conv_lines = candidate_lines[i];
        const size_t conv_pixels = LCD_H_RES * conv_lines;
        const size_t conv_bytes = conv_pixels * 3;

        ESP_LOGI(TAG,
                 "ILI9488 conv buffer try: lines=%u pixels=%u bytes=%u dma_free=%u dma_largest=%u",
                 (unsigned)conv_lines,
                 (unsigned)conv_pixels,
                 (unsigned)conv_bytes,
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_DMA),
                 (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_DMA));

        esp_err_t err = esp_lcd_new_panel_ili9488(io_handle, panel_config, conv_pixels, &lcd_handle);
        if (err == ESP_OK)
        {
            *out_conv_lines = conv_lines;
            ESP_LOGI(TAG, "ILI9488 panel created with conv lines=%u", (unsigned)conv_lines);
            return true;
        }

        ESP_LOGW(TAG,
                 "esp_lcd_new_panel_ili9488 failed (lines=%u): %s",
                 (unsigned)conv_lines,
                 esp_err_to_name(err));

        if (err != ESP_ERR_NO_MEM)
        {
            break;
        }
    }

    return false;
}

static inline void swap_rgb565_bytes_if_needed(lv_color_t *color_map, size_t px_cnt)
{
    // ILI9488 SPI path converts RGB565 to RGB666 inside the panel driver.
    // Swapping bytes here causes incorrect color conversion.
    (void)color_map;
    (void)px_cnt;
}

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                    esp_lcd_panel_io_event_data_t *edata,
                                    void *user_ctx)
{
    (void)panel_io;
    (void)edata;
    lv_disp_drv_t *disp_drv = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_drv);
    return false;
}

static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    const size_t px_cnt = (size_t)(area->x2 - area->x1 + 1) * (size_t)(area->y2 - area->y1 + 1);
    swap_rgb565_bytes_if_needed(color_map, px_cnt);

    esp_err_t err = esp_lcd_panel_draw_bitmap(lcd_handle,
                                              area->x1,
                                              area->y1,
                                              area->x2 + 1,
                                              area->y2 + 1,
                                              color_map);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_lcd_panel_draw_bitmap failed: %s", esp_err_to_name(err));
        lv_disp_flush_ready(drv);
    }
}

void lv_port_init(void)
{
    if (s_lv_port_ready)
    {
        return;
    }

    ESP_LOGI(TAG, "Initializing display port...");

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
        .max_transfer_sz = LCD_H_RES * LCD_DRAW_BUF_LINES * sizeof(lv_color_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = LCD_SPI_PCLK_HZ,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = notify_lvgl_flush_ready,
        .user_ctx = &s_disp_drv,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(6, 0, 0)
        .color_space = DISPLAY_RGB_ORDER,
#else
        .rgb_ele_order = DISPLAY_RGB_ORDER,
#endif
        .bits_per_pixel = 18,
    };

    size_t conv_lines = LCD_DRAW_BUF_LINES;
    if (!create_ili9488_panel_with_fallback(io_handle, &panel_config, &conv_lines))
    {
        ESP_LOGE(TAG, "Failed to create ILI9488 panel with all conversion buffer candidates");
        return;
    }
    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(lcd_handle, DISPLAY_SWAP_XY));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(lcd_handle, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(lcd_handle, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(lcd_handle, DISPLAY_INVERT_COLOR));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcd_handle, true));

    size_t draw_lines = conv_lines;
    bool double_buffer = true;
    const size_t free_dma_before = heap_caps_get_free_size(MALLOC_CAP_DMA);
    const size_t largest_dma_before = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);
    ESP_LOGI(TAG, "DMA heap before LVGL draw-buf alloc: free=%u largest=%u",
             (unsigned)free_dma_before, (unsigned)largest_dma_before);

    if (!alloc_draw_buffers_with_fallback(conv_lines, &draw_lines, &double_buffer))
    {
        ESP_LOGE(TAG, "Failed to allocate LVGL draw buffers (DMA/internal)");
        return;
    }

    const size_t bytes_per_buf = LCD_H_RES * draw_lines * sizeof(lv_color_t);
    ESP_LOGI(TAG, "LVGL draw buffer ready: lines=%u bytes=%u mode=%s",
             (unsigned)draw_lines,
             (unsigned)bytes_per_buf,
             double_buffer ? "double" : "single");

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LCD_H_RES * draw_lines);

    lv_disp_drv_init(&s_disp_drv);
    s_disp_drv.hor_res = LCD_H_RES;
    s_disp_drv.ver_res = LCD_V_RES;
    s_disp_drv.flush_cb = flush_cb;
    s_disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&s_disp_drv);

    ESP_LOGI(TAG, "LVGL display port initialized.");
    s_lv_port_ready = true;

    lvgl_mutex = xSemaphoreCreateMutex();
    if (lvgl_mutex == NULL)
    {
        ESP_LOGE(TAG, "Failed to create LVGL mutex");
    }
}

bool lv_port_is_ready(void)
{
    return s_lv_port_ready;
}

void lvgl_acquire(void)
{
    if (lvgl_mutex)
    {
        xSemaphoreTake(lvgl_mutex, portMAX_DELAY);
    }
}

void lvgl_release(void)
{
    if (lvgl_mutex)
    {
        xSemaphoreGive(lvgl_mutex);
    }
}
