#include "uart_receiver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include <driver/uart.h>
#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "../config.h"
#include "../upload_manager/upload_manager.h"
#include "../wifi_manager/wifi_manager.h"
#include "cJSON.h"

static const char *TAG = "UART_RX";

#define UART_BUF_SIZE 1024
#define RD_BUF_SIZE 128
#define STM_TIME_SKEW_WARN_SEC 30
#define STM_TIME_SKEW_FORCE_SEC 120
#define STM_TIME_SKEW_CONSECUTIVE 3

static sensor_data_cb_t sensor_cb = NULL;
static button_event_cb_t button_cb = NULL;

static sensor_data_t last_data;
static bool has_data = false;

static char recv_buf[UART_BUF_SIZE];
static int buf_idx = 0;
static int recv_count = 0;
static int parse_success_count = 0;
static int s_stm_skew_hit_count = 0;

typedef enum
{
    BUTTON_KEY_UNKNOWN = 0,
    BUTTON_KEY_LEFT,
    BUTTON_KEY_MIDDLE,
    BUTTON_KEY_RIGHT,
} button_key_t;

typedef enum
{
    BUTTON_PRESS_UNKNOWN = 0,
    BUTTON_PRESS_SHORT,
    BUTTON_PRESS_LONG,
    BUTTON_PRESS_DOUBLE,
    BUTTON_PRESS_REPEAT,
} button_press_kind_t;

static bool contains_token_ci(const char *text, const char *token)
{
    if (text == NULL || token == NULL)
    {
        return false;
    }

    size_t text_len = strlen(text);
    size_t token_len = strlen(token);
    if (token_len == 0 || token_len > text_len)
    {
        return false;
    }

    for (size_t i = 0; i <= text_len - token_len; ++i)
    {
        bool match = true;
        for (size_t j = 0; j < token_len; ++j)
        {
            int a = tolower((unsigned char)text[i + j]);
            int b = tolower((unsigned char)token[j]);
            if (a != b)
            {
                match = false;
                break;
            }
        }
        if (match)
        {
            return true;
        }
    }

    return false;
}

static bool streq_ci(const char *a, const char *b)
{
    if (a == NULL || b == NULL)
    {
        return false;
    }

    while (*a != '\0' && *b != '\0')
    {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
        {
            return false;
        }
        ++a;
        ++b;
    }
    return (*a == '\0' && *b == '\0');
}

static button_key_t parse_button_key_name(const char *name)
{
    if (name == NULL || name[0] == '\0')
    {
        return BUTTON_KEY_UNKNOWN;
    }

    // New protocol: key_up / up_button means "left/up" navigation.
    if (contains_token_ci(name, "left") ||
        streq_ci(name, "l") ||
        streq_ci(name, "key_up") ||
        streq_ci(name, "up_button") ||
        streq_ci(name, "up") ||
        contains_token_ci(name, "prev"))
    {
        return BUTTON_KEY_LEFT;
    }

    // New protocol: key_down / down_button means "right/down" navigation.
    if (contains_token_ci(name, "right") ||
        streq_ci(name, "r") ||
        streq_ci(name, "key_down") ||
        streq_ci(name, "down_button") ||
        streq_ci(name, "down") ||
        contains_token_ci(name, "next"))
    {
        return BUTTON_KEY_RIGHT;
    }

    // New protocol: key_ok / ok_button means center/confirm key.
    if (contains_token_ci(name, "middle") ||
        contains_token_ci(name, "center") ||
        contains_token_ci(name, "mid") ||
        streq_ci(name, "m") ||
        streq_ci(name, "c") ||
        streq_ci(name, "key_ok") ||
        streq_ci(name, "ok_button") ||
        streq_ci(name, "ok") ||
        streq_ci(name, "confirm") ||
        streq_ci(name, "enter"))
    {
        return BUTTON_KEY_MIDDLE;
    }

    return BUTTON_KEY_UNKNOWN;
}

static button_key_t parse_button_key_from_payload(const cJSON *payload)
{
    if (!cJSON_IsObject(payload))
    {
        return BUTTON_KEY_UNKNOWN;
    }

    const char *key_names[] = {"key", "key_id", "key_name", "button", "button_id", "position"};
    for (size_t i = 0; i < sizeof(key_names) / sizeof(key_names[0]); ++i)
    {
        cJSON *item = cJSON_GetObjectItem((cJSON *)payload, key_names[i]);
        if (cJSON_IsString(item) && item->valuestring != NULL)
        {
            button_key_t key = parse_button_key_name(item->valuestring);
            if (key != BUTTON_KEY_UNKNOWN)
            {
                return key;
            }
        }
        if (cJSON_IsNumber(item))
        {
            int id = item->valueint;
            // Support common mappings: 0/1/2 and partial 1/2/3 fallback.
            if (id == 0)
            {
                return BUTTON_KEY_LEFT;
            }
            if (id == 1)
            {
                return BUTTON_KEY_MIDDLE;
            }
            if (id == 2 || id == 3)
            {
                return BUTTON_KEY_RIGHT;
            }
        }
    }

    return BUTTON_KEY_UNKNOWN;
}

