#include "app_mqtt.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdarg.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "cJSON.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif

#include "../wifi_manager/wifi_manager.h"

static const char *TAG = "CLOUD_HTTP";

#define NETCFG_NAMESPACE "net_cfg"
#define NVS_KEY_BASE_URL "cloud_base_url"
#define NVS_KEY_DEVICE_ID "device_id"
#define NVS_KEY_AUTH_TOKEN "auth_token"

#define DEFAULT_BASE_URL "https://www.smartirrigation.cn"
#define DEFAULT_DEVICE_ID "MCU01"
#define DEFAULT_UPLOAD_PATH "/api/plant/upload"
#define DEFAULT_CONTROL_SEND_PATH "/api/control/send"

#define HTTP_TIMEOUT_MS 8000

static bool s_initialized = false;
static bool s_started = false;

static char s_base_url[CLOUD_BASE_URL_MAX_LEN] = {0};
static char s_device_id[CLOUD_DEVICE_ID_MAX_LEN] = {0};
static char s_auth_token[CLOUD_AUTH_TOKEN_MAX_LEN] = {0};
static char s_upload_url[CLOUD_URL_MAX_LEN] = {0};
static char s_control_send_url[CLOUD_URL_MAX_LEN] = {0};
static char s_last_error_text[CLOUD_ERROR_TEXT_MAX_LEN] = "none";

static uint32_t s_upload_ok_count = 0;
static uint32_t s_upload_fail_count = 0;
static uint32_t s_poll_ok_count = 0;
static uint32_t s_poll_fail_count = 0;
static uint32_t s_recv_cmd_count = 0;
static int s_last_http_status = 0;

static void set_last_error_text(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(s_last_error_text, sizeof(s_last_error_text), fmt, args);
    va_end(args);
}

static esp_err_t ensure_nvs_ready(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret == ESP_ERR_NVS_INVALID_STATE)
    {
        ret = ESP_OK;
    }
    return ret;
}

static void rebuild_urls(void)
{
    snprintf(s_upload_url, sizeof(s_upload_url), "%s%s", s_base_url, DEFAULT_UPLOAD_PATH);
    snprintf(s_control_send_url, sizeof(s_control_send_url), "%s%s", s_base_url, DEFAULT_CONTROL_SEND_PATH);
}

static void normalize_base_url(void)
{
    if (s_base_url[0] == '\0')
    {
        strncpy(s_base_url, DEFAULT_BASE_URL, sizeof(s_base_url) - 1);
        return;
    }

    size_t n = strlen(s_base_url);
    while (n > 0 && s_base_url[n - 1] == '/')
    {
        s_base_url[n - 1] = '\0';
        n--;
    }

    if (strcmp(s_base_url, "https://smartirrigation.cn") == 0)
    {
        strncpy(s_base_url, DEFAULT_BASE_URL, sizeof(s_base_url) - 1);
        ESP_LOGW(TAG, "base_url normalized to %s (avoid redirect)", s_base_url);
    }
}

static esp_err_t load_runtime_config(void)
{
    strncpy(s_base_url, DEFAULT_BASE_URL, sizeof(s_base_url) - 1);
    strncpy(s_device_id, DEFAULT_DEVICE_ID, sizeof(s_device_id) - 1);
    s_auth_token[0] = '\0';

    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NETCFG_NAMESPACE, NVS_READONLY, &handle);
    if (ret == ESP_OK)
    {
        size_t base_url_len = sizeof(s_base_url);
        size_t device_id_len = sizeof(s_device_id);
        if (nvs_get_str(handle, NVS_KEY_BASE_URL, s_base_url, &base_url_len) != ESP_OK)
        {
            strncpy(s_base_url, DEFAULT_BASE_URL, sizeof(s_base_url) - 1);
        }
        if (nvs_get_str(handle, NVS_KEY_DEVICE_ID, s_device_id, &device_id_len) != ESP_OK)
        {
            strncpy(s_device_id, DEFAULT_DEVICE_ID, sizeof(s_device_id) - 1);
        }

        size_t auth_len = sizeof(s_auth_token);
        if (nvs_get_str(handle, NVS_KEY_AUTH_TOKEN, s_auth_token, &auth_len) != ESP_OK)
        {
            s_auth_token[0] = '\0';
        }
        nvs_close(handle);
    }

    normalize_base_url();
    rebuild_urls();
    return ESP_OK;
}

