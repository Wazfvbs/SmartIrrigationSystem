#include "stm32_protocol.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "driver/uart.h"
#include "esp_log.h"

#include "../config.h"
#include "../wifi_manager/wifi_manager.h"

static const char *TAG = "STM32_PROTO";

#define DEFAULT_PLANT_ID "1"
#define DEFAULT_MSG_TIME "1970-01-01 00:00:00"
#define STM32_TARGET_DEVICE_ID "MCU01"

static uint32_t s_trace_seq = 1;

static void get_default_device_id(char *out, size_t out_len)
{
    if (out == NULL || out_len == 0)
    {
        return;
    }
    snprintf(out, out_len, "%s", STM32_TARGET_DEVICE_ID);
}

esp_err_t stm32_protocol_get_timestamp(char *out, size_t out_len)
{
    if (out == NULL || out_len < STM32_PROTO_TIME_LEN)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (wifi_manager_get_local_time_string(out, out_len) == ESP_OK)
    {
        return ESP_OK;
    }

    time_t now = time(NULL);
    struct tm tm_info = {0};
    if (now > 0 && localtime_r(&now, &tm_info) != NULL)
    {
        if (strftime(out, out_len, "%Y-%m-%d %H:%M:%S", &tm_info) > 0)
        {
            return ESP_OK;
        }
    }

    strncpy(out, DEFAULT_MSG_TIME, out_len - 1);
    out[out_len - 1] = '\0';
    return ESP_ERR_INVALID_STATE;
}

