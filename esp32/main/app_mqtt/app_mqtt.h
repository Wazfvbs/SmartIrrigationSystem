#ifndef APP_MQTT_H
#define APP_MQTT_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define CLOUD_BASE_URL_MAX_LEN 128
#define CLOUD_DEVICE_ID_MAX_LEN 32
#define CLOUD_AUTH_TOKEN_MAX_LEN 192
#define CLOUD_URL_MAX_LEN 220
#define CLOUD_ERROR_TEXT_MAX_LEN 96

typedef struct
{
    bool started;
    bool connected;
    int last_http_status;
    uint32_t upload_ok_count;
    uint32_t upload_fail_count;
    uint32_t poll_ok_count;
    uint32_t poll_fail_count;
    uint32_t recv_cmd_count;
    char base_url[CLOUD_BASE_URL_MAX_LEN];
    char device_id[CLOUD_DEVICE_ID_MAX_LEN];
    char send_url[CLOUD_URL_MAX_LEN];
    char poll_url[CLOUD_URL_MAX_LEN];
    char last_error_text[CLOUD_ERROR_TEXT_MAX_LEN];
} cloud_http_status_t;

esp_err_t mqtt_client_init(void);
esp_err_t mqtt_client_start(void);
esp_err_t mqtt_client_stop(void);
bool mqtt_client_is_started(void);
bool mqtt_client_is_connected(void);
int mqtt_publish_report(const char *payload);
esp_err_t mqtt_client_get_status(cloud_http_status_t *out);
void mqtt_client_dump_status(void);
esp_err_t mqtt_client_update_config(const char *broker_uri, const char *device_id, bool restart_if_running);
esp_err_t mqtt_client_update_auth_token(const char *auth_token, bool restart_if_running);

#endif // APP_MQTT_H
