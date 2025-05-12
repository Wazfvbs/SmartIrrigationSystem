#include "uart_receiver.h"
#include "upload_manager.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "UART_RECEIVER";
static sensor_data_t latest_data;

// 声明外部标志（在 main.c 定义）
extern void uart_receiver_set_display_update_flag(void);

// 串口接收任务
static void uart_receive_task(void *arg)
{
    uint8_t *data = (uint8_t *)malloc(UART_RX_BUF_SIZE);
    if (data == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate UART RX buffer");
        vTaskDelete(NULL);
        return;
    }

    while (1)
    {
        int len = uart_read_bytes(UART_PORT_NUM, data, UART_RX_BUF_SIZE - 1, pdMS_TO_TICKS(100));
        if (len > 0)
        {
            data[len] = '\0'; // 以字符串结束
            ESP_LOGI(TAG, "Received: %s", (char *)data);

            int t, h, soil, water, light;
            if (sscanf((char *)data, "T=%d,H=%d,soil=%d,water=%d,light=%d", &t, &h, &soil, &water, &light) == 5)
            {
                latest_data.env_temperature = (float)t;
                latest_data.env_humidity = (float)h;
                latest_data.soil_moisture = (float)soil;
                latest_data.water_level = (float)water;
                latest_data.light_intensity = (float)light;
                latest_data.battery_level = 0; // 暂无电量数据

                ESP_LOGI(TAG, "Parsed: T=%d, H=%d, soil=%d, water=%d, light=%d",
                         t, h, soil, water, light);

                upload_manager_update_data((char *)data);

                // 设置屏幕刷新标志
                uart_receiver_set_display_update_flag();
            }
            else
            {
                ESP_LOGW(TAG, "Invalid UART data format: %s", (char *)data);
            }
        }
    }

    free(data);
    vTaskDelete(NULL);
}

void uart_receiver_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_RX_BUF_SIZE, UART_TX_BUF_SIZE, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    xTaskCreate(uart_receive_task, "uart_receive_task", 4096, NULL, 10, NULL);
    ESP_LOGI(TAG, "UART Receiver Initialized");
}

const sensor_data_t *uart_receiver_get_latest_data(void)
{
    return &latest_data;
}
