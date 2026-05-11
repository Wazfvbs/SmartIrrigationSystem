#include "wifi_manager.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "cJSON.h"

#include "../config_manager/config_manager.h"
#include "../stm32_protocol/stm32_protocol.h"

static const char *TAG = "WiFi_Manager";

#define NETCFG_NAMESPACE "net_cfg"
#define NVS_KEY_WIFI_SSID "wifi_ssid"
#define NVS_KEY_WIFI_PASS "wifi_pass"

#define WIFI_CONNECTED_BIT BIT0
#define RECONNECT_BASE_DELAY_MS 1000U
#define RECONNECT_MAX_DELAY_MS 30000U

#define PROV_AP_PASS "12345678"
#define PROV_HTML_MAX_BODY 512
#define PROV_SCAN_MAX_AP 24
#define PROV_SCAN_RETRY_MAX 2U
#define PROV_FALLBACK_BOOT_ATTEMPTS 8U
#define PROV_FALLBACK_RUNTIME_ATTEMPTS 30U
#define PROV_NO_AP_QUICK_FALLBACK_ATTEMPTS 2U
#define PROV_SCAN_PASSIVE_FALLBACK_MS 180

#define TIME_SYNC_VALID_EPOCH 1700000000
#define TIME_SYNC_TASK_STACK 4096
#define TIME_SYNC_WAIT_MS 15000
#define TIME_SYNC_RETRY_MS 5000
#define TIME_SYNC_INITIAL_DELAY_MS 10000
#define TIME_SYNC_PUSH_COOLDOWN_MS 60000

#define THRESHOLD_SYNC_MAX_ATTEMPTS 3U
#define THRESHOLD_SYNC_ACK_TIMEOUT_MS 2500U
#define THRESHOLD_SYNC_RETRY_DELAY_MS 1200U

static EventGroupHandle_t s_wifi_event_group = NULL;
static TimerHandle_t s_reconnect_timer = NULL;
static TimerHandle_t s_threshold_sync_timer = NULL;
static SemaphoreHandle_t s_scan_mutex = NULL;
static SemaphoreHandle_t s_threshold_sync_mutex = NULL;
static esp_event_handler_instance_t s_wifi_evt_handle = NULL;
static esp_event_handler_instance_t s_ip_evt_handle = NULL;
static esp_netif_t *s_sta_netif = NULL;
static esp_netif_t *s_ap_netif = NULL;

static bool s_initialized = false;
static bool s_wifi_connected = false;
static bool s_wifi_started = false;
static bool s_provisioning = false;
static bool s_time_synced = false;
static bool s_time_sync_task_running = false;
static bool s_sntp_inited = false;

static uint32_t s_disconnect_count = 0;
static uint32_t s_reconnect_attempts = 0;
static uint32_t s_no_ap_disconnect_count = 0;
static uint32_t s_connect_success_count = 0;
static uint32_t s_last_reconnect_delay_ms = 0;
static uint32_t s_provisioning_start_count = 0;
static uint32_t s_time_sync_ok_count = 0;
static uint32_t s_time_sync_fail_count = 0;
static uint32_t s_sync_info_sent_count = 0;
static uint32_t s_sync_info_ack_ok_count = 0;
static uint32_t s_sync_info_ack_fail_count = 0;
static uint32_t s_threshold_sync_sent_count = 0;
static uint32_t s_threshold_sync_ack_ok_count = 0;
static uint32_t s_threshold_sync_ack_fail_count = 0;
static uint32_t s_threshold_sync_timeout_count = 0;
static uint32_t s_threshold_sync_giveup_count = 0;
static TickType_t s_last_sync_push_tick = 0;
static esp_ip4_addr_t s_current_ip = {0};
static wifi_credentials_t s_active_credentials = {0};
static bool s_provision_switch_pending = false;
static bool s_sync_info_wait_ack = false;
static bool s_sync_info_ack_success = false;
static bool s_sync_info_retry_used = false;
static bool s_auto_provision_pending = false;
static bool s_threshold_sync_session_active = false;
static bool s_threshold_sync_wait_ack = false;
static uint8_t s_threshold_sync_attempt = 0;
static double s_threshold_sync_lower = 30.0;
static double s_threshold_sync_upper = 70.0;
static char s_threshold_sync_trace_id[STM32_TRACE_ID_MAX_LEN] = {0};
static char s_threshold_sync_reason[40] = {0};

static httpd_handle_t s_provision_httpd = NULL;
static char s_provision_ap_ssid[33] = {0};
static char s_last_time_sync_text[20] = {0};

static const char *s_provision_html =
    "<!doctype html><html><head><meta charset='utf-8'/>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'/>"
    "<title>Smart Irrigation Setup</title>"
    "<style>"
    ":root{--bg:#f3f7fb;--card:#fff;--line:#d8e2ee;--text:#1f2d3d;--sub:#5f6f82;--pri:#1f8ef1;--ok:#159a69;--warn:#d87a1b;}"
    "*{box-sizing:border-box}body{margin:0;background:linear-gradient(160deg,#eef4fb,#f8fbff);font-family:Segoe UI,Roboto,Helvetica,Arial,sans-serif;color:var(--text)}"
    ".wrap{max-width:720px;margin:0 auto;padding:18px}.card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:16px;box-shadow:0 10px 24px rgba(31,45,61,.08)}"
    "h1{font-size:1.2rem;margin:0 0 6px}.muted{color:var(--sub);font-size:.92rem;line-height:1.45}"
    ".grid{display:grid;grid-template-columns:1fr auto;gap:10px;align-items:end;margin-top:14px}"
    "label{display:block;font-size:.86rem;color:var(--sub);margin:0 0 6px}"
    "input,select{width:100%;padding:11px 12px;border:1px solid var(--line);border-radius:10px;font-size:.98rem;background:#fff}"
    "input:focus,select:focus{outline:none;border-color:#7fb7f6;box-shadow:0 0 0 3px rgba(31,142,241,.15)}"
    ".row{display:grid;gap:10px;margin-top:10px}.btn{border:0;padding:11px 14px;border-radius:10px;cursor:pointer;font-weight:600}"
    ".btn.pri{background:var(--pri);color:#fff}.btn.ghost{background:#edf3fb;color:#29527a}.btn:disabled{opacity:.55;cursor:not-allowed}"
    ".status{margin-top:12px;padding:10px 12px;border-radius:10px;background:#f7fafc;border:1px dashed var(--line);white-space:pre-wrap;word-break:break-word;font-size:.9rem}"
    ".top{display:flex;justify-content:space-between;gap:8px;align-items:center;flex-wrap:wrap;margin-bottom:8px}"
    ".chip{font-size:.8rem;padding:4px 9px;border-radius:999px;background:#e7f4ff;color:#1f78c7;border:1px solid #cde6ff}"
    "@media (max-width:560px){.grid{grid-template-columns:1fr}.wrap{padding:12px}.card{padding:13px}}"
    "</style></head><body>"
    "<div class='wrap'><div class='card'>"
    "<div class='top'><h1>Smart Irrigation Wi-Fi Setup</h1><span class='chip' id='stateChip'>loading...</span></div>"
    "<div class='muted'>If your device moved to a new environment, select an available Wi-Fi from the list, or type SSID manually.</div>"
    "<div class='row'>"
    "<div class='grid'><div><label>Available Networks</label><select id='ssidList'><option value=''>Loading...</option></select></div><button class='btn ghost' id='btnScan' onclick='scanAp()'>Refresh</button></div>"
    "<div><label>SSID (manual or selected)</label><input id='ssid' autocomplete='off' placeholder='Wi-Fi SSID'/></div>"
    "<div><label>Password</label><input id='password' type='password' autocomplete='off' placeholder='Wi-Fi Password'/></div>"
    "<button class='btn pri' id='btnSave' onclick='saveCfg()'>Save and Connect</button>"
    "</div>"
    "<div class='status' id='msg'>Initializing...</div>"
    "</div></div>"
    "<script>"
    "const msg=document.getElementById('msg');const chip=document.getElementById('stateChip');"
    "function setMsg(t){msg.textContent=t||'';}"
    "async function refreshState(){try{const r=await fetch('/status?t='+Date.now(),{cache:'no-store'});const j=await r.json();const ssidInput=document.getElementById('ssid');chip.textContent=j.provisioning?'provisioning':'running';if(j.connected){chip.textContent+=' | online';}if(!j.provisioning&&j.ssid&&(!ssidInput.value||ssidInput.value.trim()==='')){ssidInput.value=j.ssid;}}catch(e){}}"
    "async function scanAp(){const btn=document.getElementById('btnScan');btn.disabled=true;try{setMsg('Scanning nearby Wi-Fi...');const r=await fetch('/scan?t='+Date.now(),{cache:'no-store'});if(!r.ok){throw new Error('HTTP '+r.status);}const j=await r.json();const sel=document.getElementById('ssidList');const ssidInput=document.getElementById('ssid');sel.innerHTML='';if(!j.ok||!Array.isArray(j.items)||j.items.length===0){const o=document.createElement('option');o.value='';o.textContent='No Wi-Fi found';sel.appendChild(o);setMsg('No Wi-Fi found. You can still type SSID manually.');return;}j.items.sort((a,b)=>b.rssi-a.rssi);const first=document.createElement('option');first.value='';first.textContent='Select Wi-Fi...';sel.appendChild(first);for(const i of j.items){const opt=document.createElement('option');const ssid=String(i.ssid||'');opt.value=ssid;opt.textContent=`${ssid} (${i.rssi} dBm)`;sel.appendChild(opt);}const autoSsid=String((j.items[0]&&j.items[0].ssid)||'');if(autoSsid){sel.value=autoSsid;ssidInput.value=autoSsid;setMsg(`Found ${j.items.length} networks. Auto selected: ${autoSsid}`);}else{setMsg(`Found ${j.items.length} networks.`);}}catch(e){setMsg('Scan failed: '+e);}finally{btn.disabled=false;}}"
    "document.getElementById('ssidList').addEventListener('change',e=>{if(e.target.value){document.getElementById('ssid').value=e.target.value;}});"
    "async function saveCfg(){const ssid=document.getElementById('ssid').value.trim();const password=document.getElementById('password').value;const btn=document.getElementById('btnSave');if(!ssid){setMsg('SSID is required.');return;}btn.disabled=true;try{const r=await fetch('/provision',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid,password}),cache:'no-store'});let j=null;try{j=await r.json();}catch(_e){}if(!r.ok){setMsg('Save failed: '+(j&&j.reason?j.reason:('HTTP '+r.status)));return;}setMsg((j&&j.message)?j.message:'Saved. Connecting...');}catch(e){setMsg('Submit failed: '+e);}finally{btn.disabled=false;}}"
    "scanAp();refreshState();setInterval(refreshState,3000);"
    "</script></body></html>";

