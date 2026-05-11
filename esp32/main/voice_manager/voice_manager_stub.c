#include "voice_manager.h"

#include "esp_log.h"

static const char *TAG = "VOICE_STUB";

esp_err_t voice_manager_init(void)
{
    ESP_LOGI(TAG, "Voice pipeline disabled in this build");
    return ESP_OK;
}

esp_err_t voice_manager_start(void)
{
    return ESP_OK;
}

esp_err_t voice_manager_stop(void)
{
    return ESP_OK;
}

esp_err_t voice_manager_submit_text_command(const char *text)
{
    (void)text;
    return ESP_ERR_NOT_SUPPORTED;
}

void voice_manager_dump_status(void)
{
    ESP_LOGI(TAG, "disabled");
}
