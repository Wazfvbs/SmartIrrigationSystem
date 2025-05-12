// display_manager.c
#include "display_manager.h"
#include "esp_log.h"
#include "lvgl.h"
#include "lv_port_d.h"

#define TAG "DisplayManager"

static lv_obj_t *screen_main = NULL;
static lv_obj_t *label_title = NULL;
static lv_obj_t *label_info = NULL;
static lv_obj_t *label_status = NULL;

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
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_20, 0);

    label_info = lv_label_create(screen_main);
    lv_label_set_text(label_info, "正在获取环境数据...");
    lv_obj_align(label_info, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(label_info, &lv_font_montserrat_16, 0);

    label_status = lv_label_create(screen_main);
    lv_label_set_text(label_status, "状态: 正常");
    lv_obj_align(label_status, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_text_font(label_status, &lv_font_montserrat_14, 0);

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