static bool has_valid_credentials(const wifi_credentials_t *cred)
{
    return (cred != NULL) && (cred->ssid[0] != '\0');
}

static void trim_inplace(char *s)
{
    if (s == NULL)
    {
        return;
    }

    size_t len = strlen(s);
    size_t start = 0;
    while (start < len && isspace((unsigned char)s[start]))
    {
        start++;
    }

    size_t end = len;
    while (end > start && isspace((unsigned char)s[end - 1]))
    {
        end--;
    }

    if (start > 0)
    {
        memmove(s, s + start, end - start);
    }
    s[end - start] = '\0';
}

static void set_http_no_cache_headers(httpd_req_t *req)
{
    if (req == NULL)
    {
        return;
    }
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
}

static esp_err_t send_json_response(httpd_req_t *req, cJSON *root)
{
    if (req == NULL || root == NULL)
    {
        if (root != NULL)
        {
            cJSON_Delete(root);
        }
        return ESP_ERR_INVALID_ARG;
    }

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (payload == NULL)
    {
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_set_type(req, "application/json; charset=utf-8");
        return httpd_resp_sendstr(req, "{\"ok\":false,\"reason\":\"json encode failed\"}");
    }

    set_http_no_cache_headers(req);
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    esp_err_t ret = httpd_resp_send(req, payload, HTTPD_RESP_USE_STRLEN);
    cJSON_free(payload);
    return ret;
}

static bool get_local_time_string_internal(char *out, size_t out_len)
{
    if (out == NULL || out_len < sizeof("YYYY-MM-DD HH:MM:SS"))
    {
        return false;
    }

    time_t now = time(NULL);
    if (now < TIME_SYNC_VALID_EPOCH)
    {
        return false;
    }

    struct tm tm_info = {0};
    if (localtime_r(&now, &tm_info) == NULL)
    {
        return false;
    }

    size_t n = strftime(out, out_len, "%Y-%m-%d %H:%M:%S", &tm_info);
    return n > 0;
}

static const char *threshold_sync_reason_text(void)
{
    return (s_threshold_sync_reason[0] != '\0') ? s_threshold_sync_reason : "manual";
}

static void threshold_sync_copy_reason(const char *reason)
{
    const char *src = (reason != NULL && reason[0] != '\0') ? reason : "manual";
    strncpy(s_threshold_sync_reason, src, sizeof(s_threshold_sync_reason) - 1);
    s_threshold_sync_reason[sizeof(s_threshold_sync_reason) - 1] = '\0';
}

static void threshold_sync_load_config(double *lower, double *upper)
{
    double lo = 30.0;
    double hi = 70.0;
    const device_config_t *cfg = config_manager_get_config();
    if (cfg != NULL)
    {
        lo = cfg->threshold_lower;
        hi = cfg->threshold_upper;
    }

    if (lo < 0.0)
    {
        lo = 0.0;
    }
    if (lo > 99.0)
    {
        lo = 99.0;
    }
    if (hi < 1.0)
    {
        hi = 1.0;
    }
    if (hi > 100.0)
    {
        hi = 100.0;
    }
    if (lo >= hi)
    {
        hi = lo + 1.0;
        if (hi > 100.0)
        {
            hi = 100.0;
            lo = hi - 1.0;
        }
    }

    if (lower != NULL)
    {
        *lower = lo;
    }
    if (upper != NULL)
    {
        *upper = hi;
    }
}

static bool threshold_sync_arm_timer(uint32_t delay_ms)
{
    if (s_threshold_sync_timer == NULL)
    {
        return false;
    }
    if (xTimerChangePeriod(s_threshold_sync_timer, pdMS_TO_TICKS(delay_ms), 0) != pdPASS)
    {
        return false;
    }
    return true;
}

static void threshold_sync_clear_session(void)
{
    s_threshold_sync_session_active = false;
    s_threshold_sync_wait_ack = false;
    s_threshold_sync_attempt = 0;
    s_threshold_sync_trace_id[0] = '\0';
}

static esp_err_t threshold_sync_send_next_attempt_locked(void)
{
    if (!s_threshold_sync_session_active)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_threshold_sync_attempt >= THRESHOLD_SYNC_MAX_ATTEMPTS)
    {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t attempt = (uint8_t)(s_threshold_sync_attempt + 1U);
    char trace_id[STM32_TRACE_ID_MAX_LEN] = {0};
    esp_err_t trace_ret = stm32_protocol_build_trace_id(trace_id, sizeof(trace_id));

    s_threshold_sync_attempt = attempt;
    s_threshold_sync_wait_ack = false;

    if (trace_ret != ESP_OK)
    {
        ESP_LOGW(TAG, "threshold_sync trace build failed: %s", esp_err_to_name(trace_ret));
        return trace_ret;
    }

    esp_err_t ret = stm32_protocol_send_threshold_config(s_threshold_sync_lower,
                                                         s_threshold_sync_upper,
                                                         true,
                                                         trace_id);
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "threshold_sync send failed attempt=%u/%u reason=%s err=%s",
                 (unsigned)attempt,
                 (unsigned)THRESHOLD_SYNC_MAX_ATTEMPTS,
                 threshold_sync_reason_text(),
                 esp_err_to_name(ret));
        return ret;
    }

    strncpy(s_threshold_sync_trace_id, trace_id, sizeof(s_threshold_sync_trace_id) - 1);
    s_threshold_sync_trace_id[sizeof(s_threshold_sync_trace_id) - 1] = '\0';
    s_threshold_sync_wait_ack = true;
    s_threshold_sync_sent_count++;

    if (!threshold_sync_arm_timer(THRESHOLD_SYNC_ACK_TIMEOUT_MS))
    {
        ESP_LOGW(TAG, "threshold_sync failed to arm ack timer");
        s_threshold_sync_wait_ack = false;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG,
             "threshold_sync sent trace=%s attempt=%u/%u lower=%.1f upper=%.1f reason=%s wait_ack=1",
             s_threshold_sync_trace_id,
             (unsigned)attempt,
             (unsigned)THRESHOLD_SYNC_MAX_ATTEMPTS,
             s_threshold_sync_lower,
             s_threshold_sync_upper,
             threshold_sync_reason_text());
    return ESP_OK;
}

