#include "command_manager.h"
#include "uart_receiver.h"
#include "led_manager.h"
#include "config_manager.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "Command_Manager";

// 发送指令到STM32
static void send_command_to_stm32(const char *command)
{
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
        send_command_to_stm32("water_on");
    }
    else if (strcmp(command, "water_off") == 0)
    {
        send_command_to_stm32("water_off");
    }
    else if (strcmp(command, "led_on") == 0)
    {
        led_manager_set(true);
    }
    else if (strcmp(command, "led_off") == 0)
    {
        led_manager_set(false);
    }
    else if (strcmp(command, "update_threshold") == 0)
    {
        cJSON *payload = cJSON_GetObjectItem(root, "payload");
        if (payload)
        {
            cJSON *upper = cJSON_GetObjectItem(payload, "upper");
            cJSON *lower = cJSON_GetObjectItem(payload, "lower");
            if (cJSON_IsNumber(upper) && cJSON_IsNumber(lower))
            {
                config_manager_update_threshold(upper->valuedouble, lower->valuedouble);
                send_command_to_stm32("update_threshold"); // 通知STM32
            }
        }
    }
    else if (strcmp(command, "rename_device") == 0)
    {
        cJSON *payload = cJSON_GetObjectItem(root, "payload");
        if (payload && cJSON_IsString(payload))
        {
            config_manager_update_device_name(payload->valuestring);
        }
    }
    else if (strcmp(command, "bind_species") == 0)
    {
        cJSON *payload = cJSON_GetObjectItem(root, "payload");
        if (payload && cJSON_IsString(payload))
        {
            config_manager_update_species(payload->valuestring);
        }
    }
    else
    {
        ESP_LOGW(TAG, "Unknown command received: %s", command);
    }

    cJSON_Delete(root);
}
