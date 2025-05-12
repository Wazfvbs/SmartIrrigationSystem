#include "lvgl.h"
#include "lv_port_disp.h"

void app_main(void)
{
    lv_init();
    lv_port_init(); // 使用你已有的 lv_port_disp 初始化 ST7796

    // 设置屏幕背景为蓝色
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x0000FF), LV_PART_MAIN);

    // 添加一个文本标签
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello ST7796!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    // 主循环
    while (1)
    {
        lv_timer_handler(); // 让 LVGL 刷新
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