static void threshold_sync_schedule_retry_locked(const char *trigger)
{
    if (!s_threshold_sync_session_active)
    {
        return;
    }
    if (s_threshold_sync_attempt >= THRESHOLD_SYNC_MAX_ATTEMPTS)
    {
        s_threshold_sync_giveup_count++;
        ESP_LOGE(TAG,
                 "threshold_sync giveup after %u attempts reason=%s trigger=%s",
                 (unsigned)s_threshold_sync_attempt,
                 threshold_sync_reason_text(),
                 trigger != NULL ? trigger : "unknown");
        threshold_sync_clear_session();
        return;
    }
    if (!threshold_sync_arm_timer(THRESHOLD_SYNC_RETRY_DELAY_MS))
    {
        s_threshold_sync_giveup_count++;
        ESP_LOGE(TAG,
                 "threshold_sync retry timer arm failed reason=%s trigger=%s",
                 threshold_sync_reason_text(),
                 trigger != NULL ? trigger : "unknown");
        threshold_sync_clear_session();
        return;
    }

    ESP_LOGW(TAG,
             "threshold_sync schedule retry in %" PRIu32 "ms attempt_next=%u/%u reason=%s trigger=%s",
             (uint32_t)THRESHOLD_SYNC_RETRY_DELAY_MS,
             (unsigned)(s_threshold_sync_attempt + 1U),
             (unsigned)THRESHOLD_SYNC_MAX_ATTEMPTS,
             threshold_sync_reason_text(),
             trigger != NULL ? trigger : "unknown");
}

static void threshold_sync_timer_cb(TimerHandle_t timer)
{
    (void)timer;

    if (s_threshold_sync_mutex == NULL)
    {
        return;
    }
    if (xSemaphoreTake(s_threshold_sync_mutex, pdMS_TO_TICKS(10)) != pdTRUE)
    {
        return;
    }

    if (!s_threshold_sync_session_active)
    {
        xSemaphoreGive(s_threshold_sync_mutex);
        return;
    }

    if (s_threshold_sync_wait_ack)
    {
        s_threshold_sync_wait_ack = false;
        s_threshold_sync_timeout_count++;
        ESP_LOGW(TAG,
                 "threshold_sync ack timeout trace=%s attempt=%u/%u reason=%s",
                 s_threshold_sync_trace_id[0] ? s_threshold_sync_trace_id : "none",
                 (unsigned)s_threshold_sync_attempt,
                 (unsigned)THRESHOLD_SYNC_MAX_ATTEMPTS,
                 threshold_sync_reason_text());
    }

    esp_err_t ret = threshold_sync_send_next_attempt_locked();
    if (ret != ESP_OK)
    {
        threshold_sync_schedule_retry_locked("timer_retry");
    }

    xSemaphoreGive(s_threshold_sync_mutex);
}

static esp_err_t sync_time_to_stm32(const char *reason)
{
    char ts[20] = {0};
    if (!get_local_time_string_internal(ts, sizeof(ts)))
    {
        ESP_LOGW(TAG, "Skip STM32 time sync: local time not ready");
        return ESP_ERR_INVALID_STATE;
    }

    const device_config_t *cfg = config_manager_get_config();
    const char *plant_name = (cfg != NULL && cfg->device_name[0] != '\0') ? cfg->device_name : "plant_1";
    const char *species = (cfg != NULL && cfg->species[0] != '\0') ? cfg->species : "unknown";

    bool already_waiting_ack = s_sync_info_wait_ack;
    esp_err_t ret = stm32_protocol_send_sync_info(ts, "cfg_v1.0", "strategy_v1.0", plant_name, species);
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to send sync_info to STM32: %s", esp_err_to_name(ret));
        return ret;
    }

    if (!already_waiting_ack)
    {
        s_sync_info_retry_used = false;
    }
    s_sync_info_wait_ack = true;
    s_sync_info_ack_success = false;
    s_sync_info_sent_count++;

    s_last_sync_push_tick = xTaskGetTickCount();
    ESP_LOGI(TAG,
             "sync_info sent: %s reason=%s wait_ack=1",
             ts,
             (reason != NULL && reason[0] != '\0') ? reason : "periodic");
    return ESP_OK;
}

static void time_sync_task(void *arg)
{
    uint32_t initial_delay_ms = (uint32_t)(uintptr_t)arg;

    if (!s_wifi_connected)
    {
        s_time_sync_task_running = false;
        vTaskDelete(NULL);
        return;
    }

    if (initial_delay_ms > 0)
    {
        vTaskDelay(pdMS_TO_TICKS(initial_delay_ms));
        if (!s_wifi_connected)
        {
            s_time_sync_task_running = false;
            vTaskDelete(NULL);
            return;
        }
    }

    setenv("TZ", "CST-8", 1);
    tzset();

    if (!s_sntp_inited)
    {
        esp_sntp_config_t sntp_cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG("ntp.aliyun.com");
        sntp_cfg.server_from_dhcp = false;
        sntp_cfg.wait_for_sync = true;
        sntp_cfg.start = true;
        esp_err_t ret = esp_netif_sntp_init(&sntp_cfg);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
        {
            s_time_sync_fail_count++;
            ESP_LOGW(TAG, "esp_netif_sntp_init failed: %s", esp_err_to_name(ret));
            s_time_sync_task_running = false;
            vTaskDelete(NULL);
            return;
        }
        s_sntp_inited = true;
    }

    bool synced = false;
    for (int attempt = 1; attempt <= 2; ++attempt)
    {
        esp_err_t start_ret = esp_netif_sntp_start();
        if (start_ret != ESP_OK)
        {
            ESP_LOGW(TAG, "esp_netif_sntp_start failed (attempt=%d): %s", attempt, esp_err_to_name(start_ret));
        }

        esp_err_t wait_ret = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(TIME_SYNC_WAIT_MS));
        char ts[20] = {0};
        if (wait_ret == ESP_OK && get_local_time_string_internal(ts, sizeof(ts)))
        {
            s_time_synced = true;
            s_time_sync_ok_count++;
            strncpy(s_last_time_sync_text, ts, sizeof(s_last_time_sync_text) - 1);
            s_last_time_sync_text[sizeof(s_last_time_sync_text) - 1] = '\0';
            ESP_LOGI(TAG, "SNTP time sync success (attempt=%d): %s", attempt, ts);
            (void)sync_time_to_stm32("sntp_ok");
            synced = true;
            break;
        }

        ESP_LOGW(TAG,
                 "SNTP sync not ready (attempt=%d, ret=%s), retry in %" PRIu32 " ms",
                 attempt,
                 esp_err_to_name(wait_ret),
                 (uint32_t)TIME_SYNC_RETRY_MS);
        if (attempt < 2)
        {
            vTaskDelay(pdMS_TO_TICKS(TIME_SYNC_RETRY_MS));
        }
    }

    if (!synced)
    {
        s_time_sync_fail_count++;
    }

    s_time_sync_task_running = false;
    vTaskDelete(NULL);
}

static void schedule_time_sync(uint32_t initial_delay_ms)
{
    if (s_time_synced)
    {
        char ts[20] = {0};
        if (get_local_time_string_internal(ts, sizeof(ts)))
        {
            return;
        }
    }

    if (s_time_sync_task_running)
    {
        return;
    }

    s_time_sync_task_running = true;
    BaseType_t ok = xTaskCreate(time_sync_task,
                                "time_sync",
                                TIME_SYNC_TASK_STACK,
                                (void *)(uintptr_t)initial_delay_ms,
                                5,
                                NULL);
    if (ok != pdPASS)
    {
        s_time_sync_task_running = false;
        s_time_sync_fail_count++;
        ESP_LOGW(TAG, "Failed to create time_sync task");
    }
}

static const char *dhcp_status_to_str(esp_netif_dhcp_status_t status)
{
    switch (status)
    {
    case ESP_NETIF_DHCP_INIT:
        return "INIT";
    case ESP_NETIF_DHCP_STARTED:
        return "STARTED";
    case ESP_NETIF_DHCP_STOPPED:
        return "STOPPED";
    default:
        return "UNKNOWN";
    }
}

static void dump_ap_netif_status(const char *stage)
{
    if (s_ap_netif == NULL)
    {
        ESP_LOGE(TAG, "[%s] AP netif is NULL", stage);
        return;
    }

    esp_netif_ip_info_t ip_info = {0};
    esp_err_t ret = esp_netif_get_ip_info(s_ap_netif, &ip_info);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG,
                 "[%s] AP netif ip=" IPSTR " gw=" IPSTR " mask=" IPSTR,
                 stage,
                 IP2STR(&ip_info.ip),
                 IP2STR(&ip_info.gw),
                 IP2STR(&ip_info.netmask));
    }
    else
    {
        ESP_LOGW(TAG, "[%s] esp_netif_get_ip_info(AP) failed: %s", stage, esp_err_to_name(ret));
    }

    esp_netif_dhcp_status_t dhcps = ESP_NETIF_DHCP_INIT;
    ret = esp_netif_dhcps_get_status(s_ap_netif, &dhcps);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "[%s] AP DHCP server status: %s", stage, dhcp_status_to_str(dhcps));
    }
    else
    {
        ESP_LOGW(TAG, "[%s] esp_netif_dhcps_get_status(AP) failed: %s", stage, esp_err_to_name(ret));
    }
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

