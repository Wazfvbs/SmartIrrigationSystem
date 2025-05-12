#include "mqtt_client.h"
#include "command_manager.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MQTT_CLIENT";

#define MQTT_BROKER_URI "mqtt://192.168.1.100" // 树莓派服务器局域网IP，改成你的
#define DEVICE_ID "esp32s3_device001"          // 设备ID
#define REPORT_TOPIC "/device/esp32s3_device001/report"
#define COMMAND_TOPIC "/device/esp32s3_device001/command"
#define ACK_TOPIC "/device/esp32s3_device001/ack"

static esp_mqtt_client_handle_t client = NULL;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_subscribe(client, COMMAND_TOPIC, 1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "Subscribed to topic: %s", COMMAND_TOPIC);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "Received command: Topic=%.*s Data=%.*s",
                 event->topic_len, event->topic, event->data_len, event->data);
        // 这里可以进一步解析JSON命令，然后执行对应操作
        break;
    default:
        break;
    }
}

void mqtt_client_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);

    ESP_LOGI(TAG, "MQTT client started");
}

// 提供一个上报接口（定时上报调用）
void mqtt_publish_report(const char *payload)
{
    if (client != NULL)
    {
        esp_mqtt_client_publish(client, REPORT_TOPIC, payload, 0, 1, 0);
    }
}