static esp_err_t save_runtime_config(void)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NETCFG_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = nvs_set_str(handle, NVS_KEY_BASE_URL, s_base_url);
    if (ret == ESP_OK)
    {
        ret = nvs_set_str(handle, NVS_KEY_DEVICE_ID, s_device_id);
    }
    if (ret == ESP_OK)
    {
        if (s_auth_token[0] != '\0')
        {
            ret = nvs_set_str(handle, NVS_KEY_AUTH_TOKEN, s_auth_token);
        }
        else
        {
            ret = nvs_erase_key(handle, NVS_KEY_AUTH_TOKEN);
            if (ret == ESP_ERR_NVS_NOT_FOUND)
            {
                ret = ESP_OK;
            }
        }
    }
    if (ret == ESP_OK)
    {
        ret = nvs_commit(handle);
    }
    nvs_close(handle);
    return ret;
}

static esp_err_t http_request_json(esp_http_client_method_t method,
                                   const char *url,
                                   const char *payload,
                                   char *resp_buf,
                                   size_t resp_buf_size,
                                   int *status_code)
{
    esp_http_client_config_t cfg = {
        .url = url,
        .timeout_ms = HTTP_TIMEOUT_MS,
    };
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
    cfg.crt_bundle_attach = esp_crt_bundle_attach;
#endif

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    esp_http_client_set_method(client, method);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Accept", "application/json");
    esp_http_client_set_header(client, "X-Device-ID", s_device_id);
    if (s_auth_token[0] != '\0')
    {
        esp_http_client_set_header(client, "Authorization", s_auth_token);
    }
    if (payload != NULL)
    {
        esp_http_client_set_post_field(client, payload, strlen(payload));
    }

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        s_last_http_status = esp_http_client_get_status_code(client);
        if (status_code != NULL)
        {
            *status_code = s_last_http_status;
        }
        if (resp_buf != NULL && resp_buf_size > 0)
        {
            int read_len = esp_http_client_read_response(client, resp_buf, resp_buf_size - 1);
            if (read_len < 0)
            {
                resp_buf[0] = '\0';
            }
            else
            {
                resp_buf[read_len] = '\0';
            }
        }
    }

    esp_http_client_cleanup(client);
    return err;
}

// 当前后端 openapi 未提供 /api/control/poll，暂不启用轮询拉取命令。