static esp_err_t save_credentials_to_nvs(const wifi_credentials_t *cred)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NETCFG_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = nvs_set_str(handle, NVS_KEY_WIFI_SSID, cred->ssid);
    if (ret == ESP_OK)
    {
        ret = nvs_set_str(handle, NVS_KEY_WIFI_PASS, cred->password);
    }
    if (ret == ESP_OK)
    {
        ret = nvs_commit(handle);
    }

    nvs_close(handle);
    return ret;
}

static esp_err_t load_credentials_from_nvs(wifi_credentials_t *out)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NETCFG_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK)
    {
        return ret;
    }

    size_t ssid_len = sizeof(out->ssid);
    size_t pass_len = sizeof(out->password);
    ret = nvs_get_str(handle, NVS_KEY_WIFI_SSID, out->ssid, &ssid_len);
    if (ret == ESP_OK)
    {
        ret = nvs_get_str(handle, NVS_KEY_WIFI_PASS, out->password, &pass_len);
    }

    nvs_close(handle);
    return ret;
}

static uint32_t calc_reconnect_delay_ms(uint32_t attempt)
{
    uint32_t shift = attempt;
    if (shift > 5U)
    {
        shift = 5U;
    }
    uint32_t delay = RECONNECT_BASE_DELAY_MS << shift;
    if (delay > RECONNECT_MAX_DELAY_MS)
    {
        delay = RECONNECT_MAX_DELAY_MS;
    }
    return delay;
}

static esp_err_t apply_credentials(const wifi_credentials_t *cred)
{
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, cred->ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, cred->password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    wifi_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    return esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
}

static void stop_provision_http_server(void)
{
    if (s_provision_httpd != NULL)
    {
        httpd_stop(s_provision_httpd);
        s_provision_httpd = NULL;
    }
}

static esp_err_t start_sta_connection(void)
{
    if (!has_valid_credentials(&s_active_credentials))
    {
        return ESP_ERR_INVALID_STATE;
    }

    stop_provision_http_server();
    s_provisioning = false;
    s_no_ap_disconnect_count = 0;
    s_reconnect_attempts = 0;

    esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = apply_credentials(&s_active_credentials);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (!s_wifi_started)
    {
        ret = esp_wifi_start();
        if (ret == ESP_OK)
        {
            s_wifi_started = true;
        }
        return ret;
    }

    ret = esp_wifi_disconnect();
    if (ret != ESP_OK && ret != ESP_ERR_WIFI_NOT_CONNECT)
    {
        ESP_LOGW(TAG, "esp_wifi_disconnect failed: %s", esp_err_to_name(ret));
    }
    return esp_wifi_connect();
}

static void provision_switch_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_err_t inner_ret = start_sta_connection();
    if (inner_ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to switch STA after provisioning: %s", esp_err_to_name(inner_ret));
    }
    else
    {
        ESP_LOGI(TAG, "Provisioning completed, switched to STA ssid=\"%s\"", s_active_credentials.ssid);
    }
    s_provision_switch_pending = false;
    vTaskDelete(NULL);
}

static esp_err_t provision_root_handler(httpd_req_t *req)
{
    set_http_no_cache_headers(req);
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, s_provision_html, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t provision_status_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_set_type(req, "application/json; charset=utf-8");
        return httpd_resp_sendstr(req, "{\"ok\":false,\"reason\":\"oom\"}");
    }

    cJSON_AddBoolToObject(root, "provisioning", s_provisioning);
    cJSON_AddBoolToObject(root, "connected", s_wifi_connected);
    cJSON_AddStringToObject(root, "ssid", s_active_credentials.ssid);
    return send_json_response(req, root);
}

static const char *auth_mode_to_str(wifi_auth_mode_t mode)
{
    switch (mode)
    {
    case WIFI_AUTH_OPEN:
        return "OPEN";
    case WIFI_AUTH_WEP:
        return "WEP";
    case WIFI_AUTH_WPA_PSK:
        return "WPA_PSK";
    case WIFI_AUTH_WPA2_PSK:
        return "WPA2_PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:
        return "WPA_WPA2_PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE:
        return "WPA2_ENTERPRISE";
    case WIFI_AUTH_WPA3_PSK:
        return "WPA3_PSK";
    case WIFI_AUTH_WPA2_WPA3_PSK:
        return "WPA2_WPA3_PSK";
    case WIFI_AUTH_WAPI_PSK:
        return "WAPI_PSK";
    default:
        return "UNKNOWN";
    }
}

static esp_err_t provision_scan_handler(httpd_req_t *req)
{
    uint32_t free_heap = (uint32_t)esp_get_free_heap_size();
    uint32_t stack_hw = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI(TAG,
             "Provision /scan requested (free_heap=%" PRIu32 ", stack_hw=%" PRIu32 " words)",
             free_heap,
             stack_hw);

    bool scan_lock_taken = false;
    const char *fail_reason = NULL;
    cJSON *root = cJSON_CreateObject();
    cJSON *items = NULL;
    wifi_ap_record_t *ap_records = NULL;
    char(*seen_ssids)[33] = NULL;
    uint16_t ap_count = 0;
    uint16_t record_count = 0;
    uint16_t unique_count = 0;
    esp_err_t ret = ESP_OK;

    if (root == NULL)
    {
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_set_type(req, "application/json; charset=utf-8");
        return httpd_resp_sendstr(req, "{\"ok\":false,\"reason\":\"oom\"}");
    }

    if (s_scan_mutex == NULL)
    {
        fail_reason = "scan mutex not ready";
        ret = ESP_ERR_INVALID_STATE;
        goto fail;
    }

    if (xSemaphoreTake(s_scan_mutex, 0) != pdTRUE)
    {
        fail_reason = "scan busy";
        ret = ESP_ERR_TIMEOUT;
        goto fail;
    }
    scan_lock_taken = true;

    wifi_mode_t mode = WIFI_MODE_NULL;
    ret = esp_wifi_get_mode(&mode);
    if (ret != ESP_OK)
    {
        goto fail;
    }
    if (mode != WIFI_MODE_APSTA)
    {
        ESP_LOGW(TAG, "Provision scan force mode APSTA (old_mode=%d)", (int)mode);
        ret = esp_wifi_set_mode(WIFI_MODE_APSTA);
        if (ret != ESP_OK)
        {
            goto fail;
        }
    }

    wifi_scan_config_t scan_cfg = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {.active = {.min = 40, .max = 130}}};

    ret = ESP_FAIL;
    for (uint32_t attempt = 1; attempt <= PROV_SCAN_RETRY_MAX; ++attempt)
    {
        ESP_LOGI(TAG, "Provision scan start attempt=%" PRIu32 "/%" PRIu32, attempt, (uint32_t)PROV_SCAN_RETRY_MAX);
        ret = esp_wifi_scan_start(&scan_cfg, true);
        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "Provision scan start ok (attempt=%" PRIu32 ")", attempt);
            break;
        }

        ESP_LOGW(TAG,
                 "Provision scan start failed (attempt=%" PRIu32 "/%" PRIu32 "): %s",
                 attempt,
                 (uint32_t)PROV_SCAN_RETRY_MAX,
                 esp_err_to_name(ret));

        if (ret == ESP_ERR_WIFI_STATE)
        {
            (void)esp_wifi_scan_stop();
            vTaskDelay(pdMS_TO_TICKS(180));
            continue;
        }

        if (ret == ESP_ERR_WIFI_NOT_STARTED)
        {
            esp_err_t start_ret = esp_wifi_start();
            if (start_ret == ESP_OK || start_ret == ESP_ERR_INVALID_STATE)
            {
                s_wifi_started = true;
                vTaskDelay(pdMS_TO_TICKS(180));
                continue;
            }
        }
    }

    if (ret != ESP_OK)
    {
        goto fail;
    }

    ret = esp_wifi_scan_get_ap_num(&ap_count);
    if (ret != ESP_OK)
    {
        goto fail;
    }
    uint16_t raw_ap_count = ap_count;
    if (ap_count > PROV_SCAN_MAX_AP)
    {
        ap_count = PROV_SCAN_MAX_AP;
    }
    ESP_LOGI(TAG, "Provision scan AP num: raw=%u capped=%u", (unsigned int)raw_ap_count, (unsigned int)ap_count);

    if (ap_count == 0)
    {
        ESP_LOGW(TAG, "Provision active scan found 0 AP, fallback to passive scan");
        wifi_scan_config_t passive_cfg = {
            .ssid = NULL,
            .bssid = NULL,
            .channel = 0,
            .show_hidden = true,
            .scan_type = WIFI_SCAN_TYPE_PASSIVE,
            .scan_time = {.passive = PROV_SCAN_PASSIVE_FALLBACK_MS}};
        esp_err_t passive_ret = esp_wifi_scan_start(&passive_cfg, true);
        if (passive_ret == ESP_OK)
        {
            passive_ret = esp_wifi_scan_get_ap_num(&ap_count);
            if (passive_ret != ESP_OK)
            {
                ESP_LOGW(TAG, "Passive scan get_ap_num failed: %s", esp_err_to_name(passive_ret));
                ap_count = 0;
            }
            if (ap_count > PROV_SCAN_MAX_AP)
            {
                ap_count = PROV_SCAN_MAX_AP;
            }
            ESP_LOGI(TAG, "Provision passive scan AP num: %u", (unsigned int)ap_count);
        }
        else
        {
            ESP_LOGW(TAG, "Provision passive scan fallback failed: %s", esp_err_to_name(passive_ret));
        }
    }

    items = cJSON_AddArrayToObject(root, "items");
    if (items == NULL)
    {
        fail_reason = "oom";
        ret = ESP_ERR_NO_MEM;
        goto fail;
    }

    seen_ssids = calloc(PROV_SCAN_MAX_AP, sizeof(*seen_ssids));
    if (seen_ssids == NULL)
    {
        fail_reason = "oom";
        ret = ESP_ERR_NO_MEM;
        goto fail;
    }

    if (ap_count > 0)
    {
        ap_records = calloc(ap_count, sizeof(*ap_records));
        if (ap_records == NULL)
        {
            fail_reason = "oom";
            ret = ESP_ERR_NO_MEM;
            goto fail;
        }

        record_count = ap_count;
        ret = esp_wifi_scan_get_ap_records(&record_count, ap_records);
        if (ret != ESP_OK)
        {
            goto fail;
        }
        ESP_LOGI(TAG, "Provision scan records fetched: %u", (unsigned int)record_count);
    }

    for (uint16_t i = 0; i < record_count; ++i)
    {
        const char *ssid = (const char *)ap_records[i].ssid;
        if (ssid == NULL || ssid[0] == '\0')
        {
            continue;
        }

        bool duplicate = false;
        for (uint16_t k = 0; k < unique_count; ++k)
        {
            if (strcmp(seen_ssids[k], ssid) == 0)
            {
                duplicate = true;
                break;
            }
        }
        if (duplicate)
        {
            continue;
        }

        strncpy(seen_ssids[unique_count], ssid, sizeof(seen_ssids[unique_count]) - 1);
        unique_count++;

        cJSON *item = cJSON_CreateObject();
        if (item == NULL)
        {
            continue;
        }
        cJSON_AddStringToObject(item, "ssid", ssid);
        cJSON_AddNumberToObject(item, "rssi", ap_records[i].rssi);
        cJSON_AddStringToObject(item, "auth", auth_mode_to_str(ap_records[i].authmode));
        cJSON_AddItemToArray(items, item);

        if (unique_count >= PROV_SCAN_MAX_AP)
        {
            break;
        }
    }

    cJSON_AddBoolToObject(root, "ok", true);
    cJSON_AddNumberToObject(root, "count", unique_count);
    ESP_LOGI(TAG,
             "Provision scan done: %u AP(s), free_heap=%" PRIu32 ", stack_hw=%" PRIu32 " words",
             (unsigned int)unique_count,
             (uint32_t)esp_get_free_heap_size(),
             (uint32_t)uxTaskGetStackHighWaterMark(NULL));
    goto done;

