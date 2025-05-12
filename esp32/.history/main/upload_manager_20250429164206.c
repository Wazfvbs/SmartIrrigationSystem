#include "upload_manager.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "Upload_Manager";

#define UPLOAD_INTERVAL_SECONDS 300 // 上传间隔5分钟
#define MAX_UPLOAD_FAILS 5          // 连续5次失败进入离线模式

static char latest_data[1024] = {0}; // 保存最新一次UART数据
static int upload_fail_count = 0;

static void upload_task(void *arg)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(UPLOAD_INTERVAL_SECONDS * 1000));

        if (strlen(latest_data) == 0)
        {
            ESP_LOGW(TAG, "No data to upload, skipping...");
            continue;
        }

        ESP_LOGI(TAG, "Uploading data: %s", latest_data);

        // 这里调用MQTT上传接口
        mqtt_publish_report(latest_data);

        // TODO: 后续可以加回执ACK确认，如果需要更加严格确认上传成功
        // 目前先简单假设上传后，失败次数归零
        upload_fail_count = 0;
    }
}

void upload_manager_start(void)
{
    xTaskCreate(upload_task, "upload_task", 4096, NULL, 8, NULL);
    ESP_LOGI(TAG, "Upload Manager started");
}

void upload_manager_update_data(const char *json_data)
{
    if (json_data != NULL)
    {
        strncpy(latest_data, json_data, sizeof(latest_data) - 1);
        latest_data[sizeof(latest_data) - 1] = '\0'; // 保证字符串结束符
        ESP_LOGI(TAG, "Updated latest data to upload");
    }
}