esp_err_t mqtt_client_init(void)
{
    if (s_initialized)
    {
        return ESP_OK;
    }

    esp_err_t ret = ensure_nvs_ready();
    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = load_runtime_config();
    if (ret != ESP_OK)
    {
        return ret;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Cloud HTTP initialized base_url=%s device_id=%s", s_base_url, s_device_id);
    return ESP_OK;
}

esp_err_t mqtt_client_start(void)
{
    if (!s_initialized)
    {
        esp_err_t ret = mqtt_client_init();
        if (ret != ESP_OK)
        {
            return ret;
        }
    }
    if (s_started)
    {
        return ESP_OK;
    }

    s_started = true;
    ESP_LOGI(TAG, "Cloud HTTP started, upload=%s control_send=%s (poll disabled)", s_upload_url, s_control_send_url);
    return ESP_OK;
}

esp_err_t mqtt_client_stop(void)
{
    if (!s_started)
    {
        return ESP_OK;
    }

    s_started = false;
    ESP_LOGI(TAG, "Cloud HTTP stopped");
    return ESP_OK;
}

bool mqtt_client_is_started(void)
{
    return s_started;
}

bool mqtt_client_is_connected(void)
{
    return s_started && wifi_manager_is_connected();
}

int mqtt_publish_report(const char *payload)
{
    if (payload == NULL || payload[0] == '\0')
    {
        return -1;
    }
    if (!mqtt_client_is_connected())
    {
        s_upload_fail_count++;
        ESP_LOGW(TAG, "Upload skipped (cloud not ready), fail_count=%" PRIu32, s_upload_fail_count);
        set_last_error_text("upload skipped: cloud offline");
        return -1;
    }

    int status = 0;
    char resp_buf[192] = {0};
    esp_err_t err = http_request_json(HTTP_METHOD_POST, s_upload_url, payload, resp_buf, sizeof(resp_buf), &status);
    if (err == ESP_OK && status >= 200 && status < 300)
    {
        s_upload_ok_count++;
        set_last_error_text("none");
        return status;
    }

    s_upload_fail_count++;
    if (status == 401 || status == 403)
    {
        ESP_LOGW(TAG, "Upload unauthorized err=%s status=%d body=%s", esp_err_to_name(err), status, resp_buf[0] ? resp_buf : "<empty>");
        set_last_error_text("upload unauthorized (%d)", status);
    }
    else if (status >= 500 && status < 600)
    {
        ESP_LOGW(TAG, "Upload server error err=%s status=%d body=%s", esp_err_to_name(err), status, resp_buf[0] ? resp_buf : "<empty>");
        set_last_error_text("upload server error (%d)", status);
    }
    else
    {
        ESP_LOGW(TAG, "Upload failed err=%s status=%d body=%s", esp_err_to_name(err), status, resp_buf[0] ? resp_buf : "<empty>");
        set_last_error_text("upload failed: %s/%d", esp_err_to_name(err), status);
    }
    return -1;
}

esp_err_t mqtt_client_get_status(cloud_http_status_t *out)
{
    if (out == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(out, 0, sizeof(*out));
    out->started = s_started;
    out->connected = mqtt_client_is_connected();
    out->last_http_status = s_last_http_status;
    out->upload_ok_count = s_upload_ok_count;
    out->upload_fail_count = s_upload_fail_count;
    out->poll_ok_count = s_poll_ok_count;
    out->poll_fail_count = s_poll_fail_count;
    out->recv_cmd_count = s_recv_cmd_count;
    strncpy(out->base_url, s_base_url, sizeof(out->base_url) - 1);
    strncpy(out->device_id, s_device_id, sizeof(out->device_id) - 1);
    strncpy(out->send_url, s_upload_url, sizeof(out->send_url) - 1);
    strncpy(out->poll_url, s_control_send_url, sizeof(out->poll_url) - 1);
    strncpy(out->last_error_text, s_last_error_text, sizeof(out->last_error_text) - 1);
    return ESP_OK;
}

void mqtt_client_dump_status(void)
{
    ESP_LOGI(TAG,
             "started=%d wifi=%d base_url=%s device_id=%s auth=%d upload=%s control_send=%s upload_ok=%" PRIu32 " upload_fail=%" PRIu32 " poll_ok=%" PRIu32 " poll_fail=%" PRIu32 " commands=%" PRIu32 " last_http=%d",
             s_started,
             wifi_manager_is_connected(),
             s_base_url,
             s_device_id,
             s_auth_token[0] != '\0',
             s_upload_url,
             s_control_send_url,
             s_upload_ok_count,
             s_upload_fail_count,
             s_poll_ok_count,
             s_poll_fail_count,
             s_recv_cmd_count,
             s_last_http_status);
}

esp_err_t mqtt_client_update_config(const char *broker_uri, const char *device_id, bool restart_if_running)
{
    if (broker_uri == NULL || broker_uri[0] == '\0' || device_id == NULL || device_id[0] == '\0')
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(broker_uri) >= sizeof(s_base_url) || strlen(device_id) >= sizeof(s_device_id))
    {
        return ESP_ERR_INVALID_SIZE;
    }

    strncpy(s_base_url, broker_uri, sizeof(s_base_url) - 1);
    strncpy(s_device_id, device_id, sizeof(s_device_id) - 1);
    normalize_base_url();
    rebuild_urls();

    esp_err_t ret = save_runtime_config();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to save cloud config: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Cloud config updated base_url=%s device_id=%s", s_base_url, s_device_id);

    if (restart_if_running && s_started)
    {
        mqtt_client_stop();
        ret = mqtt_client_start();
    }

    return ret;
}

esp_err_t mqtt_client_update_auth_token(const char *auth_token, bool restart_if_running)
{
    if (auth_token == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(auth_token) >= sizeof(s_auth_token))
    {
        return ESP_ERR_INVALID_SIZE;
    }

    memset(s_auth_token, 0, sizeof(s_auth_token));
    strncpy(s_auth_token, auth_token, sizeof(s_auth_token) - 1);

    esp_err_t ret = save_runtime_config();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to save auth token: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Cloud auth token updated (enabled=%d)", s_auth_token[0] != '\0');

    if (restart_if_running && s_started)
    {
        mqtt_client_stop();
        ret = mqtt_client_start();
    }

    return ret;
}
