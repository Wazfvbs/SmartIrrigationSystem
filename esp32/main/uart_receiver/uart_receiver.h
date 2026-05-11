#ifndef UART_RECEIVER_H
#define UART_RECEIVER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    BUTTON_SHORT_PRESS,
    BUTTON_DOUBLE_CLICK,
    BUTTON_LONG_PRESS,
    BUTTON_LEFT_SHORT_PRESS,
    BUTTON_MIDDLE_SHORT_PRESS,
    BUTTON_RIGHT_SHORT_PRESS,
    BUTTON_LEFT_LONG_PRESS,
    BUTTON_MIDDLE_LONG_PRESS,
    BUTTON_RIGHT_LONG_PRESS,
} button_event_t;

typedef struct
{
    char device_id[32];
    char trace_id[32];
    float temp;
    float humidity;
    float soil;
    uint32_t light;
    float water_level;
    bool water_level_valid;
    char water[16];
    uint8_t battery;
    char timestamp[20];
} sensor_data_t;

typedef void (*sensor_data_cb_t)(const sensor_data_t *data);
typedef void (*button_event_cb_t)(button_event_t event);

/**
 * @brief 初始化 UART 接收器，启动解析任务
 */
void uart_receiver_init(void);

/**
 * @brief 注册传感器数据回调，当接收到 sensor JSON 时触发
 */
void uart_receiver_register_sensor_callback(sensor_data_cb_t cb);

/**
 * @brief 注册按钮事件回调，当接收到 button JSON 时触发
 */
void uart_receiver_register_button_callback(button_event_cb_t cb);

     /**
      * @brief 获取最新一次解析后的传感器数据（NULL 表示还未收到）
      */
    const sensor_data_t *uart_receiver_get_latest_data(void);
#endif // UART_RECEIVER_H