static button_press_kind_t parse_press_kind(const char *event_type)
{
    if (event_type == NULL || event_type[0] == '\0')
    {
        return BUTTON_PRESS_UNKNOWN;
    }

    if (contains_token_ci(event_type, "double"))
    {
        return BUTTON_PRESS_DOUBLE;
    }
    if (contains_token_ci(event_type, "repeat"))
    {
        return BUTTON_PRESS_REPEAT;
    }
    if (contains_token_ci(event_type, "long"))
    {
        return BUTTON_PRESS_LONG;
    }
    if (contains_token_ci(event_type, "none"))
    {
        return BUTTON_PRESS_UNKNOWN;
    }
    if (contains_token_ci(event_type, "short") ||
        contains_token_ci(event_type, "press") ||
        contains_token_ci(event_type, "release") ||
        contains_token_ci(event_type, "click") ||
        contains_token_ci(event_type, "tap"))
    {
        return BUTTON_PRESS_SHORT;
    }

    return BUTTON_PRESS_UNKNOWN;
}

static void normalize_cloud_timestamp(char *ts)
{
    if (ts == NULL || ts[0] == '\0')
    {
        return;
    }

    char *space = strchr(ts, ' ');
    if (space != NULL)
    {
        *space = 'T';
    }
}

static void fill_upload_timestamp(const sensor_data_t *sd, char *out, size_t out_len)
{
    if (out == NULL || out_len == 0)
    {
        return;
    }

    if (wifi_manager_get_local_time_string(out, out_len) == ESP_OK)
    {
        normalize_cloud_timestamp(out);
        return;
    }

    if (sd != NULL && sd->timestamp[0] != '\0')
    {
        strncpy(out, sd->timestamp, out_len - 1);
        out[out_len - 1] = '\0';
        normalize_cloud_timestamp(out);
        return;
    }

    strncpy(out, "1970-01-01T00:00:00", out_len - 1);
    out[out_len - 1] = '\0';
}

static bool parse_water_level_text(const char *water_text, float *out_level)
{
    if (water_text == NULL || water_text[0] == '\0' || out_level == NULL)
    {
        return false;
    }

    char *end = NULL;
    float val = strtof(water_text, &end);
    if (end == water_text)
    {
        return false;
    }
    *out_level = val;
    return true;
}

static bool parse_battery_item(cJSON *item, uint8_t *out)
{
    if (item == NULL || out == NULL)
    {
        return false;
    }

    int batt = -1;
    if (cJSON_IsNumber(item))
    {
        batt = item->valueint;
    }
    else if (cJSON_IsString(item))
    {
        char *end = NULL;
        long val = strtol(item->valuestring, &end, 10);
        if (end != item->valuestring)
        {
            batt = (int)val;
        }
    }

    if (batt < 0)
    {
        return false;
    }

    if (batt > 100)
    {
        batt = 100;
    }
    *out = (uint8_t)batt;
    return true;
}

static bool parse_timestamp_to_epoch(const char *ts, time_t *out_epoch)
{
    if (ts == NULL || out_epoch == NULL)
    {
        return false;
    }

    int y = 0;
    int m = 0;
    int d = 0;
    int hh = 0;
    int mm = 0;
    int ss = 0;
    if (sscanf(ts, "%d-%d-%d %d:%d:%d", &y, &m, &d, &hh, &mm, &ss) != 6)
    {
        return false;
    }
    if (y < 2000 || m < 1 || m > 12 || d < 1 || d > 31 ||
        hh < 0 || hh > 23 || mm < 0 || mm > 59 || ss < 0 || ss > 59)
    {
        return false;
    }

    struct tm tm_info = {0};
    tm_info.tm_year = y - 1900;
    tm_info.tm_mon = m - 1;
    tm_info.tm_mday = d;
    tm_info.tm_hour = hh;
    tm_info.tm_min = mm;
    tm_info.tm_sec = ss;
    tm_info.tm_isdst = -1;

    time_t epoch = mktime(&tm_info);
    if (epoch <= 0)
    {
        return false;
    }

    *out_epoch = epoch;
    return true;
}

