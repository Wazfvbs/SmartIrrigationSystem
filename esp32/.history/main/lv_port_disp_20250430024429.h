#ifndef LV_PORT_DISP_H
#define LV_PORT_DISP_H

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 初始化 LVGL 显示接口，注册显示驱动
     */
    void lv_port_init(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LV_PORT_DISP_H