esp_err_t stm32_protocol_build_trace_id(char *out, size_t out_len)
{
    if (out == NULL || out_len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char ts[STM32_PROTO_TIME_LEN] = {0};
    (void)stm32_protocol_get_timestamp(ts, sizeof(ts));

    int y = 1970;
    int m = 1;
    int d = 1;
    if (strlen(ts) >= 10)
    {
        int parsed = sscanf(ts, "%d-%d-%d", &y, &m, &d);
        if (parsed != 3)
        {
            y = 1970;
            m = 1;
            d = 1;
        }
    }

    uint32_t seq = s_trace_seq++;
    if (s_trace_seq > 999999U)
    {
        s_trace_seq = 1;
    }

    int n = snprintf(out, out_len, "%04d%02d%02d-%06" PRIu32, y, m, d, seq);
    if (n <= 0 || (size_t)n >= out_len)
    {
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

esp_err_t stm32_protocol_send(const char *msg_type,
                              bool require_ack,
                              const cJSON *payload,
                              const char *trace_id)
{
    if (msg_type == NULL || msg_type[0] == '\0')
    {
        return ESP_ERR_INVALID_ARG;
    }

    char ts[STM32_PROTO_TIME_LEN] = {0};
    (void)stm32_protocol_get_timestamp(ts, sizeof(ts));

    char trace[STM32_TRACE_ID_MAX_LEN] = {0};
    if (trace_id != NULL && trace_id[0] != '\0')
    {
        strncpy(trace, trace_id, sizeof(trace) - 1);
    }
    else
    {
        esp_err_t trace_ret = stm32_protocol_build_trace_id(trace, sizeof(trace));
        if (trace_ret != ESP_OK)
        {
            return trace_ret;
        }
    }

    char device_id[32] = {0};
    get_default_device_id(device_id, sizeof(device_id));

    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(root, "msg_type", msg_type);
    cJSON_AddStringToObject(root, "device_id", device_id);
    cJSON_AddStringToObject(root, "plant_id", DEFAULT_PLANT_ID);
    cJSON_AddStringToObject(root, "trace_id", trace);
    cJSON_AddStringToObject(root, "timestamp", ts);
    cJSON_AddBoolToObject(root, "require_ack", require_ack);

    cJSON *payload_node = NULL;
    if (payload != NULL && cJSON_IsObject(payload))
    {
        payload_node = cJSON_Duplicate((cJSON *)payload, 1);
    }
    else
    {
        payload_node = cJSON_CreateObject();
    }

    if (payload_node == NULL)
    {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddItemToObject(root, "payload", payload_node);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (json == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    int len = (int)strlen(json);
    int written = uart_write_bytes(STM32_UART_PORT, json, len);
    int written_nl = uart_write_bytes(STM32_UART_PORT, "\n", 1);
    cJSON_free(json);

    if (written < 0 || written_nl < 0)
    {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "TX -> STM32 msg_type=%s trace_id=%s ack=%d", msg_type, trace, require_ack);
    return ESP_OK;
}

esp_err_t stm32_protocol_send_control_command(const char *cmd,
                                              const char *action,
                                              int duration_sec,
                                              bool force,
                                              bool require_ack,
                                              const char *trace_id)
{
    cJSON *payload = cJSON_CreateObject();
    if (payload == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(payload, "cmd", (cmd && cmd[0]) ? cmd : "water_control");
    cJSON_AddStringToObject(payload, "action", (action && action[0]) ? action : "start");
    cJSON_AddNumberToObject(payload, "duration_sec", duration_sec);
    cJSON_AddBoolToObject(payload, "force", force);

    esp_err_t ret = stm32_protocol_send("control_command", require_ack, payload, trace_id);
    cJSON_Delete(payload);
    return ret;
}

esp_err_t stm32_protocol_send_mode_command(const char *mode,
                                           const char *reason,
                                           bool force,
                                           bool require_ack,
                                           const char *trace_id)
{
    cJSON *payload = cJSON_CreateObject();
    if (payload == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(payload, "mode", (mode && mode[0]) ? mode : "auto");
    cJSON_AddStringToObject(payload, "reason", (reason && reason[0]) ? reason : "esp32_mode_update");
    cJSON_AddBoolToObject(payload, "force", force);

    esp_err_t ret = stm32_protocol_send("mode_command", require_ack, payload, trace_id);
    cJSON_Delete(payload);
    return ret;
}

esp_err_t stm32_protocol_send_config_command(const cJSON *config_payload,
                                             bool require_ack,
                                             const char *trace_id)
{
    return stm32_protocol_send("config_command", require_ack, config_payload, trace_id);
}

esp_err_t stm32_protocol_send_threshold_config(double lower,
                                               double upper,
                                               bool require_ack,
                                               const char *trace_id)
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

    esp_err_t ret = stm32_protocol_send_config_command(payload, require_ack, trace_id);
    cJSON_Delete(payload);
    return ret;
}

esp_err_t stm32_protocol_send_sync_info(const char *sync_time,
                                        const char *config_version,
                                        const char *strategy_version,
                                        const char *plant_name,
                                        const char *species)
{
    char ts[STM32_PROTO_TIME_LEN] = {0};
    if (sync_time != NULL && sync_time[0] != '\0')
    {
        strncpy(ts, sync_time, sizeof(ts) - 1);
    }
    else
    {
        (void)stm32_protocol_get_timestamp(ts, sizeof(ts));
    }

    cJSON *payload = cJSON_CreateObject();
    if (payload == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(payload, "sync_time", ts);
    cJSON_AddStringToObject(payload, "config_version", (config_version && config_version[0]) ? config_version : "cfg_v1.0");
    cJSON_AddStringToObject(payload, "strategy_version", (strategy_version && strategy_version[0]) ? strategy_version : "strategy_v1.0");

    cJSON *profile = cJSON_CreateObject();
    if (profile != NULL)
    {
        cJSON_AddStringToObject(profile, "plant_name", (plant_name && plant_name[0]) ? plant_name : "plant_1");
        cJSON_AddStringToObject(profile, "species", (species && species[0]) ? species : "unknown");
        cJSON_AddItemToObject(payload, "plant_profile", profile);
    }

    esp_err_t ret = stm32_protocol_send("sync_info", true, payload, NULL);
    cJSON_Delete(payload);
    return ret;
}

esp_err_t stm32_protocol_send_command_context(const char *command_source,
                                              const char *priority,
                                              bool allow_override,
                                              bool require_ack,
                                              const char *operator_id,
                                              const char *remark,
                                              const char *trace_id)
{
    cJSON *payload = cJSON_CreateObject();
    if (payload == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(payload, "command_source", (command_source && command_source[0]) ? command_source : "web");
    cJSON_AddStringToObject(payload, "priority", (priority && priority[0]) ? priority : "normal");
    cJSON_AddBoolToObject(payload, "allow_override", allow_override);
    cJSON_AddBoolToObject(payload, "require_ack", require_ack);
    cJSON_AddStringToObject(payload, "operator_id", (operator_id && operator_id[0]) ? operator_id : "unknown");
    cJSON_AddStringToObject(payload, "remark", (remark && remark[0]) ? remark : "none");

    esp_err_t ret = stm32_protocol_send("command_context", false, payload, trace_id);
    cJSON_Delete(payload);
    return ret;
}

esp_err_t stm32_protocol_send_menu_command(const char *menu_action,
                                           const char *menu_page,
                                           int main_index,
                                           int sub_index,
                                           bool threshold_editing,
                                           bool require_ack,
                                           const char *trace_id)
{
    cJSON *payload = cJSON_CreateObject();
    if (payload == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(payload, "menu_action", (menu_action && menu_action[0]) ? menu_action : "state_sync");
    cJSON_AddStringToObject(payload, "menu_page", (menu_page && menu_page[0]) ? menu_page : "page_unknown");
    cJSON_AddNumberToObject(payload, "main_index", main_index);
    cJSON_AddNumberToObject(payload, "sub_index", sub_index);
    cJSON_AddBoolToObject(payload, "threshold_editing", threshold_editing);

    esp_err_t ret = stm32_protocol_send("menu_command", require_ack, payload, trace_id);
    cJSON_Delete(payload);
    return ret;
}