static void maybe_request_time_sync_by_stm_skew(const sensor_data_t *sd)
{
    if (sd == NULL || sd->timestamp[0] == '\0')
    {
        return;
    }
    if (!wifi_manager_time_is_synced())
    {
        return;
    }

    time_t stm_epoch = 0;
    if (!parse_timestamp_to_epoch(sd->timestamp, &stm_epoch))
    {
        return;
    }

    time_t esp_epoch = time(NULL);
    if (esp_epoch < 1700000000)
    {
        return;
    }

    long diff_sec = labs((long)(esp_epoch - stm_epoch));
    if (diff_sec <= STM_TIME_SKEW_WARN_SEC)
    {
        s_stm_skew_hit_count = 0;
        return;
    }

    s_stm_skew_hit_count++;

    bool should_sync = false;
    if (diff_sec > STM_TIME_SKEW_FORCE_SEC)
    {
        should_sync = true;
    }
    else if (s_stm_skew_hit_count >= STM_TIME_SKEW_CONSECUTIVE)
    {
        should_sync = true;
    }

    ESP_LOGW(TAG,
             "STM/ESP time skew=%ld sec (hits=%d, ts=%s)",
             diff_sec,
             s_stm_skew_hit_count,
             sd->timestamp);

    if (!should_sync)
    {
        return;
    }

    esp_err_t ret = wifi_manager_request_time_sync("stm32_timestamp_skew");
    if (ret == ESP_OK)
    {
        s_stm_skew_hit_count = 0;
    }
    else
    {
        ESP_LOGW(TAG, "Failed to request time sync by skew: %s", esp_err_to_name(ret));
    }
}

static bool build_upload_json(const sensor_data_t *sd, char *out, size_t out_len)
{
    if (sd == NULL || out == NULL || out_len == 0)
    {
        return false;
    }

    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        return false;
    }

    char ts[20] = {0};
    fill_upload_timestamp(sd, ts, sizeof(ts));

    float water_level = 0.0f;
    bool has_water_level = sd->water_level_valid;
    if (has_water_level)
    {
        water_level = sd->water_level;
    }
    else
    {
        has_water_level = parse_water_level_text(sd->water, &water_level);
    }

    cJSON_AddStringToObject(root, "plant_id", "1");
    cJSON_AddStringToObject(root, "timestamp", ts);
    cJSON_AddNumberToObject(root, "temperature", sd->temp);
    cJSON_AddNumberToObject(root, "humidity", sd->humidity);
    cJSON_AddNumberToObject(root, "soil_moisture", sd->soil);
    cJSON_AddNumberToObject(root, "light", (double)sd->light);
    cJSON_AddNumberToObject(root, "water_level", has_water_level ? (double)water_level : 0.0);
    cJSON_AddNumberToObject(root, "battery", (double)sd->battery);
    cJSON_AddStringToObject(root, "mode", "auto");

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (json == NULL)
    {
        return false;
    }

    size_t n = strlen(json);
    if (n + 1 > out_len)
    {
        cJSON_free(json);
        return false;
    }

    memcpy(out, json, n + 1);
    cJSON_free(json);
    return true;
}

static void publish_sensor_data(const sensor_data_t *sd, const char *raw_frame)
{
    if (sd == NULL)
    {
        return;
    }

    last_data = *sd;
    has_data = true;
    maybe_request_time_sync_by_stm_skew(sd);

    if (sensor_cb != NULL)
    {
        sensor_cb(sd);
    }

    char upload_json[512] = {0};
    if (build_upload_json(sd, upload_json, sizeof(upload_json)))
    {
        upload_manager_update_data(upload_json);
    }
    else if (raw_frame != NULL && raw_frame[0] != '\0')
    {
        ESP_LOGW(TAG, "Failed to build normalized upload JSON, fallback to raw frame");
        upload_manager_update_data(raw_frame);
    }
}

static const char *button_event_to_str(button_event_t evt)
{
    switch (evt)
    {
    case BUTTON_SHORT_PRESS:
        return "short_press(legacy)";
    case BUTTON_DOUBLE_CLICK:
        return "double_click(legacy)";
    case BUTTON_LONG_PRESS:
        return "long_press(legacy)";
    case BUTTON_LEFT_SHORT_PRESS:
        return "left_short_press";
    case BUTTON_MIDDLE_SHORT_PRESS:
        return "middle_short_press";
    case BUTTON_RIGHT_SHORT_PRESS:
        return "right_short_press";
    case BUTTON_LEFT_LONG_PRESS:
        return "left_long_press";
    case BUTTON_MIDDLE_LONG_PRESS:
        return "middle_long_press";
    case BUTTON_RIGHT_LONG_PRESS:
        return "right_long_press";
    default:
        return "unknown";
    }
}

