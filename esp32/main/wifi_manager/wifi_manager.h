#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

typedef struct
{
    char ssid[33];
    char password[65];
} wifi_credentials_t;

esp_err_t wifi_manager_init(void);
bool wifi_manager_is_connected(void);
bool wifi_manager_is_provisioning(void);
esp_err_t wifi_manager_start_provisioning(void);
esp_err_t wifi_manager_get_credentials(wifi_credentials_t *out);
esp_err_t wifi_manager_get_sta_ip_string(char *out, size_t out_len);
esp_err_t wifi_manager_get_provision_ap_ssid(char *out, size_t out_len);
esp_err_t wifi_manager_get_provision_ap_password(char *out, size_t out_len);
esp_err_t wifi_manager_set_credentials(const char *ssid, const char *password, bool reconnect_now);
esp_err_t wifi_manager_clear_credentials(void);
bool wifi_manager_time_is_synced(void);
esp_err_t wifi_manager_get_local_time_string(char *out, size_t out_len);
esp_err_t wifi_manager_request_time_sync(const char *reason);
void wifi_manager_notify_stm_sync_ack(const char *trace_id, const char *ack_status);
esp_err_t wifi_manager_request_threshold_sync(const char *reason);
void wifi_manager_notify_stm_config_ack(const char *ack_type,
                                        const char *origin_trace_id,
                                        const char *ack_status,
                                        int result_code);
void wifi_manager_dump_status(void);

#endif // WIFI_MANAGER_H