fail:
    cJSON_AddBoolToObject(root, "ok", false);
    if (fail_reason != NULL)
    {
        cJSON_AddStringToObject(root, "reason", fail_reason);
    }
    else
    {
        cJSON_AddStringToObject(root, "reason", esp_err_to_name(ret));
    }

done:
    if (scan_lock_taken)
    {
        xSemaphoreGive(s_scan_mutex);
    }
    free(ap_records);
    free(seen_ssids);
    return send_json_response(req, root);
}

static esp_err_t provision_save_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Provision /provision requested");

    if (req->content_len <= 0 || req->content_len > PROV_HTML_MAX_BODY)
    {
        httpd_resp_set_status(req, "400 Bad Request");
        return httpd_resp_sendstr(req, "{\"ok\":false,\"reason\":\"invalid content length\"}");
    }

    char body[PROV_HTML_MAX_BODY + 1];
    int received = 0;
    while (received < req->content_len)
    {
        int ret = httpd_req_recv(req, body + received, req->content_len - received);
        if (ret <= 0)
        {
            httpd_resp_set_status(req, "400 Bad Request");
            return httpd_resp_sendstr(req, "{\"ok\":false,\"reason\":\"recv failed\"}");
        }
        received += ret;
    }
    body[received] = '\0';

    cJSON *root = cJSON_Parse(body);
    if (root == NULL)
    {
        httpd_resp_set_status(req, "400 Bad Request");
        return httpd_resp_sendstr(req, "{\"ok\":false,\"reason\":\"invalid json\"}");
    }

    cJSON *ssid = cJSON_GetObjectItem(root, "ssid");
    cJSON *password = cJSON_GetObjectItem(root, "password");
    if (!cJSON_IsString(ssid) || !cJSON_IsString(password))
    {
        cJSON_Delete(root);
        httpd_resp_set_status(req, "400 Bad Request");
        return httpd_resp_sendstr(req, "{\"ok\":false,\"reason\":\"ssid/password required\"}");
    }

    char ssid_clean[33] = {0};
    char pass_clean[65] = {0};
    strncpy(ssid_clean, ssid->valuestring, sizeof(ssid_clean) - 1);
    strncpy(pass_clean, password->valuestring, sizeof(pass_clean) - 1);
    trim_inplace(ssid_clean);

    if (ssid_clean[0] == '\0')
    {
        cJSON_Delete(root);
        httpd_resp_set_status(req, "400 Bad Request");
        return httpd_resp_sendstr(req, "{\"ok\":false,\"reason\":\"ssid empty\"}");
    }

    esp_err_t ret = wifi_manager_set_credentials(ssid_clean, pass_clean, false);
    cJSON_Delete(root);
    if (ret != ESP_OK)
    {
        char err_body[96];
        snprintf(err_body, sizeof(err_body), "{\"ok\":false,\"reason\":\"%s\"}", esp_err_to_name(ret));
        httpd_resp_set_status(req, "400 Bad Request");
        return httpd_resp_sendstr(req, err_body);
    }

    wifi_credentials_t verify = {0};
    esp_err_t verify_ret = load_credentials_from_nvs(&verify);
    if (verify_ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Provision saved SSID to NVS: \"%s\"", verify.ssid);
    }
    else
    {
        ESP_LOGW(TAG, "Provision save verify read failed: %s", esp_err_to_name(verify_ret));
    }

    if (!s_provision_switch_pending)
    {
        s_provision_switch_pending = true;
        BaseType_t create_ok = xTaskCreate(provision_switch_task, "prov_switch", 4096, NULL, 5, NULL);
        if (create_ok != pdPASS)
        {
            s_provision_switch_pending = false;
            httpd_resp_set_status(req, "500 Internal Server Error");
            return httpd_resp_sendstr(req, "{\"ok\":false,\"reason\":\"prov switch task create failed\"}");
        }
    }

    set_http_no_cache_headers(req);
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    return httpd_resp_sendstr(req, "{\"ok\":true,\"message\":\"saved, connecting...\"}");
}

static esp_err_t start_provision_http_server(void)
{
    if (s_provision_httpd != NULL)
    {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_uri_handlers = 8;
    // Use adaptive stack to reduce "ESP_ERR_HTTPD_TASK" under memory pressure.
    static const uint16_t stack_candidates[] = {8192, 6144, 4096};

    esp_err_t ret = ESP_FAIL;
    for (size_t i = 0; i < sizeof(stack_candidates) / sizeof(stack_candidates[0]); ++i)
    {
        config.stack_size = stack_candidates[i];
        uint32_t free_internal = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        uint32_t largest_internal = (uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
        ESP_LOGI(TAG,
                 "Try start provision HTTPD (stack=%u, free_internal=%" PRIu32 ", largest_internal=%" PRIu32 ")",
                 (unsigned int)config.stack_size,
                 free_internal,
                 largest_internal);

        ret = httpd_start(&s_provision_httpd, &config);
        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "Provision HTTPD started (stack=%u)", (unsigned int)config.stack_size);
            break;
        }

        ESP_LOGW(TAG, "httpd_start failed (stack=%u): %s", (unsigned int)config.stack_size, esp_err_to_name(ret));
        s_provision_httpd = NULL;
    }

    if (ret != ESP_OK)
    {
        return ret;
    }

    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = provision_root_handler,
        .user_ctx = NULL};
    httpd_uri_t status = {
        .uri = "/status",
        .method = HTTP_GET,
        .handler = provision_status_handler,
        .user_ctx = NULL};
    httpd_uri_t save = {
        .uri = "/provision",
        .method = HTTP_POST,
        .handler = provision_save_handler,
        .user_ctx = NULL};
    httpd_uri_t scan = {
        .uri = "/scan",
        .method = HTTP_GET,
        .handler = provision_scan_handler,
        .user_ctx = NULL};

    ret = httpd_register_uri_handler(s_provision_httpd, &root);
    if (ret != ESP_OK)
    {
        httpd_stop(s_provision_httpd);
        s_provision_httpd = NULL;
        return ret;
    }
    ret = httpd_register_uri_handler(s_provision_httpd, &status);
    if (ret != ESP_OK)
    {
        httpd_stop(s_provision_httpd);
        s_provision_httpd = NULL;
        return ret;
    }
    ret = httpd_register_uri_handler(s_provision_httpd, &save);
    if (ret != ESP_OK)
    {
        httpd_stop(s_provision_httpd);
        s_provision_httpd = NULL;
        return ret;
    }
    ret = httpd_register_uri_handler(s_provision_httpd, &scan);
    if (ret != ESP_OK)
    {
        httpd_stop(s_provision_httpd);
        s_provision_httpd = NULL;
        return ret;
    }

    return ESP_OK;
}