static bool map_button_event_type(const cJSON *payload, const char *event_type, button_event_t *out)
{
    if (out == NULL)
    {
        return false;
    }

    button_press_kind_t kind = parse_press_kind(event_type);
    button_key_t key = parse_button_key_name(event_type);
    if (key == BUTTON_KEY_UNKNOWN)
    {
        key = parse_button_key_from_payload(payload);
    }

    if (kind == BUTTON_PRESS_UNKNOWN && event_type != NULL)
    {
        // If key name exists but press type is omitted, default to short press.
        if (key != BUTTON_KEY_UNKNOWN)
        {
            kind = BUTTON_PRESS_SHORT;
        }
        else if (strcmp(event_type, "double_click") == 0)
        {
            kind = BUTTON_PRESS_DOUBLE;
        }
        else if (strcmp(event_type, "long_press") == 0)
        {
            kind = BUTTON_PRESS_LONG;
        }
        else if (strcmp(event_type, "short_press") == 0 ||
                 strcmp(event_type, "press") == 0 ||
                 strcmp(event_type, "release") == 0)
        {
            kind = BUTTON_PRESS_SHORT;
        }
    }

    if (key == BUTTON_KEY_LEFT)
    {
        *out = (kind == BUTTON_PRESS_LONG) ? BUTTON_LEFT_LONG_PRESS : BUTTON_LEFT_SHORT_PRESS;
        return true;
    }
    if (key == BUTTON_KEY_MIDDLE)
    {
        *out = (kind == BUTTON_PRESS_LONG) ? BUTTON_MIDDLE_LONG_PRESS : BUTTON_MIDDLE_SHORT_PRESS;
        return true;
    }
    if (key == BUTTON_KEY_RIGHT)
    {
        *out = (kind == BUTTON_PRESS_LONG) ? BUTTON_RIGHT_LONG_PRESS : BUTTON_RIGHT_SHORT_PRESS;
        return true;
    }

    if (kind == BUTTON_PRESS_DOUBLE)
    {
        *out = BUTTON_DOUBLE_CLICK;
        return true;
    }
    if (kind == BUTTON_PRESS_LONG)
    {
        *out = BUTTON_LONG_PRESS;
        return true;
    }
    if (kind == BUTTON_PRESS_SHORT)
    {
        *out = BUTTON_SHORT_PRESS;
        return true;
    }
    if (kind == BUTTON_PRESS_REPEAT)
    {
        *out = BUTTON_SHORT_PRESS;
        return true;
    }

    return false;
}

static bool parse_wrapped_telemetry(cJSON *root, sensor_data_t *out)
{
    if (!cJSON_IsObject(root) || out == NULL)
    {
        return false;
    }

    cJSON *payload = cJSON_GetObjectItem(root, "payload");
    if (!cJSON_IsObject(payload))
    {
        return false;
    }

    cJSON *temp = cJSON_GetObjectItem(payload, "temperature");
    cJSON *humidity = cJSON_GetObjectItem(payload, "humidity");
    cJSON *soil = cJSON_GetObjectItem(payload, "soil_moisture");
    cJSON *light = cJSON_GetObjectItem(payload, "light");
    if (!cJSON_IsNumber(temp) || !cJSON_IsNumber(humidity) || !cJSON_IsNumber(soil) || !cJSON_IsNumber(light))
    {
        return false;
    }

    memset(out, 0, sizeof(*out));
    out->temp = temp->valuedouble;
    out->humidity = humidity->valuedouble;
    out->soil = soil->valuedouble;
    out->light = (uint32_t)light->valuedouble;

    cJSON *top_item = cJSON_GetObjectItem(root, "device_id");
    if (cJSON_IsString(top_item))
    {
        strncpy(out->device_id, top_item->valuestring, sizeof(out->device_id) - 1);
    }
    top_item = cJSON_GetObjectItem(root, "trace_id");
    if (cJSON_IsString(top_item))
    {
        strncpy(out->trace_id, top_item->valuestring, sizeof(out->trace_id) - 1);
    }
    top_item = cJSON_GetObjectItem(root, "timestamp");
    if (cJSON_IsString(top_item))
    {
        strncpy(out->timestamp, top_item->valuestring, sizeof(out->timestamp) - 1);
    }

    cJSON *battery = cJSON_GetObjectItem(payload, "battery");
    uint8_t batt = 0;
    if (parse_battery_item(battery, &batt))
    {
        out->battery = batt;
    }

    cJSON *water_level = cJSON_GetObjectItem(payload, "water_level");
    if (cJSON_IsNumber(water_level))
    {
        out->water_level = water_level->valuedouble;
        out->water_level_valid = true;
        snprintf(out->water, sizeof(out->water), "%.1f", out->water_level);
    }

    cJSON *water_status = cJSON_GetObjectItem(payload, "water_status");
    if (cJSON_IsString(water_status) && water_status->valuestring != NULL)
    {
        strncpy(out->water, water_status->valuestring, sizeof(out->water) - 1);
    }

    return true;
}

