#include "upload_manager.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../app_mqtt/app_mqtt.h"

static const char *TAG = "Upload_Manager";

#define UPLOAD_INTERVAL_SECONDS 300
#define MAX_UPLOAD_FAILS 5

static char latest_data[1024] = {0};
static int upload_fail_count = 0;

static void upload_task(void *arg)
{
    (void)arg;
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(UPLOAD_INTERVAL_SECONDS * 1000));

        if (strlen(latest_data) == 0)
        {
            ESP_LOGW(TAG, "No data to upload, skipping");
            continue;
        }

        ESP_LOGI(TAG, "Uploading data");

        if (!mqtt_client_is_connected())
        {
            upload_fail_count++;
            ESP_LOGW(TAG, "Cloud offline, skip upload (fail_count=%d/%d)", upload_fail_count, MAX_UPLOAD_FAILS);
            continue;
        }

        int msg_id = mqtt_publish_report(latest_data);
        if (msg_id < 0)
        {
            upload_fail_count++;
            ESP_LOGW(TAG, "Upload publish failed (fail_count=%d/%d)", upload_fail_count, MAX_UPLOAD_FAILS);
        }
        else
        {
            upload_fail_count = 0;
            ESP_LOGI(TAG, "Upload published, msg_id=%d", msg_id);
        }

        if (upload_fail_count >= MAX_UPLOAD_FAILS)
        {
            ESP_LOGE(TAG, "Upload failures reached threshold, device is likely offline");
        }
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
        latest_data[sizeof(latest_data) - 1] = '\0';
        ESP_LOGI(TAG, "Updated latest data to upload");
    }
}
