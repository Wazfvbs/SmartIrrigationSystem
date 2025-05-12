#include "config_manager.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "Config_Manager";
static device_config_t g_config;

#define CONFIG_NAMESPACE "device_config"

void config_manager_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    nvs_handle_t nvs_handle;
    err = nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS");
        return;
    }

    size_t required_size = sizeof(g_config);
    err = nvs_get_blob(nvs_handle, "config", &g_config, &required_size);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Loaded config from NVS");
    }
    else
    {
        ESP_LOGW(TAG, "No config found, using default values");
        strcpy(g_config.device_name, "ESP32_Device");
        strcpy(g_config.species, "Unknown");
        g_config.threshold_upper = 70.0;
        g_config.threshold_lower = 30.0;
        nvs_set_blob(nvs_handle, "config", &g_config, sizeof(g_config));
        nvs_commit(nvs_handle);
    }

    nvs_close(nvs_handle);
}

static void save_config(void)
{
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &nvs_handle));
    ESP_ERROR_CHECK(nvs_set_blob(nvs_handle, "config", &g_config, sizeof(g_config)));
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));
    nvs_close(nvs_handle);
}

void config_manager_update_device_name(const char *name)
{
    strncpy(g_config.device_name, name, sizeof(g_config.device_name) - 1);
    g_config.device_name[sizeof(g_config.device_name) - 1] = '\0';
    save_config();
    ESP_LOGI(TAG, "Updated device name to %s", g_config.device_name);
}

void config_manager_update_species(const char *species)
{
    strncpy(g_config.species, species, sizeof(g_config.species) - 1);
    g_config.species[sizeof(g_config.species) - 1] = '\0';
    save_config();
    ESP_LOGI(TAG, "Updated species to %s", g_config.species);
}

void config_manager_update_threshold(double upper, double lower)
{
    g_config.threshold_upper = upper;
    g_config.threshold_lower = lower;
    save_config();
    ESP_LOGI(TAG, "Updated thresholds to upper=%.2f, lower=%.2f", upper, lower);
}

const device_config_t *config_manager_get_config(void)
{
    return &g_config;
}