esp_err_t wifi_manager_start_provisioning(void)
{
    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_provisioning)
    {
        return ESP_OK;
    }

    esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (ret != ESP_OK)
    {
        return ret;
    }
    ret = esp_wifi_disconnect();
    if (ret != ESP_OK && ret != ESP_ERR_WIFI_NOT_CONNECT)
    {
        ESP_LOGW(TAG, "esp_wifi_disconnect before provisioning failed: %s", esp_err_to_name(ret));
    }

    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    snprintf(s_provision_ap_ssid, sizeof(s_provision_ap_ssid), "SmartIrrigation-%02X%02X", mac[4], mac[5]);

    wifi_config_t ap_cfg = {0};
    strncpy((char *)ap_cfg.ap.ssid, s_provision_ap_ssid, sizeof(ap_cfg.ap.ssid) - 1);
    strncpy((char *)ap_cfg.ap.password, PROV_AP_PASS, sizeof(ap_cfg.ap.password) - 1);
    ap_cfg.ap.ssid_len = strlen((char *)ap_cfg.ap.ssid);
    ap_cfg.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    ap_cfg.ap.max_connection = 4;
    ap_cfg.ap.channel = 1;

    ret = esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (!s_wifi_started)
    {
        ret = esp_wifi_start();
        if (ret != ESP_OK)
        {
            return ret;
        }
        s_wifi_started = true;
    }

    if (s_ap_netif == NULL)
    {
        ESP_LOGE(TAG, "AP netif is NULL before provisioning");
        return ESP_ERR_INVALID_STATE;
    }

    esp_netif_dhcp_status_t dhcps = ESP_NETIF_DHCP_INIT;
    ret = esp_netif_dhcps_get_status(s_ap_netif, &dhcps);
    if (ret == ESP_OK && dhcps != ESP_NETIF_DHCP_STARTED)
    {
        ret = esp_netif_dhcps_start(s_ap_netif);
        if (ret != ESP_OK && ret != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED)
        {
            ESP_LOGW(TAG, "esp_netif_dhcps_start(AP) failed: %s", esp_err_to_name(ret));
        }
    }

    ret = start_provision_http_server();
    if (ret != ESP_OK)
    {
        return ret;
    }

    s_provisioning = true;
    s_no_ap_disconnect_count = 0;
    s_provisioning_start_count++;
    s_wifi_connected = false;
    memset(&s_current_ip, 0, sizeof(s_current_ip));
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

    ESP_LOGW(TAG, "No Wi-Fi profile. Provisioning AP started:");
    ESP_LOGW(TAG, "AP SSID: %s", s_provision_ap_ssid);
    ESP_LOGW(TAG, "AP Password: %s", PROV_AP_PASS);
    ESP_LOGW(TAG, "Open: http://192.168.4.1");
    dump_ap_netif_status("provisioning_started");

    return ESP_OK;
}

static void reconnect_timer_cb(TimerHandle_t timer)
{
    (void)timer;
    if (s_provisioning)
    {
        return;
    }
    esp_err_t ret = esp_wifi_connect();
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "esp_wifi_connect failed in reconnect timer: %s", esp_err_to_name(ret));
    }
}

static void auto_provision_task(void *arg)
{
    const char *reason = (const char *)arg;
    ESP_LOGW(TAG, "Auto fallback to provisioning, reason=%s", reason ? reason : "unknown");

    esp_err_t ret = wifi_manager_start_provisioning();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "Auto provisioning start failed: %s", esp_err_to_name(ret));
    }

    s_auto_provision_pending = false;
    vTaskDelete(NULL);
}

static void request_auto_provision(const char *reason)
{
    if (s_provisioning || s_auto_provision_pending)
    {
        return;
    }

    s_auto_provision_pending = true;
    BaseType_t ok = xTaskCreate(auto_provision_task,
                                "wifi_auto_prov",
                                4096,
                                (void *)reason,
                                5,
                                NULL);
    if (ok != pdPASS)
    {
        s_auto_provision_pending = false;
        ESP_LOGE(TAG, "Failed to create wifi_auto_prov task");
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START)
    {
        ESP_LOGI(TAG, "WIFI_EVENT_AP_START");
        dump_ap_netif_status("ap_start");
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        const wifi_event_ap_staconnected_t *ev = (const wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "WIFI_EVENT_AP_STACONNECTED: " MACSTR ", AID=%d", MAC2STR(ev->mac), ev->aid);
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        const wifi_event_ap_stadisconnected_t *ev = (const wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGW(TAG, "WIFI_EVENT_AP_STADISCONNECTED: " MACSTR ", AID=%d", MAC2STR(ev->mac), ev->aid);
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        const wifi_event_sta_connected_t *ev = (const wifi_event_sta_connected_t *)event_data;
        ESP_LOGI(TAG,
                 "WIFI_EVENT_STA_CONNECTED: ssid=\"%.*s\" channel=%u",
                 (int)ev->ssid_len,
                 ev->ssid,
                 (unsigned int)ev->channel);
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        if (!s_provisioning && has_valid_credentials(&s_active_credentials))
        {
            ESP_LOGI(TAG, "WIFI_EVENT_STA_START, connecting to SSID \"%s\"", s_active_credentials.ssid);
            esp_wifi_connect();
        }
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        s_wifi_connected = false;
        memset(&s_current_ip, 0, sizeof(s_current_ip));
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        s_disconnect_count++;

        if (s_provisioning || !has_valid_credentials(&s_active_credentials))
        {
            return;
        }

        const wifi_event_sta_disconnected_t *disc = (const wifi_event_sta_disconnected_t *)event_data;
        int disc_reason = disc ? disc->reason : -1;
        uint32_t delay_ms = calc_reconnect_delay_ms(s_reconnect_attempts);
        s_reconnect_attempts++;
        s_last_reconnect_delay_ms = delay_ms;

        ESP_LOGW(TAG,
                 "Disconnected (reason=%d), retry=%" PRIu32 ", next reconnect in %" PRIu32 " ms",
                 disc_reason,
                 s_reconnect_attempts,
                 delay_ms);

        if (disc_reason == WIFI_REASON_NO_AP_FOUND)
        {
            s_no_ap_disconnect_count++;
            if (s_no_ap_disconnect_count >= PROV_NO_AP_QUICK_FALLBACK_ATTEMPTS)
            {
                ESP_LOGW(TAG,
                         "Saved SSID \"%s\" not found around device (no_ap_hits=%" PRIu32 "), entering provisioning mode",
                         s_active_credentials.ssid,
                         s_no_ap_disconnect_count);
                xTimerStop(s_reconnect_timer, 0);
                request_auto_provision("no_ap_found");
                return;
            }
        }
        else
        {
            s_no_ap_disconnect_count = 0;
        }

        uint32_t fallback_threshold = (s_connect_success_count == 0U)
                                          ? PROV_FALLBACK_BOOT_ATTEMPTS
                                          : PROV_FALLBACK_RUNTIME_ATTEMPTS;
        if (s_reconnect_attempts >= fallback_threshold)
        {
            ESP_LOGW(TAG,
                     "Reconnect attempts reached threshold (%" PRIu32 "), entering provisioning mode",
                     fallback_threshold);
            xTimerStop(s_reconnect_timer, 0);
            request_auto_provision("sta_reconnect_exhausted");
            return;
        }

        xTimerStop(s_reconnect_timer, 0);
        xTimerChangePeriod(s_reconnect_timer, pdMS_TO_TICKS(delay_ms), 0);
        xTimerStart(s_reconnect_timer, 0);
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        const ip_event_got_ip_t *event = (const ip_event_got_ip_t *)event_data;
        s_wifi_connected = true;
        s_auto_provision_pending = false;
        s_reconnect_attempts = 0;
        s_no_ap_disconnect_count = 0;
        s_connect_success_count++;
        s_current_ip = event->ip_info.ip;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        ESP_LOGI(TAG, "Connected, got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        uint32_t delay_ms = (s_time_sync_ok_count == 0) ? TIME_SYNC_INITIAL_DELAY_MS : 0;
        schedule_time_sync(delay_ms);
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_AP_STAIPASSIGNED)
    {
        const ip_event_ap_staipassigned_t *ev = (const ip_event_ap_staipassigned_t *)event_data;
        ESP_LOGI(TAG, "IP_EVENT_AP_STAIPASSIGNED: " MACSTR " -> " IPSTR, MAC2STR(ev->mac), IP2STR(&ev->ip));
        return;
    }
}

esp_err_t wifi_manager_init(void)
{
    esp_err_t ret = ensure_nvs_ready();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    if (!s_initialized)
    {
        s_wifi_event_group = xEventGroupCreate();
        if (s_wifi_event_group == NULL)
        {
            ESP_LOGE(TAG, "Failed to create wifi event group");
            return ESP_ERR_NO_MEM;
        }

        s_reconnect_timer = xTimerCreate(
            "wifi_reconn",
            pdMS_TO_TICKS(RECONNECT_BASE_DELAY_MS),
            pdFALSE,
            NULL,
            reconnect_timer_cb);
        if (s_reconnect_timer == NULL)
        {
            ESP_LOGE(TAG, "Failed to create wifi reconnect timer");
            return ESP_ERR_NO_MEM;
        }

        s_scan_mutex = xSemaphoreCreateMutex();
        if (s_scan_mutex == NULL)
        {
            ESP_LOGE(TAG, "Failed to create scan mutex");
            return ESP_ERR_NO_MEM;
        }

        s_threshold_sync_mutex = xSemaphoreCreateMutex();
        if (s_threshold_sync_mutex == NULL)
        {
            ESP_LOGE(TAG, "Failed to create threshold sync mutex");
            return ESP_ERR_NO_MEM;
        }

        s_threshold_sync_timer = xTimerCreate(
            "th_sync",
            pdMS_TO_TICKS(THRESHOLD_SYNC_ACK_TIMEOUT_MS),
            pdFALSE,
            NULL,
            threshold_sync_timer_cb);
        if (s_threshold_sync_timer == NULL)
        {
            ESP_LOGE(TAG, "Failed to create threshold sync timer");
            return ESP_ERR_NO_MEM;
        }

        ret = esp_netif_init();
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
        {
            ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(ret));
            return ret;
        }

        ret = esp_event_loop_create_default();
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
        {
            ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(ret));
            return ret;
        }

        if (s_sta_netif == NULL)
        {
            s_sta_netif = esp_netif_create_default_wifi_sta();
        }
        if (s_ap_netif == NULL)
        {
            s_ap_netif = esp_netif_create_default_wifi_ap();
        }
        if (s_sta_netif == NULL || s_ap_netif == NULL)
        {
            ESP_LOGE(TAG, "Failed to create default wifi netifs (sta=%p ap=%p)", s_sta_netif, s_ap_netif);
            return ESP_FAIL;
        }

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ret = esp_wifi_init(&cfg);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
        {
            ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(ret));
            return ret;
        }

        ret = esp_wifi_set_country_code("01", true);
        if (ret != ESP_OK)
        {
            ESP_LOGW(TAG, "esp_wifi_set_country_code(01) failed: %s", esp_err_to_name(ret));
        }

        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL,
            &s_wifi_evt_handle));

        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            IP_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL,
            &s_ip_evt_handle));

        s_initialized = true;
    }

    memset(&s_active_credentials, 0, sizeof(s_active_credentials));
    ret = load_credentials_from_nvs(&s_active_credentials);
    if (ret != ESP_OK || !has_valid_credentials(&s_active_credentials))
    {
        ESP_LOGW(TAG, "No saved Wi-Fi profile, entering provisioning mode");
        return wifi_manager_start_provisioning();
    }
    else
    {
        ESP_LOGI(TAG, "Using saved Wi-Fi profile from NVS (SSID=\"%s\")", s_active_credentials.ssid);
    }

    ret = start_sta_connection();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start STA connection: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Wi-Fi manager initialized");
    return ESP_OK;
}

