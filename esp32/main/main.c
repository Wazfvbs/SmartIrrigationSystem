#include <stdio.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl.h"

#include "app_mqtt/app_mqtt.h"
#include "config_manager/config_manager.h"
#include "display_manager/display_manager.h"
#include "lv_port/lv_port_disp.h"
#include "uart_receiver/uart_receiver.h"
#include "ui/history_ui.h"
#include "ui/ui.h"
#include "upload_manager/upload_manager.h"
#include "wifi_manager/wifi_manager.h"

static const char *TAG = "APP";
static QueueHandle_t s_button_evt_queue = NULL;

extern void uart_receiver_register_sensor_callback(sensor_data_cb_t cb);

static void my_sensor_handler(const sensor_data_t *sd)
{
    ESP_LOGI(TAG,
             "SENSOR CALLBACK: T=%.1f H=%.1f S=%.1f L=%lu W=%s",
             sd->temp,
             sd->humidity,
             sd->soil,
             (unsigned long)sd->light,
             sd->water);
}

static void my_button_handler(button_event_t event)
{
    if (s_button_evt_queue == NULL)
    {
        return;
    }

    if (xQueueSend(s_button_evt_queue, &event, 0) != pdPASS)
    {
        button_event_t drop = BUTTON_SHORT_PRESS;
        (void)xQueueReceive(s_button_evt_queue, &drop, 0);
        (void)xQueueSend(s_button_evt_queue, &event, 0);
    }
}

static void lv_tick_task(void *arg)
{
    (void)arg;
    lv_tick_inc(1);
}

static void network_supervisor_task(void *arg)
{
    (void)arg;

    TickType_t last_dump_tick = xTaskGetTickCount();
    bool last_wifi_connected = false;

    while (1)
    {
        bool wifi_connected = wifi_manager_is_connected();

        if (wifi_connected && !mqtt_client_is_started())
        {
            esp_err_t ret = mqtt_client_start();
            if (ret != ESP_OK)
            {
                ESP_LOGW(TAG, "mqtt_client_start failed: %s", esp_err_to_name(ret));
            }
        }
        else if (!wifi_connected && mqtt_client_is_started())
        {
            esp_err_t ret = mqtt_client_stop();
            if (ret != ESP_OK)
            {
                ESP_LOGW(TAG, "mqtt_client_stop failed: %s", esp_err_to_name(ret));
            }
        }

        if (wifi_connected != last_wifi_connected)
        {
            ESP_LOGI(TAG, "Wi-Fi state changed: %s", wifi_connected ? "connected" : "disconnected");
            last_wifi_connected = wifi_connected;
        }

        TickType_t now = xTaskGetTickCount();
        if ((now - last_dump_tick) >= pdMS_TO_TICKS(30000))
        {
            wifi_manager_dump_status();
            mqtt_client_dump_status();
            last_dump_tick = now;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void)
{
    config_manager_init();
    ESP_ERROR_CHECK(mqtt_client_init());
    ESP_ERROR_CHECK(wifi_manager_init());

    s_button_evt_queue = xQueueCreate(8, sizeof(button_event_t));
    if (s_button_evt_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create button event queue");
    }

    uart_receiver_init();
    esp_err_t threshold_sync_ret = wifi_manager_request_threshold_sync("esp_boot");
    if (threshold_sync_ret != ESP_OK)
    {
        ESP_LOGW(TAG, "Initial threshold sync request failed: %s", esp_err_to_name(threshold_sync_ret));
    }
    upload_manager_start();
    uart_receiver_register_sensor_callback(my_sensor_handler);
    uart_receiver_register_button_callback(my_button_handler);

    lv_init();

    const esp_timer_create_args_t tick_args = {
        .callback = lv_tick_task,
        .name = "lv_tick"};
    esp_timer_handle_t tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, 1000));

    display_manager_init();

    xTaskCreate(network_supervisor_task, "net_supervisor", 4096, NULL, 9, NULL);

    while (1)
    {
        const sensor_data_t *data = uart_receiver_get_latest_data();
        if (s_button_evt_queue != NULL)
        {
            button_event_t evt = BUTTON_SHORT_PRESS;
            while (xQueueReceive(s_button_evt_queue, &evt, 0) == pdTRUE)
            {
                ui_handle_button_event(evt);
            }
        }
        display_manager_show_env(data);
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
