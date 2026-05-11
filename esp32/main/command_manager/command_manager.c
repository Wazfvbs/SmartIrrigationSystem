#include "../command_manager/command_manager.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

#include "app_mqtt/app_mqtt.h"
#include "cJSON.h"
#include "config_manager/config_manager.h"
#include "stm32_protocol/stm32_protocol.h"
#include "voice_manager/voice_manager.h"
#include "wifi_manager/wifi_manager.h"

static const char *TAG = "Command_Manager";

static const char *json_get_string(const cJSON *obj, const char *key, const char *default_value)
{
    if (obj == NULL || key == NULL)
    {
        return default_value;
    }
    cJSON *item = cJSON_GetObjectItem((cJSON *)obj, key);
    if (cJSON_IsString(item) && item->valuestring != NULL)
    {
        return item->valuestring;
    }
    return default_value;
}

static int json_get_int(const cJSON *obj, const char *key, int default_value)
{
    if (obj == NULL || key == NULL)
    {
        return default_value;
    }
    cJSON *item = cJSON_GetObjectItem((cJSON *)obj, key);
    if (cJSON_IsNumber(item))
    {
        return item->valueint;
    }
    return default_value;
}

static bool json_get_bool(const cJSON *obj, const char *key, bool default_value)
{
    if (obj == NULL || key == NULL)
    {
        return default_value;
    }
    cJSON *item = cJSON_GetObjectItem((cJSON *)obj, key);
    if (cJSON_IsBool(item))
    {
        return cJSON_IsTrue(item);
    }
    return default_value;
}

static bool json_get_double(const cJSON *obj, const char *key, double *out)
{
    if (obj == NULL || key == NULL || out == NULL)
    {
        return false;
    }
    cJSON *item = cJSON_GetObjectItem((cJSON *)obj, key);
    if (cJSON_IsNumber(item))
    {
        *out = item->valuedouble;
        return true;
    }
    return false;
}

