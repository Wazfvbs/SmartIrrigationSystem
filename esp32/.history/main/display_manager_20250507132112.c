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
#include "lvgl.h"
#include "lv_port_disp.h"

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
#define PIN_NUM_MOSI 42
#define PIN_NUM_CLK 40
#define PIN_NUM_CS 39
#define PIN_NUM_DC 45
#define PIN_NUM_RST 48
#define PIN_NUM_BK 47

#define LCD_H_RES 320
#define LCD_V_RES 480

static lv_obj_t *screen_main = NULL;
static lv_obj_t *label_title = NULL;
static lv_obj_t *label_info = NULL;
static lv_obj_t *label_status = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
extern lv_font_t songti_20;


esp_err_t display_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing LVGL display manager...");

    lv_port_init(); // 初始化 LVGL 驱动和显示接口

    screen_main = lv_obj_create(NULL);
    lv_obj_clear_flag(screen_main, LV_OBJ_FLAG_SCROLLABLE);
    lv_scr_load(screen_main);

    label_title = lv_label_create(screen_main);
    lv_label_set_text(label_title, "智慧浇水系统");
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_font(label_title, &songti_20, 0);

    label_info = lv_label_create(screen_main);
    lv_label_set_text(label_info, "正在获取环境数据...");
    lv_obj_align(label_info, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(label_info, &songti_20, 0);

    label_status = lv_label_create(screen_main);
    lv_label_set_text(label_status, "状态: 正常");
    lv_obj_align(label_status, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_text_font(label_status, &songti_20, 0);

    ESP_LOGI(TAG, "LVGL display ready.");
    return ESP_OK;
}

esp_err_t display_manager_show_env(const sensor_data_t *data)
{
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
             "温度: %.1f℃\n湿度: %.1f%%\n土壤湿度: %.1f%%\n光照: %.1flux\n水位: %.1f%%\n电量: %.1f%%",
             data->env_temperature,
             data->env_humidity,
             data->soil_moisture,
             data->light_intensity,
             data->water_level,
             data->battery_level);

    lv_label_set_text(label_info, buffer);
    lv_label_set_text(label_status, "状态: 数据刷新成功");
    return ESP_OK;
}

esp_err_t display_manager_show_message(const char *message)
{
    lv_label_set_text(label_info, message);
    lv_label_set_text(label_status, "状态: 消息展示中");
    return ESP_OK;
}

esp_err_t display_manager_show_watering(void)
{
    lv_label_set_text(label_info, "💧 正在浇水中，请稍候...");
    lv_label_set_text(label_status, "状态: 浇水中");
    return ESP_OK;
}