static bool handle_wrapped_message(cJSON *root)
{
    cJSON *msg_type_item = cJSON_GetObjectItem(root, "msg_type");
    if (!cJSON_IsString(msg_type_item) || msg_type_item->valuestring == NULL)
    {
        return false;
    }

    const char *msg_type = msg_type_item->valuestring;
    const char *trace_id = NULL;
    cJSON *trace_item = cJSON_GetObjectItem(root, "trace_id");
    if (cJSON_IsString(trace_item))
    {
        trace_id = trace_item->valuestring;
    }

    if (strcmp(msg_type, "telemetry_report") == 0)
    {
        sensor_data_t sd = {0};
        if (!parse_wrapped_telemetry(root, &sd))
        {
            ESP_LOGW(TAG, "Invalid telemetry_report payload");
            return true;
        }

        publish_sensor_data(&sd, recv_buf);
        ESP_LOGI(TAG,
                 "telemetry_report parsed trace=%s T=%.1f H=%.1f S=%.1f L=%lu",
                 trace_id ? trace_id : "none",
                 sd.temp,
                 sd.humidity,
                 sd.soil,
                 (unsigned long)sd.light);
        return true;
    }

    if (strcmp(msg_type, "boot_report") == 0)
    {
        cJSON *payload = cJSON_GetObjectItem(root, "payload");
        const char *fw_version = cJSON_IsObject(payload) ? cJSON_GetStringValue(cJSON_GetObjectItem(payload, "fw_version")) : NULL;
        ESP_LOGW(TAG,
                 "boot_report received trace=%s fw=%s, request threshold sync",
                 trace_id ? trace_id : "none",
                 fw_version ? fw_version : "unknown");

        esp_err_t ret = wifi_manager_request_threshold_sync("stm_boot_report");
        if (ret != ESP_OK)
        {
            ESP_LOGW(TAG, "threshold sync request by boot_report failed: %s", esp_err_to_name(ret));
        }
        return true;
    }

    if (strcmp(msg_type, "key_event_report") == 0)
    {
        cJSON *payload = cJSON_GetObjectItem(root, "payload");
        const char *event_type = cJSON_IsObject(payload) ? cJSON_GetStringValue(cJSON_GetObjectItem(payload, "event_type")) : NULL;
        if (event_type == NULL)
        {
            event_type = cJSON_GetStringValue(cJSON_GetObjectItem(root, "event_type"));
        }
        const char *key_id = cJSON_IsObject(payload) ? cJSON_GetStringValue(cJSON_GetObjectItem(payload, "key_id")) : NULL;
        const char *key_name = cJSON_IsObject(payload) ? cJSON_GetStringValue(cJSON_GetObjectItem(payload, "key_name")) : NULL;
        const char *key_state = cJSON_IsObject(payload) ? cJSON_GetStringValue(cJSON_GetObjectItem(payload, "key_state")) : NULL;
        const char *trigger_action = cJSON_IsObject(payload) ? cJSON_GetStringValue(cJSON_GetObjectItem(payload, "trigger_action")) : NULL;
        const char *action_result = cJSON_IsObject(payload) ? cJSON_GetStringValue(cJSON_GetObjectItem(payload, "action_result")) : NULL;
        const char *control_source = cJSON_IsObject(payload) ? cJSON_GetStringValue(cJSON_GetObjectItem(payload, "control_source")) : NULL;
        const char *page = cJSON_IsObject(payload) ? cJSON_GetStringValue(cJSON_GetObjectItem(payload, "page")) : NULL;
        const char *menu_action = cJSON_IsObject(payload) ? cJSON_GetStringValue(cJSON_GetObjectItem(payload, "menu_action")) : NULL;
        int menu_index = -1;
        int main_index = -1;
        int sub_index = -1;
        bool threshold_editing = false;
        cJSON *menu_index_item = cJSON_IsObject(payload) ? cJSON_GetObjectItem(payload, "menu_index") : NULL;
        cJSON *main_index_item = cJSON_IsObject(payload) ? cJSON_GetObjectItem(payload, "main_index") : NULL;
        cJSON *sub_index_item = cJSON_IsObject(payload) ? cJSON_GetObjectItem(payload, "sub_index") : NULL;
        cJSON *threshold_edit_item = cJSON_IsObject(payload) ? cJSON_GetObjectItem(payload, "threshold_editing") : NULL;
        if (cJSON_IsNumber(menu_index_item))
        {
            menu_index = menu_index_item->valueint;
        }
        if (cJSON_IsNumber(main_index_item))
        {
            main_index = main_index_item->valueint;
        }
        if (cJSON_IsNumber(sub_index_item))
        {
            sub_index = sub_index_item->valueint;
        }
        if (cJSON_IsBool(threshold_edit_item))
        {
            threshold_editing = cJSON_IsTrue(threshold_edit_item);
        }

        button_press_kind_t press_kind = parse_press_kind(event_type);
        if (action_result != NULL && !streq_ci(action_result, "success"))
        {
            ESP_LOGW(TAG,
                     "Ignore key_event_report trace=%s key_id=%s key_name=%s event=%s result=%s",
                     trace_id ? trace_id : "none",
                     key_id ? key_id : "null",
                     key_name ? key_name : "null",
                     event_type ? event_type : "null",
                     action_result);
            return true;
        }
        if (event_type != NULL && streq_ci(event_type, "none"))
        {
            ESP_LOGI(TAG,
                     "Ignore key_event_report(trace=%s): event_type=none key=%s/%s state=%s",
                     trace_id ? trace_id : "none",
                     key_id ? key_id : "null",
                     key_name ? key_name : "null",
                     key_state ? key_state : "null");
            return true;
        }
        // Many MCUs report short/double on both pressed+released; keep released edge to avoid duplicate trigger.
        if (key_state != NULL && streq_ci(key_state, "pressed") &&
            (press_kind == BUTTON_PRESS_SHORT || press_kind == BUTTON_PRESS_DOUBLE))
        {
            ESP_LOGI(TAG,
                     "Ignore pressed edge key_event_report trace=%s key=%s/%s event=%s",
                     trace_id ? trace_id : "none",
                     key_id ? key_id : "null",
                     key_name ? key_name : "null",
                     event_type ? event_type : "null");
            return true;
        }

        button_event_t evt = BUTTON_SHORT_PRESS;
        if (map_button_event_type(payload, event_type, &evt))
        {
            if (button_cb != NULL)
            {
                button_cb(evt);
            }
            ESP_LOGI(TAG,
                     "key_event_report trace=%s key_id=%s key_name=%s event=%s state=%s action=%s result=%s src=%s page=%s menu_action=%s menu_idx=%d main=%d sub=%d edit=%d mapped=%s",
                     trace_id ? trace_id : "none",
                     key_id ? key_id : "null",
                     key_name ? key_name : "null",
                     event_type ? event_type : "null",
                     key_state ? key_state : "null",
                     trigger_action ? trigger_action : "null",
                     action_result ? action_result : "null",
                     control_source ? control_source : "null",
                     page ? page : "null",
                     menu_action ? menu_action : "null",
                     menu_index,
                     main_index,
                     sub_index,
                     threshold_editing ? 1 : 0,
                     button_event_to_str(evt));
        }
        else
        {
            ESP_LOGW(TAG,
                     "Unknown key_event_report: key_id=%s key_name=%s event=%s state=%s",
                     key_id ? key_id : "null",
                     key_name ? key_name : "null",
                     event_type ? event_type : "null",
                     key_state ? key_state : "null");
        }
        return true;
    }

    if (strcmp(msg_type, "menu_state_report") == 0)
    {
        cJSON *payload = cJSON_GetObjectItem(root, "payload");
        const char *menu_action = cJSON_IsObject(payload) ? cJSON_GetStringValue(cJSON_GetObjectItem(payload, "menu_action")) : NULL;
        const char *menu_page = cJSON_IsObject(payload) ? cJSON_GetStringValue(cJSON_GetObjectItem(payload, "menu_page")) : NULL;
        int menu_index = -1;
        int main_index = -1;
        int sub_index = -1;
        bool threshold_editing = false;
        cJSON *menu_index_item = cJSON_IsObject(payload) ? cJSON_GetObjectItem(payload, "menu_index") : NULL;
        cJSON *main_index_item = cJSON_IsObject(payload) ? cJSON_GetObjectItem(payload, "main_index") : NULL;
        cJSON *sub_index_item = cJSON_IsObject(payload) ? cJSON_GetObjectItem(payload, "sub_index") : NULL;
        cJSON *threshold_edit_item = cJSON_IsObject(payload) ? cJSON_GetObjectItem(payload, "threshold_editing") : NULL;
        if (cJSON_IsNumber(menu_index_item))
        {
            menu_index = menu_index_item->valueint;
        }
        if (cJSON_IsNumber(main_index_item))
        {
            main_index = main_index_item->valueint;
        }
        if (cJSON_IsNumber(sub_index_item))
        {
            sub_index = sub_index_item->valueint;
        }
        if (cJSON_IsBool(threshold_edit_item))
        {
            threshold_editing = cJSON_IsTrue(threshold_edit_item);
        }

        ESP_LOGI(TAG,
                 "menu_state_report trace=%s action=%s page=%s menu_idx=%d main=%d sub=%d edit=%d",
                 trace_id ? trace_id : "none",
                 menu_action ? menu_action : "none",
                 menu_page ? menu_page : "none",
                 menu_index,
                 main_index,
                 sub_index,
                 threshold_editing ? 1 : 0);
        return true;
    }

    if (strcmp(msg_type, "config_ack_report") == 0)
    {
        cJSON *payload = cJSON_GetObjectItem(root, "payload");
        const char *ack_type = cJSON_IsObject(payload) ? cJSON_GetStringValue(cJSON_GetObjectItem(payload, "ack_type")) : NULL;
        const char *ack_status = cJSON_IsObject(payload) ? cJSON_GetStringValue(cJSON_GetObjectItem(payload, "ack_status")) : NULL;
        const char *origin_trace_id = cJSON_IsObject(payload) ? cJSON_GetStringValue(cJSON_GetObjectItem(payload, "origin_trace_id")) : NULL;
        int result_code = -1;
        cJSON *result_code_item = cJSON_IsObject(payload) ? cJSON_GetObjectItem(payload, "result_code") : NULL;
        if (cJSON_IsNumber(result_code_item))
        {
            result_code = result_code_item->valueint;
        }
        ESP_LOGI(TAG,
                 "config_ack_report trace=%s origin=%s type=%s status=%s code=%d",
                 trace_id ? trace_id : "none",
                 origin_trace_id ? origin_trace_id : "none",
                 ack_type ? ack_type : "none",
                 ack_status ? ack_status : "none",
                 result_code);

        if (ack_type != NULL && strcmp(ack_type, "sync_info") == 0)
        {
            wifi_manager_notify_stm_sync_ack(origin_trace_id ? origin_trace_id : trace_id, ack_status);
        }
        wifi_manager_notify_stm_config_ack(ack_type, origin_trace_id, ack_status, result_code);
        return true;
    }

    if (strcmp(msg_type, "alert_report") == 0)
    {
        cJSON *payload = cJSON_GetObjectItem(root, "payload");
        const char *alert_code = cJSON_IsObject(payload) ? cJSON_GetStringValue(cJSON_GetObjectItem(payload, "alert_code")) : NULL;
        const char *alert_level = cJSON_IsObject(payload) ? cJSON_GetStringValue(cJSON_GetObjectItem(payload, "alert_level")) : NULL;
        ESP_LOGW(TAG,
                 "alert_report trace=%s code=%s level=%s",
                 trace_id ? trace_id : "none",
                 alert_code ? alert_code : "none",
                 alert_level ? alert_level : "none");
        return true;
    }

    if (strcmp(msg_type, "actuator_status_report") == 0 ||
        strcmp(msg_type, "device_status_report") == 0 ||
        strcmp(msg_type, "ack") == 0)
    {
        ESP_LOGI(TAG, "Received %s trace=%s", msg_type, trace_id ? trace_id : "none");
        return true;
    }

    ESP_LOGW(TAG, "Unknown wrapped msg_type: %s", msg_type);
    return true;
}

