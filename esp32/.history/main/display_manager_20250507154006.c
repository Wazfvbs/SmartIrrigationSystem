// display_manager.c
#include "display_manager.h"
#include "esp_log.h"
#include "lvgl.h"
#include "lv_port_disp.h"
// 硬件定义（实则由 lv_port_disp 控制）
#define TAG "DisplayManager"

// 外部字体引用
extern lv_font_t cjk_22;

static lv_obj_t *screen_main = NULL;
static lv_obj_t *label_title = NULL;
static lv_obj_t *label_info = NULL;
static lv_obj_t *label_status = NULL;

esp_err_t display_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing LVGL display manager...");

    lv_port_init(); // 初始化 LVGL 屏幕

    // 创建主屏幕并设置背景色
    screen_main = lv_obj_create(NULL);
    lv_obj_clear_flag(screen_main, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen_main, lv_color_white(), LV_PART_MAIN); // 设置白色背景
    lv_scr_load(screen_main);

    // 创建标题
    label_title = lv_label_create(screen_main);
    lv_label_set_text(label_title, "智慧浇水系统");
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_font(label_title, &cjk_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_title, lv_color_black(), LV_PART_MAIN);

    // 创建信息标签
    label_info = lv_label_create(screen_main);
    lv_label_set_text(label_info, "正在获取环境数据...");
    lv_obj_align(label_info, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(label_info, &cjk_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_info, lv_color_black(), LV_PART_MAIN);

    // 创建状态栏标签
    label_status = lv_label_create(screen_main);
    lv_label_set_text(label_status, "状态: 等待中");
    lv_obj_align(label_status, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_text_font(label_status, &cjk_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_status, lv_color_black(), LV_PART_MAIN);

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

    lvgl_acquire(); // 加锁开始
    lv_label_set_text(label_info, buffer);
    lv_label_set_text(label_status, "状态: 数据刷新成功");
    lvgl_release(); // 解锁结束

    return ESP_OK;
}

esp_err_t display_manager_show_message(const char *message)
{
    lvgl_acquire();
    lv_label_set_text(label_info, message);
    lv_label_set_text(label_status, "状态: 消息展示中");
    lvgl_release();
    return ESP_OK;
}

esp_err_t display_manager_show_watering(void)
{
    lvgl_acquire();
    lv_label_set_text(label_info, "💧 正在浇水中，请稍候...");
    lv_label_set_text(label_status, "状态: 浇水中");
    lvgl_release();
    return ESP_OK;
}
