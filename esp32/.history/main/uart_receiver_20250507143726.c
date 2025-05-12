#include "uart_receiver.h"
#include "upload_manager.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "UART_RECEIVER";
static sensor_data_t latest_data;
// 接收任务
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
            data[len] = '\0'; // 保证是字符串
            ESP_LOGI(TAG, "Received: %s", (char *)data);

            // 解析 STM32 传输的格式 T=xx,H=xx,soil=xx,water=xx,light=xx
            int t, h, soil, water, light;
            if (sscanf((char *)data, "T=%d,H=%d,soil=%d,water=%d,light=%d", &t, &h, &soil, &water, &light) == 5)
            {
                latest_data.env_temperature = (float)t;
                latest_data.env_humidity = (float)h;
                latest_data.soil_moisture = (float)soil;
                latest_data.water_level = (float)water;
                latest_data.light_intensity = (float)light;
                latest_data.battery_level = 0; // STM32未传，保留为0

                ESP_LOGI(TAG, "Parsed: T=%d, H=%d, soil=%d, water=%d, light=%d",
                         t, h, soil, water, light);

                // 更新上传模块（建议此函数内部调用 uart_receiver_get_latest_data）
                upload_manager_update_data((char *)data);
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

    // 配置UART参数
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_RX_BUF_SIZE, UART_TX_BUF_SIZE, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE)); // 这里使用GPIO17（TX）和GPIO18（RX），根据实际接线调整

    // 创建接收任务
    xTaskCreate(uart_receive_task, "uart_receive_task", 4096, NULL, 10, NULL);

    ESP_LOGI(TAG, "UART Receiver Initialized");
}
static sensor_data_t latest_data;

const sensor_data_t *uart_receiver_get_latest_data(void)
{
    return &latest_data;
}