static bool handle_legacy_flat_message(cJSON *root)
{
    cJSON *btn = cJSON_GetObjectItem(root, "button_event");
    if (btn && cJSON_IsString(btn) && button_cb != NULL)
    {
        const char ch = btn->valuestring[0];
        button_cb(ch == 'd' ? BUTTON_DOUBLE_CLICK : BUTTON_LONG_PRESS);
        ESP_LOGI(TAG, "Legacy button event: %s", btn->valuestring);
        return true;
    }

    cJSON *tmp = cJSON_GetObjectItem(root, "temp");
    cJSON *humidity = cJSON_GetObjectItem(root, "humidity");
    cJSON *soil = cJSON_GetObjectItem(root, "soil");
    cJSON *light = cJSON_GetObjectItem(root, "light");
    if (!cJSON_IsNumber(tmp) || !cJSON_IsNumber(humidity) || !cJSON_IsNumber(soil) || !cJSON_IsNumber(light))
    {
        return false;
    }

    sensor_data_t sd = {0};
    cJSON *item = cJSON_GetObjectItem(root, "device_id");
    if (cJSON_IsString(item))
    {
        strncpy(sd.device_id, item->valuestring, sizeof(sd.device_id) - 1);
    }

    sd.temp = cJSON_GetObjectItem(root, "temp")->valuedouble;
    sd.humidity = humidity->valuedouble;
    sd.soil = soil->valuedouble;
    sd.light = (uint32_t)light->valuedouble;

    item = cJSON_GetObjectItem(root, "battery");
    uint8_t batt = 0;
    if (parse_battery_item(item, &batt))
    {
        sd.battery = batt;
    }

    item = cJSON_GetObjectItem(root, "water");
    if (cJSON_IsString(item) && item->valuestring != NULL)
    {
        strncpy(sd.water, item->valuestring, sizeof(sd.water) - 1);
        sd.water_level_valid = parse_water_level_text(sd.water, &sd.water_level);
    }
    else if (cJSON_IsNumber(item))
    {
        sd.water_level = item->valuedouble;
        sd.water_level_valid = true;
        snprintf(sd.water, sizeof(sd.water), "%.1f", sd.water_level);
    }

    item = cJSON_GetObjectItem(root, "timestamp");
    if (cJSON_IsString(item))
    {
        strncpy(sd.timestamp, item->valuestring, sizeof(sd.timestamp) - 1);
    }

    publish_sensor_data(&sd, recv_buf);
    ESP_LOGI(TAG, "Legacy telemetry parsed T=%.1f H=%.1f S=%.1f", sd.temp, sd.humidity, sd.soil);
    return true;
}