static esp_err_t send_threshold_config_to_stm32(double lower, double upper, const char *trace_id)
{
    cJSON *payload = cJSON_CreateObject();
    if (payload == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(payload, "config_type", "threshold");
    cJSON *config = cJSON_AddObjectToObject(payload, "config");
    if (config == NULL)
    {
        cJSON_Delete(payload);
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddNumberToObject(config, "soil_moisture_min", lower);
    cJSON_AddNumberToObject(config, "soil_moisture_max", upper);

    esp_err_t ret = stm32_protocol_send_config_command(payload, true, trace_id);
    cJSON_Delete(payload);
    return ret;
}

static void handle_standard_command(const cJSON *root, const char *msg_type)
{
    cJSON *payload = cJSON_GetObjectItem((cJSON *)root, "payload");
    if (!cJSON_IsObject(payload))
    {
        payload = NULL;
    }

    const char *trace_id = json_get_string(root, "trace_id", NULL);
    bool require_ack = json_get_bool(root, "require_ack", true);

    if (strcmp(msg_type, "remote_control_command") == 0)
    {
        const char *cmd = json_get_string(payload, "cmd", "water_control");
        const char *action = json_get_string(payload, "action", "start");
        int duration_sec = json_get_int(payload, "duration_sec", 10);
        bool force = json_get_bool(payload, "force", false);

        esp_err_t ret = stm32_protocol_send_control_command(cmd, action, duration_sec, force, require_ack, trace_id);
        ESP_LOGI(TAG, "remote_control_command -> control_command result=%s", esp_err_to_name(ret));

        const char *operator_id = json_get_string(payload, "operator_id", "web_user");
        (void)stm32_protocol_send_command_context("web", "normal", true, require_ack, operator_id, "remote_control", trace_id);
        return;
    }

    if (strcmp(msg_type, "remote_config_command") == 0)
    {
        esp_err_t ret = stm32_protocol_send_config_command(payload, require_ack, trace_id);
        ESP_LOGI(TAG, "remote_config_command -> config_command result=%s", esp_err_to_name(ret));

        cJSON *cfg = payload ? cJSON_GetObjectItem(payload, "config") : NULL;
        if (cJSON_IsObject(cfg))
        {
            double lower = 0.0;
            double upper = 0.0;
            if (json_get_double(cfg, "soil_moisture_min", &lower) && json_get_double(cfg, "soil_moisture_max", &upper))
            {
                config_manager_update_threshold(upper, lower);
            }
        }

        const char *operator_id = json_get_string(payload, "operator_id", "web_user");
        (void)stm32_protocol_send_command_context("web", "normal", true, require_ack, operator_id, "remote_config", trace_id);
        return;
    }

    if (strcmp(msg_type, "network_config_command") == 0)
    {
        const char *ssid = json_get_string(payload, "ssid", NULL);
        const char *password = json_get_string(payload, "password", NULL);
        if (ssid != NULL && password != NULL)
        {
            esp_err_t ret = wifi_manager_set_credentials(ssid, password, true);
            ESP_LOGI(TAG, "network_config_command wifi result=%s", esp_err_to_name(ret));
        }

        const char *server_host = json_get_string(payload, "server_host", NULL);
        int server_port = json_get_int(payload, "server_port", 443);
        if (server_host != NULL)
        {
            char base_url[128];
            if (server_port > 0 && server_port != 443)
            {
                snprintf(base_url, sizeof(base_url), "https://%s:%d", server_host, server_port);
            }
            else
            {
                snprintf(base_url, sizeof(base_url), "https://%s", server_host);
            }
            esp_err_t ret = mqtt_client_update_config(base_url, "MCU01", true);
            ESP_LOGI(TAG, "network_config_command cloud result=%s", esp_err_to_name(ret));
        }

        const char *auth_token = json_get_string(payload, "auth_token",
                                                 json_get_string(payload, "authorization", NULL));
        if (auth_token != NULL)
        {
            esp_err_t ret = mqtt_client_update_auth_token(auth_token, true);
            ESP_LOGI(TAG, "network_config_command auth token result=%s", esp_err_to_name(ret));
        }
        return;
    }

    if (strcmp(msg_type, "maintenance_command") == 0)
    {
        const char *cmd = json_get_string(payload, "cmd", "self_check");
        esp_err_t ret = stm32_protocol_send_control_command(cmd, "execute", 0, true, require_ack, trace_id);
        ESP_LOGI(TAG, "maintenance_command -> control_command result=%s", esp_err_to_name(ret));
        return;
    }

    if (strcmp(msg_type, "binding_info_command") == 0)
    {
        const char *display_name = json_get_string(payload, "display_name", NULL);
        const char *species = json_get_string(payload, "species", NULL);
        if (display_name != NULL)
        {
            config_manager_update_device_name(display_name);
        }
        if (species != NULL)
        {
            config_manager_update_species(species);
        }
        return;
    }

    if (strcmp(msg_type, "voice_text_command") == 0)
    {
        const char *text = json_get_string(payload, "text", NULL);
        if (text != NULL)
        {
            esp_err_t ret = voice_manager_submit_text_command(text);
            ESP_LOGI(TAG, "voice_text_command submit result=%s text=\"%s\"", esp_err_to_name(ret), text);
        }
        else
        {
            ESP_LOGW(TAG, "voice_text_command missing payload.text");
        }
        return;
    }

    if (strcmp(msg_type, "control_command") == 0 ||
        strcmp(msg_type, "mode_command") == 0 ||
        strcmp(msg_type, "config_command") == 0 ||
        strcmp(msg_type, "sync_info") == 0 ||
        strcmp(msg_type, "command_context") == 0)
    {
        esp_err_t ret = stm32_protocol_send(msg_type, require_ack, payload, trace_id);
        ESP_LOGI(TAG, "forward %s to STM32 result=%s", msg_type, esp_err_to_name(ret));
        return;
    }

    ESP_LOGW(TAG, "Unsupported msg_type: %s", msg_type);
}

static void handle_legacy_command(const cJSON *root)
{
    const char *command = json_get_string(root, "command", NULL);
    if (command == NULL)
    {
        ESP_LOGE(TAG, "Invalid command format");
        return;
    }

    cJSON *payload = cJSON_GetObjectItem((cJSON *)root, "payload");
    if (!cJSON_IsObject(payload))
    {
        payload = NULL;
    }

    ESP_LOGI(TAG, "Parsed legacy command: %s", command);

    if (strcmp(command, "water_on") == 0)
    {
        int duration_sec = json_get_int(payload, "duration_sec", 10);
        esp_err_t ret = stm32_protocol_send_control_command("water_control", "start", duration_sec, false, true, NULL);
        ESP_LOGI(TAG, "water_on -> control_command result=%s", esp_err_to_name(ret));
    }
    else if (strcmp(command, "water_off") == 0)
    {
        esp_err_t ret = stm32_protocol_send_control_command("water_control", "stop", 0, false, true, NULL);
        ESP_LOGI(TAG, "water_off -> control_command result=%s", esp_err_to_name(ret));
    }
    else if (strcmp(command, "led_on") == 0)
    {
        esp_err_t ret = stm32_protocol_send_control_command("fill_light_control", "start", 0, false, true, NULL);
        ESP_LOGI(TAG, "led_on -> control_command result=%s", esp_err_to_name(ret));
    }
    else if (strcmp(command, "led_off") == 0)
    {
        esp_err_t ret = stm32_protocol_send_control_command("fill_light_control", "stop", 0, false, true, NULL);
        ESP_LOGI(TAG, "led_off -> control_command result=%s", esp_err_to_name(ret));
    }
    else if (strcmp(command, "set_mode") == 0)
    {
        const char *mode = json_get_string(payload, "mode", "auto");
        const char *reason = json_get_string(payload, "reason", "legacy_set_mode");
        bool force = json_get_bool(payload, "force", true);
        esp_err_t ret = stm32_protocol_send_mode_command(mode, reason, force, true, NULL);
        ESP_LOGI(TAG, "set_mode -> mode_command result=%s", esp_err_to_name(ret));
    }
    else if (strcmp(command, "update_threshold") == 0)
    {
        double upper = 70.0;
        double lower = 30.0;
        bool ok_upper = json_get_double(payload, "upper", &upper);
        bool ok_lower = json_get_double(payload, "lower", &lower);
        if (ok_upper && ok_lower)
        {
            config_manager_update_threshold(upper, lower);
            esp_err_t ret = send_threshold_config_to_stm32(lower, upper, NULL);
            ESP_LOGI(TAG, "update_threshold -> config_command result=%s", esp_err_to_name(ret));
        }
        else
        {
            ESP_LOGW(TAG, "update_threshold payload invalid");
        }
    }
    else if (strcmp(command, "rename_device") == 0)
    {
        const char *name = json_get_string(payload, "name", json_get_string(root, "payload", NULL));
        if (name != NULL)
        {
            config_manager_update_device_name(name);
        }
    }
    else if (strcmp(command, "bind_species") == 0)
    {
        const char *species = json_get_string(payload, "species", json_get_string(root, "payload", NULL));
        if (species != NULL)
        {
            config_manager_update_species(species);
        }
    }
    else if (strcmp(command, "set_wifi_credentials") == 0)
    {
        const char *ssid = json_get_string(payload, "ssid", NULL);
        const char *password = json_get_string(payload, "password", NULL);
        if (ssid != NULL && password != NULL)
        {
            esp_err_t ret = wifi_manager_set_credentials(ssid, password, true);
            ESP_LOGI(TAG, "set_wifi_credentials result=%s", esp_err_to_name(ret));
        }
        else
        {
            ESP_LOGW(TAG, "set_wifi_credentials payload invalid");
        }
    }
    else if (strcmp(command, "set_mqtt_config") == 0 || strcmp(command, "set_http_config") == 0)
    {
        const char *base_url = json_get_string(payload, "base_url", json_get_string(payload, "broker_uri", NULL));
        const char *device_id = json_get_string(payload, "device_id", NULL);
        if (base_url != NULL || device_id != NULL)
        {
            cloud_http_status_t cloud = {0};
            (void)mqtt_client_get_status(&cloud);

            const char *effective_base_url = (base_url != NULL) ? base_url : cloud.base_url;
            const char *effective_device_id = (device_id != NULL) ? device_id : cloud.device_id;
            esp_err_t ret = mqtt_client_update_config(effective_base_url, effective_device_id, true);
            ESP_LOGI(TAG, "set_mqtt_config result=%s", esp_err_to_name(ret));
        }

        const char *auth_token = json_get_string(payload, "auth_token",
                                                 json_get_string(payload, "authorization", NULL));
        if (auth_token != NULL)
        {
            esp_err_t ret = mqtt_client_update_auth_token(auth_token, true);
            ESP_LOGI(TAG, "set_mqtt_config auth token result=%s", esp_err_to_name(ret));
        }

        if (base_url == NULL && device_id == NULL && auth_token == NULL)
        {
            ESP_LOGW(TAG, "set_mqtt_config payload invalid");
        }
    }
    else if (strcmp(command, "start_provisioning") == 0)
    {
        esp_err_t ret = wifi_manager_start_provisioning();
        ESP_LOGI(TAG, "start_provisioning result=%s", esp_err_to_name(ret));
    }
    else if (strcmp(command, "net_dump") == 0)
    {
        wifi_manager_dump_status();
        mqtt_client_dump_status();
    }
    else if (strcmp(command, "clear_wifi_credentials") == 0)
    {
        esp_err_t ret = wifi_manager_clear_credentials();
        ESP_LOGI(TAG, "clear_wifi_credentials result=%s", esp_err_to_name(ret));
    }
    else if (strcmp(command, "voice_text") == 0)
    {
        const char *text = json_get_string(payload, "text", json_get_string(root, "payload", NULL));
        if (text != NULL)
        {
            esp_err_t ret = voice_manager_submit_text_command(text);
            ESP_LOGI(TAG, "voice_text submit result=%s text=\"%s\"", esp_err_to_name(ret), text);
        }
        else
        {
            ESP_LOGW(TAG, "voice_text payload invalid");
        }
    }
    else
    {
        ESP_LOGW(TAG, "Unknown legacy command: %s", command);
    }
}

void command_manager_handle(const char *topic, const char *data, int data_len)
{
    (void)topic;

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

    const char *msg_type = json_get_string(root, "msg_type", NULL);
    if (msg_type != NULL)
    {
        handle_standard_command(root, msg_type);
    }
    else
    {
        handle_legacy_command(root);
    }

    cJSON_Delete(root);
}