bool wifi_manager_is_connected(void)
{
    return s_wifi_connected;
}

bool wifi_manager_is_provisioning(void)
{
    return s_provisioning;
}

esp_err_t wifi_manager_get_credentials(wifi_credentials_t *out)
{
    if (out == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *out = s_active_credentials;
    return ESP_OK;
}

esp_err_t wifi_manager_get_sta_ip_string(char *out, size_t out_len)
{
    if (out == NULL || out_len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_wifi_connected)
    {
        out[0] = '\0';
        return ESP_ERR_INVALID_STATE;
    }

    int n = snprintf(out, out_len, IPSTR, IP2STR(&s_current_ip));
    if (n < 0 || (size_t)n >= out_len)
    {
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

esp_err_t wifi_manager_get_provision_ap_ssid(char *out, size_t out_len)
{
    if (out == NULL || out_len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_provision_ap_ssid[0] == '\0')
    {
        out[0] = '\0';
        return ESP_ERR_INVALID_STATE;
    }

    if (strlen(s_provision_ap_ssid) + 1 > out_len)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    strcpy(out, s_provision_ap_ssid);
    return ESP_OK;
}

esp_err_t wifi_manager_get_provision_ap_password(char *out, size_t out_len)
{
    if (out == NULL || out_len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (strlen(PROV_AP_PASS) + 1 > out_len)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    strcpy(out, PROV_AP_PASS);
    return ESP_OK;
}

esp_err_t wifi_manager_set_credentials(const char *ssid, const char *password, bool reconnect_now)
{
    if (ssid == NULL || ssid[0] == '\0' || password == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(ssid) >= sizeof(s_active_credentials.ssid) ||
        strlen(password) >= sizeof(s_active_credentials.password))
    {
        return ESP_ERR_INVALID_SIZE;
    }

    memset(&s_active_credentials, 0, sizeof(s_active_credentials));
    strncpy(s_active_credentials.ssid, ssid, sizeof(s_active_credentials.ssid) - 1);
    strncpy(s_active_credentials.password, password, sizeof(s_active_credentials.password) - 1);

    esp_err_t ret = save_credentials_to_nvs(&s_active_credentials);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to persist Wi-Fi profile: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Wi-Fi credentials updated (SSID=\"%s\")", s_active_credentials.ssid);

    if (s_initialized && reconnect_now)
    {
        ret = start_sta_connection();
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to apply updated Wi-Fi profile: %s", esp_err_to_name(ret));
            return ret;
        }
    }

    return ESP_OK;
}

esp_err_t wifi_manager_clear_credentials(void)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NETCFG_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK)
    {
        return ret;
    }

    nvs_erase_key(handle, NVS_KEY_WIFI_SSID);
    nvs_erase_key(handle, NVS_KEY_WIFI_PASS);
    ret = nvs_commit(handle);
    nvs_close(handle);

    if (ret == ESP_OK)
    {
        memset(&s_active_credentials, 0, sizeof(s_active_credentials));
        ESP_LOGI(TAG, "Cleared saved Wi-Fi credentials from NVS");
    }
    return ret;
}

bool wifi_manager_time_is_synced(void)
{
    return s_time_synced;
}

esp_err_t wifi_manager_get_local_time_string(char *out, size_t out_len)
{
    if (out == NULL || out_len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!get_local_time_string_internal(out, out_len))
    {
        out[0] = '\0';
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

esp_err_t wifi_manager_request_time_sync(const char *reason)
{
    if (!s_wifi_connected)
    {
        return ESP_ERR_INVALID_STATE;
    }

    TickType_t now = xTaskGetTickCount();
    bool in_cooldown = (s_last_sync_push_tick != 0 &&
                        (now - s_last_sync_push_tick) < pdMS_TO_TICKS(TIME_SYNC_PUSH_COOLDOWN_MS));
    bool allow_retry_once = false;

    if (in_cooldown &&
        reason != NULL &&
        strcmp(reason, "stm32_timestamp_skew") == 0 &&
        s_sync_info_wait_ack &&
        !s_sync_info_ack_success &&
        !s_sync_info_retry_used)
    {
        allow_retry_once = true;
        s_sync_info_retry_used = true;
        ESP_LOGW(TAG, "Bypass sync cooldown once for skew because sync_info ack is pending");
    }

    if (in_cooldown && !allow_retry_once)
    {
        ESP_LOGI(TAG, "Skip time sync request (cooldown), reason=%s", reason ? reason : "manual");
        return ESP_OK;
    }

    if (s_time_synced)
    {
        char ts[20] = {0};
        if (get_local_time_string_internal(ts, sizeof(ts)))
        {
            return sync_time_to_stm32(reason);
        }
    }

    schedule_time_sync(0);
    ESP_LOGI(TAG, "Queued SNTP sync task, reason=%s", reason ? reason : "manual");
    return ESP_OK;
}

void wifi_manager_notify_stm_sync_ack(const char *trace_id, const char *ack_status)
{
    bool success = (ack_status != NULL && strcmp(ack_status, "success") == 0);
    if (success)
    {
        s_sync_info_wait_ack = false;
        s_sync_info_ack_success = true;
        s_sync_info_retry_used = false;
        s_sync_info_ack_ok_count++;
        ESP_LOGI(TAG,
                 "Time synced to STM32 confirmed by config_ack_report: trace=%s ack_status=%s",
                 trace_id ? trace_id : "none",
                 ack_status);
        return;
    }

    s_sync_info_ack_fail_count++;
    ESP_LOGW(TAG,
             "sync_info ack received but not success: trace=%s ack_status=%s",
             trace_id ? trace_id : "none",
             ack_status ? ack_status : "null");
}

esp_err_t wifi_manager_request_threshold_sync(const char *reason)
{
    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_threshold_sync_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(s_threshold_sync_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }

    if (s_threshold_sync_session_active)
    {
        ESP_LOGW(TAG,
                 "threshold_sync override previous session trace=%s reason=%s",
                 s_threshold_sync_trace_id[0] ? s_threshold_sync_trace_id : "none",
                 threshold_sync_reason_text());
    }

    threshold_sync_load_config(&s_threshold_sync_lower, &s_threshold_sync_upper);
    threshold_sync_copy_reason(reason);
    threshold_sync_clear_session();
    s_threshold_sync_session_active = true;

    if (s_threshold_sync_timer != NULL)
    {
        (void)xTimerStop(s_threshold_sync_timer, 0);
    }

    esp_err_t ret = threshold_sync_send_next_attempt_locked();
    if (ret != ESP_OK)
    {
        threshold_sync_schedule_retry_locked("request");
        if (s_threshold_sync_session_active)
        {
            ret = ESP_OK;
        }
    }

    xSemaphoreGive(s_threshold_sync_mutex);
    return ret;
}

void wifi_manager_notify_stm_config_ack(const char *ack_type,
                                        const char *origin_trace_id,
                                        const char *ack_status,
                                        int result_code)
{
    if (ack_type == NULL || strcmp(ack_type, "config_command") != 0)
    {
        return;
    }
    if (s_threshold_sync_mutex == NULL)
    {
        return;
    }
    if (xSemaphoreTake(s_threshold_sync_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        return;
    }

    if (!s_threshold_sync_session_active || !s_threshold_sync_wait_ack)
    {
        ESP_LOGW(TAG,
                 "threshold_sync stray ack origin=%s status=%s code=%d",
                 origin_trace_id ? origin_trace_id : "none",
                 ack_status ? ack_status : "none",
                 result_code);
        xSemaphoreGive(s_threshold_sync_mutex);
        return;
    }

    if (origin_trace_id == NULL || strcmp(origin_trace_id, s_threshold_sync_trace_id) != 0)
    {
        ESP_LOGW(TAG,
                 "threshold_sync ack trace mismatch origin=%s expected=%s",
                 origin_trace_id ? origin_trace_id : "none",
                 s_threshold_sync_trace_id);
        xSemaphoreGive(s_threshold_sync_mutex);
        return;
    }

    bool success = (ack_status != NULL && strcmp(ack_status, "success") == 0);
    s_threshold_sync_wait_ack = false;
    if (s_threshold_sync_timer != NULL)
    {
        (void)xTimerStop(s_threshold_sync_timer, 0);
    }

    if (success)
    {
        s_threshold_sync_ack_ok_count++;
        ESP_LOGI(TAG,
                 "threshold_sync ack success trace=%s attempt=%u/%u reason=%s",
                 s_threshold_sync_trace_id,
                 (unsigned)s_threshold_sync_attempt,
                 (unsigned)THRESHOLD_SYNC_MAX_ATTEMPTS,
                 threshold_sync_reason_text());
        threshold_sync_clear_session();
        xSemaphoreGive(s_threshold_sync_mutex);
        return;
    }

    s_threshold_sync_ack_fail_count++;
    ESP_LOGW(TAG,
             "threshold_sync ack failed trace=%s status=%s code=%d attempt=%u/%u reason=%s",
             s_threshold_sync_trace_id,
             ack_status ? ack_status : "none",
             result_code,
             (unsigned)s_threshold_sync_attempt,
             (unsigned)THRESHOLD_SYNC_MAX_ATTEMPTS,
             threshold_sync_reason_text());
    threshold_sync_schedule_retry_locked("ack_failed");
    xSemaphoreGive(s_threshold_sync_mutex);
}

void wifi_manager_dump_status(void)
{
    if (s_provisioning)
    {
        ESP_LOGI(TAG,
                 "status=provisioning ap_ssid=\"%s\" starts=%" PRIu32 " disconnects=%" PRIu32 " time_synced=%d sync_ok=%" PRIu32 " sync_fail=%" PRIu32 " sync_wait_ack=%d sync_sent=%" PRIu32 " sync_ack_ok=%" PRIu32 " sync_ack_fail=%" PRIu32 " th_wait_ack=%d th_sent=%" PRIu32 " th_ack_ok=%" PRIu32 " th_ack_fail=%" PRIu32 " th_timeout=%" PRIu32 " th_giveup=%" PRIu32,
                 s_provision_ap_ssid,
                 s_provisioning_start_count,
                 s_disconnect_count,
                 s_time_synced,
                 s_time_sync_ok_count,
                 s_time_sync_fail_count,
                 s_sync_info_wait_ack,
                 s_sync_info_sent_count,
                 s_sync_info_ack_ok_count,
                 s_sync_info_ack_fail_count,
                 s_threshold_sync_wait_ack,
                 s_threshold_sync_sent_count,
                 s_threshold_sync_ack_ok_count,
                 s_threshold_sync_ack_fail_count,
                 s_threshold_sync_timeout_count,
                 s_threshold_sync_giveup_count);
        return;
    }

    if (s_wifi_connected)
    {
        ESP_LOGI(TAG,
                 "status=connected ssid=\"%s\" ip=" IPSTR " connect_ok=%" PRIu32 " disconnects=%" PRIu32 " time_synced=%d sync_ok=%" PRIu32 " sync_fail=%" PRIu32 " sync_wait_ack=%d sync_sent=%" PRIu32 " sync_ack_ok=%" PRIu32 " sync_ack_fail=%" PRIu32 " th_wait_ack=%d th_sent=%" PRIu32 " th_ack_ok=%" PRIu32 " th_ack_fail=%" PRIu32 " th_timeout=%" PRIu32 " th_giveup=%" PRIu32 " last_sync=\"%s\"",
                 s_active_credentials.ssid,
                 IP2STR(&s_current_ip),
                 s_connect_success_count,
                 s_disconnect_count,
                 s_time_synced,
                 s_time_sync_ok_count,
                 s_time_sync_fail_count,
                 s_sync_info_wait_ack,
                 s_sync_info_sent_count,
                 s_sync_info_ack_ok_count,
                 s_sync_info_ack_fail_count,
                 s_threshold_sync_wait_ack,
                 s_threshold_sync_sent_count,
                 s_threshold_sync_ack_ok_count,
                 s_threshold_sync_ack_fail_count,
                 s_threshold_sync_timeout_count,
                 s_threshold_sync_giveup_count,
                 s_last_time_sync_text[0] != '\0' ? s_last_time_sync_text : "none");
    }
    else
    {
        ESP_LOGI(TAG,
                 "status=disconnected ssid=\"%s\" reconnect_attempts=%" PRIu32 " last_backoff_ms=%" PRIu32 " disconnects=%" PRIu32 " time_synced=%d sync_ok=%" PRIu32 " sync_fail=%" PRIu32 " sync_wait_ack=%d sync_sent=%" PRIu32 " sync_ack_ok=%" PRIu32 " sync_ack_fail=%" PRIu32 " th_wait_ack=%d th_sent=%" PRIu32 " th_ack_ok=%" PRIu32 " th_ack_fail=%" PRIu32 " th_timeout=%" PRIu32 " th_giveup=%" PRIu32 " last_sync=\"%s\"",
                 s_active_credentials.ssid,
                 s_reconnect_attempts,
                 s_last_reconnect_delay_ms,
                 s_disconnect_count,
                 s_time_synced,
                 s_time_sync_ok_count,
                 s_time_sync_fail_count,
                 s_sync_info_wait_ack,
                 s_sync_info_sent_count,
                 s_sync_info_ack_ok_count,
                 s_sync_info_ack_fail_count,
                 s_threshold_sync_wait_ack,
                 s_threshold_sync_sent_count,
                 s_threshold_sync_ack_ok_count,
                 s_threshold_sync_ack_fail_count,
                 s_threshold_sync_timeout_count,
                 s_threshold_sync_giveup_count,
                 s_last_time_sync_text[0] != '\0' ? s_last_time_sync_text : "none");
    }
}
