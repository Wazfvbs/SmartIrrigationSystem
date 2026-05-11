// display_manager.c
#include "display_manager.h"
#include "esp_log.h"
#include "lvgl.h"
#include "../lv_port/lv_port_disp.h"
#include <stdio.h>   // for snprintf
#include <stdint.h> // for uint32_t, uint8_t
#include "../ui/ui.h"
#include "../ui/default_ui.h"

#define TAG "DisplayManager"

// 澶栭儴瀛椾綋寮曠敤
extern lv_font_t cjk_22;

static lv_obj_t *screen_main = NULL;
static lv_obj_t *label_title = NULL;
static lv_obj_t *label_info = NULL;
static lv_obj_t *label_status = NULL;

esp_err_t display_manager_init1(void)
{
    ESP_LOGI(TAG, "Initializing LVGL display manager...");

    lv_port_init(); // init LVGL display port
    if (!lv_port_is_ready())
    {
        ESP_LOGE(TAG, "lv_port_init failed: display port not ready");
        return ESP_FAIL;
    }
    lvgl_acquire();

    // 鍒涘缓涓诲睆骞曞苟璁剧疆鑳屾櫙鑹?
    screen_main = lv_obj_create(NULL);
    lv_obj_clear_flag(screen_main, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen_main, lv_color_white(), LV_PART_MAIN);
    lv_scr_load(screen_main);

    // Create title
    label_title = lv_label_create(screen_main);
    lv_label_set_text(label_title, "Smart Irrigation");
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 10);
    

    // Create info label
    label_info = lv_label_create(screen_main);
    lv_label_set_text(label_info, "Loading sensor data...");
    lv_obj_align(label_info, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(label_info, &cjk_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_info, lv_color_black(), LV_PART_MAIN);

    // Create status label
    label_status = lv_label_create(screen_main);
    lv_label_set_text(label_status, "Status: waiting");
    lv_obj_align(label_status, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_text_font(label_status, &cjk_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_status, lv_color_black(), LV_PART_MAIN);

    lvgl_release();
    ESP_LOGI(TAG, "LVGL display ready.");
    return ESP_OK;
}

esp_err_t display_manager_show_env1(const sensor_data_t *data)
{
    char buffer[256];
    // 娉ㄦ剰锛歭ight 鏄?uint32_t锛岀敤 %lu锛?battery 鏄?uint8_t锛岀敤 %hhu
    snprintf(buffer, sizeof(buffer),
             "娓╁害: %.1f鈩僜n"
             "婀垮害: %.1f%%\n"
             "鍦熷￥婀垮害: %.1f%%\n"
             "鍏夌収: %lu lux\n"
             "姘翠綅: %s\n"
             "鐢甸噺: %hhu%%",
             data->temp,
             data->humidity,
             data->soil,
             (unsigned long)data->light,
             data->water,
             data->battery);

    lvgl_acquire();
    lv_label_set_text(label_info, buffer);
    // 寮哄埗鍒锋柊鏍囩鎵€鍦ㄥ尯鍩?
    lv_obj_invalidate(label_info);

    lv_label_set_text(label_status, "鐘舵€? 鏁版嵁鍒锋柊鎴愬姛");
    lv_obj_invalidate(label_status);
    lvgl_release();

    return ESP_OK;
}

esp_err_t display_manager_show_message(const char *message)
{
    lvgl_acquire();
    lv_label_set_text(label_info, message);
    lv_obj_invalidate(label_info);
    lv_label_set_text(label_status, "Status: message");
    lv_obj_invalidate(label_status);
    lvgl_release();
    return ESP_OK;
}

esp_err_t display_manager_show_watering(void)
{
    lvgl_acquire();
    lv_label_set_text(label_info, "Watering in progress...");
    lv_obj_invalidate(label_info);
    lv_label_set_text(label_status, "Status: watering");
    lv_obj_invalidate(label_status);
    lvgl_release();
    return ESP_OK;
}

esp_err_t display_manager_init(void)
{
    // 鏍稿績宸插湪 main 涓?lv_init() 杩囦簡锛岃繖閲屽彧鍋氶┍鍔ㄧ粦瀹?
    lv_port_init();       // init LVGL display driver
    if (!lv_port_is_ready())
    {
        ESP_LOGE(TAG, "lv_port_init failed: display port not ready");
        return ESP_FAIL;
    }

    ui_set_mode(&ui_default); // 缁戝畾骞跺垵濮嬪寲榛樿 UI
    return ESP_OK;
}

esp_err_t display_manager_show_env(const sensor_data_t *data)
{
    if (current_ui && current_ui->update)
    {
        current_ui->update(data);
    }
    return ESP_OK;
}