void uart_receiver_register_sensor_callback(sensor_data_cb_t cb)
{
    sensor_cb = cb;
}

void uart_receiver_register_button_callback(button_event_cb_t cb)
{
    button_cb = cb;
}

const sensor_data_t *uart_receiver_get_latest_data(void)
{
    return has_data ? &last_data : NULL;
}

static void uart_rx_task(void *arg)
{
    (void)arg;
    uint8_t data[RD_BUF_SIZE];
    while (1)
    {
        int len = uart_read_bytes(STM32_UART_PORT, data, RD_BUF_SIZE, pdMS_TO_TICKS(100));
        if (len <= 0)
        {
            continue;
        }

        for (int i = 0; i < len; i++)
        {
            char c = (char)data[i];
            if (c == '\r')
            {
                continue;
            }

            if (buf_idx < UART_BUF_SIZE - 1)
            {
                recv_buf[buf_idx++] = c;
            }

            if (c != '\n')
            {
                continue;
            }

            recv_buf[buf_idx] = '\0';
            if (buf_idx > 0 && recv_buf[buf_idx - 1] == '\n')
            {
                recv_buf[--buf_idx] = '\0';
            }
            recv_count++;
            ESP_LOGI(TAG, "Frame #%d: %s", recv_count, recv_buf);

            cJSON *root = cJSON_Parse(recv_buf);
            if (root != NULL)
            {
                parse_success_count++;
                bool consumed = handle_wrapped_message(root);
                if (!consumed)
                {
                    consumed = handle_legacy_flat_message(root);
                }
                if (!consumed)
                {
                    ESP_LOGW(TAG, "JSON parsed but message format not recognized");
                }
                cJSON_Delete(root);
            }
            else
            {
                const char *err = cJSON_GetErrorPtr();
                ESP_LOGW(TAG, "JSON parse failed at: %s", err ? err : "unknown");
            }

            buf_idx = 0;
            memset(recv_buf, 0, UART_BUF_SIZE);
        }
    }
}

void uart_receiver_init(void)
{
    uart_config_t uart_cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    ESP_ERROR_CHECK(uart_param_config(STM32_UART_PORT, &uart_cfg));
    ESP_ERROR_CHECK(uart_set_pin(
        STM32_UART_PORT,
        STM32_UART_TX,
        STM32_UART_RX,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(
        STM32_UART_PORT,
        UART_BUF_SIZE * 2,
        0,
        0,
        NULL,
        0));

    xTaskCreate(
        uart_rx_task,
        "uart_rx_task",
        4096,
        NULL,
        configMAX_PRIORITIES - 1,
        NULL);
    ESP_LOGI(TAG, "UART receiver initialized on port %d (TX=%d, RX=%d)",
             STM32_UART_PORT, STM32_UART_TX, STM32_UART_RX);
}
