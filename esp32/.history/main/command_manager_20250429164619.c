#include "command_manager.h"
#include "uart_receiver.h" // 如果需要转发给STM32
#include "esp_log.h"
#include "cJSON.h" // 使用cJSON解析库（ESP-IDF自带）
#include <string.h>

static const char *TAG = "Command_Manager";

// 发送指令到STM32（示例函数，可以根据实际需要修改）
static void send_command_to_stm32(const char *command)
{
    // 这里简单通过UART发送指令，实际可以加上自定义封包
    const char *prefix = "CMD:";
    char send_buf[128];
    snprintf(send_buf, sizeof(send_buf), "%s%s\n", prefix, command);
    uart_write_bytes(UART_PORT_NUM, send_buf, strlen(send_buf));
}

void command_manager_handle(const char *topic, const char *data, int data_len)
{
    if (data == NULL || data_len <= 0)
    {
        ESP_LOGW(TAG, "Empty command data received");
        return;
    }

    // 解析JSON
    cJSON *root = cJSON_ParseWithLength(data, data_len);
    if (root == NULL)
    {
        ESP_LOGE(TAG, "Failed to parse command JSON");
        return;
    }

    cJSON *command_item = cJSON_GetObjectItem(root, "command");
    if (!cJSON_IsString(command_item))
    {
        ESP_LOGE(TAG, "Invalid command format");
        cJSON_Delete(root);
        return;
    }

    const char *command = command_item->valuestring;
    ESP_LOGI(TAG, "Parsed command: %s", command);

    // 根据指令分类处理
    if (strcmp(command, "water_on") == 0)
    {
        // 转发给STM32执行启动浇水
        send_command_to_stm32("water_on");
    }
    else if (strcmp(command, "water_off") == 0)
    {
        send_command_to_stm32("water_off");
    }
    else if (strcmp(command, "led_on") == 0)
    {
        // TODO: 本地执行，比如打开LED
    }
    else if (strcmp(command, "led_off") == 0)
    {
        // TODO: 本地执行，比如关闭LED
    }
    else if (strcmp(command, "update_threshold") == 0)
    {
        // TODO: 处理更新阈值
    }
    else if (strcmp(command, "rename_device") == 0)
    {
        // TODO: 修改设备昵称
    }
    else if (strcmp(command, "bind_species") == 0)
    {
        // TODO: 绑定植物种类
    }
    else
    {
        ESP_LOGW(TAG, "Unknown command received: %s", command);
    }

    cJSON_Delete(root);
}
