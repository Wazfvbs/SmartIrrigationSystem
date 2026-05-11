#include "menu_ui.h"

#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "lvgl.h"

#include "../app_mqtt/app_mqtt.h"
#include "../config_manager/config_manager.h"
#include "../stm32_protocol/stm32_protocol.h"
#include "../uart_receiver/uart_receiver.h"
#include "../wifi_manager/wifi_manager.h"
#include "default_ui.h"

#define TAG "MENU_UI"
#define MENU_MAX_ITEMS 8
#define MENU_TIMEOUT_MS 20000
#define ACTION_HINT_MS 4000
#define MENU_MAIN_LIST_TOP 42
#define MENU_MAIN_LIST_STEP 20
#define MENU_SUB_LIST_GAP 8
#define MENU_SUB_MIN_STEP 12

typedef enum
{
    MENU_PAGE_MAIN = 0,
    MENU_PAGE_MANUAL,
    MENU_PAGE_MODE,
    MENU_PAGE_THRESHOLD,
    MENU_PAGE_NETWORK,
    MENU_PAGE_DEVICE_STATUS,
    MENU_PAGE_SYSTEM_INFO,
    MENU_PAGE_CONFIRM,
} menu_page_t;

typedef enum
{
    CONFIRM_NONE = 0,
    CONFIRM_START_WATER,
    CONFIRM_CLEAR_WIFI,
    CONFIRM_RESTORE_DEFAULT,
    CONFIRM_REBOOT,
} confirm_action_t;

typedef enum
{
    UI_STATE_NO_DATA = 0,
    UI_STATE_PROVISIONING,
    UI_STATE_NETWORK_FAULT,
    UI_STATE_ALERT,
    UI_STATE_NORMAL
} ui_state_t;

static lv_obj_t *scr = NULL;
static lv_obj_t *panel_header = NULL;
static lv_obj_t *lbl_title = NULL;
static lv_obj_t *lbl_time = NULL;
static lv_obj_t *panel_content = NULL;
static lv_obj_t *lbl_section = NULL;
static lv_obj_t *lbl_info = NULL;
static lv_obj_t *lbl_items[MENU_MAX_ITEMS] = {0};
static lv_obj_t *panel_hint = NULL;
static lv_obj_t *lbl_hint = NULL;

static lv_color_t s_item_selected_bg = {0};
static lv_color_t s_item_selected_fg = {0};
static lv_color_t s_item_selected_border = {0};

static menu_page_t s_page = MENU_PAGE_MAIN;
static int s_main_index = 0;
static int s_sub_index = 0;
static int s_entry_main_index = 0;
static int s_page_item_count = 0;
static bool s_threshold_editing = false;
static TickType_t s_last_input_tick = 0;
static TickType_t s_action_hint_until = 0;
static TickType_t s_last_stm_data_tick = 0;
static char s_last_stm_trace[32] = {0};
static char s_action_hint[96] = {0};
static char s_confirm_text[96] = {0};
static confirm_action_t s_confirm_action = CONFIRM_NONE;
static menu_page_t s_confirm_return_page = MENU_PAGE_MAIN;
static int s_confirm_return_index = 0;

static int s_threshold_lower = 30;
static int s_threshold_upper = 70;
static int s_water_level_alarm = 20;
static int s_watering_duration_sec = 10;

static const char *k_main_items[] = {
    "Manual Control",
    "Mode Settings",
    "Threshold Settings",
    "Network Settings",
    "Device Status",
    "System Info",
};

static const char *menu_page_to_protocol(menu_page_t page)
{
    switch (page)
    {
    case MENU_PAGE_MAIN:
        return "page_menu_main";
    case MENU_PAGE_MANUAL:
        return "page_menu_manual";
    case MENU_PAGE_MODE:
        return "page_menu_mode";
    case MENU_PAGE_THRESHOLD:
        return "page_menu_threshold";
    case MENU_PAGE_NETWORK:
        return "page_menu_network";
    case MENU_PAGE_DEVICE_STATUS:
        return "page_menu_device_status";
    case MENU_PAGE_SYSTEM_INFO:
        return "page_menu_system_info";
    case MENU_PAGE_CONFIRM:
        return "page_menu_confirm";
    default:
        return "page_menu_unknown";
    }
}

static void report_menu_state_to_stm32(const char *action, bool require_ack)
{
    (void)stm32_protocol_send_menu_command(action,
                                           menu_page_to_protocol(s_page),
                                           s_main_index,
                                           s_sub_index,
                                           s_threshold_editing,
                                           require_ack,
                                           NULL);
}

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

static bool water_low_from_text(const char *water_text)
{
    if (water_text == NULL)
    {
        return false;
    }
    return contains_token_ci(water_text, "low") ||
           contains_token_ci(water_text, "empty") ||
           contains_token_ci(water_text, "lack");
}

static bool cloud_has_fault(const cloud_http_status_t *cloud)
{
    if (cloud == NULL || !cloud->started)
    {
        return false;
    }
    return cloud->last_error_text[0] != '\0' &&
           strcmp(cloud->last_error_text, "none") != 0;
}

static bool is_alert_state(const sensor_data_t *d, const device_config_t *cfg)
{
    if (d == NULL)
    {
        return false;
    }
    if (cfg != NULL && d->soil < (float)cfg->threshold_lower)
    {
        return true;
    }
    if (water_low_from_text(d->water))
    {
        return true;
    }
    return false;
}

static ui_state_t decide_menu_state(const sensor_data_t *d,
                                    const device_config_t *cfg,
                                    const cloud_http_status_t *cloud)
{
    if (wifi_manager_is_provisioning())
    {
        return UI_STATE_PROVISIONING;
    }
    if (d == NULL)
    {
        return UI_STATE_NO_DATA;
    }
    if (!wifi_manager_is_connected())
    {
        return UI_STATE_NETWORK_FAULT;
    }
    if (cloud != NULL &&
        cloud->started &&
        cloud->last_error_text[0] != '\0' &&
        strcmp(cloud->last_error_text, "none") != 0)
    {
        return UI_STATE_NETWORK_FAULT;
    }
    if (cfg != NULL && d->soil < (float)cfg->threshold_lower)
    {
        return UI_STATE_ALERT;
    }
    if (water_low_from_text(d->water))
    {
        return UI_STATE_ALERT;
    }
    return UI_STATE_NORMAL;
}

static void apply_menu_theme(ui_state_t state)
{
    lv_color_t bg_top = lv_color_hex(0xE8F5FF);
    lv_color_t bg_bottom = lv_color_hex(0xD9F0EA);
    lv_color_t hint_bg = lv_color_hex(0xF6FBFF);
    lv_color_t hint_fg = lv_color_hex(0x213142);
    lv_color_t hint_border = lv_color_hex(0xD3DEEA);
    lv_color_t selected_bg = lv_color_hex(0xE9F4FF);
    lv_color_t selected_fg = lv_color_hex(0x12304A);
    lv_color_t selected_border = lv_color_hex(0x8FB8E4);

    switch (state)
    {
    case UI_STATE_PROVISIONING:
        bg_top = lv_color_hex(0xFFF7E8);
        bg_bottom = lv_color_hex(0xEAF3FF);
        hint_bg = lv_color_hex(0xFFF3DD);
        hint_border = lv_color_hex(0xEED4A8);
        selected_bg = lv_color_hex(0xFFECCB);
        selected_fg = lv_color_hex(0x6B4300);
        selected_border = lv_color_hex(0xE4BA73);
        break;
    case UI_STATE_NETWORK_FAULT:
        bg_top = lv_color_hex(0xFFEDEE);
        bg_bottom = lv_color_hex(0xFFE2E4);
        hint_bg = lv_color_hex(0xFFE1E6);
        hint_border = lv_color_hex(0xE8B7C0);
        selected_bg = lv_color_hex(0xFFDCE4);
        selected_fg = lv_color_hex(0x5C1F2A);
        selected_border = lv_color_hex(0xE08AA0);
        break;
    case UI_STATE_ALERT:
        bg_top = lv_color_hex(0xFFF7EB);
        bg_bottom = lv_color_hex(0xFFEFD9);
        hint_bg = lv_color_hex(0xFFEED2);
        hint_border = lv_color_hex(0xEAC695);
        selected_bg = lv_color_hex(0xFFE4BF);
        selected_fg = lv_color_hex(0x5B3000);
        selected_border = lv_color_hex(0xE3A76C);
        break;
    case UI_STATE_NO_DATA:
        bg_top = lv_color_hex(0xEEF2F7);
        bg_bottom = lv_color_hex(0xE4EBF3);
        hint_bg = lv_color_hex(0xEDF2F8);
        hint_border = lv_color_hex(0xC7D4E5);
        selected_bg = lv_color_hex(0xE4ECF6);
        selected_fg = lv_color_hex(0x2D3F52);
        selected_border = lv_color_hex(0xA3B4C8);
        break;
    case UI_STATE_NORMAL:
    default:
        break;
    }

    s_item_selected_bg = selected_bg;
    s_item_selected_fg = selected_fg;
    s_item_selected_border = selected_border;

    lv_obj_set_style_bg_color(scr, bg_top, LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(scr, bg_bottom, LV_PART_MAIN);

    lv_obj_set_style_bg_color(panel_hint, hint_bg, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel_hint, hint_border, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_hint, hint_fg, LV_PART_MAIN);
}

static int choose_entry_main_index(const sensor_data_t *latest_data)
{
    if (wifi_manager_is_provisioning())
    {
        return 3; // Network Settings
    }

    if (latest_data == NULL)
    {
        return 4; // Device Status
    }

    if (!wifi_manager_is_connected())
    {
        return 3; // Network Settings
    }

    cloud_http_status_t cloud = {0};
    (void)mqtt_client_get_status(&cloud);
    if (cloud_has_fault(&cloud))
    {
        return 3; // Network Settings
    }

    const device_config_t *cfg = config_manager_get_config();
    if (is_alert_state(latest_data, cfg))
    {
        return 4; // Device Status
    }

    return 0; // Manual Control
}

static void set_action_hint(const char *text)
{
    if (text == NULL)
    {
        s_action_hint[0] = '\0';
        s_action_hint_until = 0;
        return;
    }
    strncpy(s_action_hint, text, sizeof(s_action_hint) - 1);
    s_action_hint[sizeof(s_action_hint) - 1] = '\0';
    s_action_hint_until = xTaskGetTickCount() + pdMS_TO_TICKS(ACTION_HINT_MS);
}

static void apply_hint_text(const char *default_hint)
{
    if (lbl_hint == NULL)
    {
        return;
    }

    TickType_t now = xTaskGetTickCount();
    if (s_action_hint[0] != '\0' && now < s_action_hint_until)
    {
        lv_label_set_text(lbl_hint, s_action_hint);
        return;
    }

    lv_label_set_text(lbl_hint, default_hint != NULL ? default_hint : "");
}

static void update_header(const sensor_data_t *d)
{
    (void)d;
    const device_config_t *cfg = config_manager_get_config();
    const char *device_name = (cfg != NULL && cfg->device_name[0] != '\0') ? cfg->device_name : "ESP32";
    const char *species = (cfg != NULL && cfg->species[0] != '\0') ? cfg->species : "Unknown";
    lv_label_set_text_fmt(lbl_title, "%s | %s", device_name, species);

    char ts[20] = {0};
    if (wifi_manager_get_local_time_string(ts, sizeof(ts)) == ESP_OK && strlen(ts) >= 16)
    {
        char hhmm[6] = {0};
        memcpy(hhmm, &ts[11], 5);
        hhmm[5] = '\0';
        lv_label_set_text(lbl_time, hhmm);
    }
    else
    {
        lv_label_set_text(lbl_time, "--:--");
    }
}

static void clear_item_lines(void)
{
    for (int i = 0; i < MENU_MAX_ITEMS; ++i)
    {
        if (lbl_items[i] != NULL)
        {
            lv_label_set_text(lbl_items[i], "");
            lv_obj_set_style_bg_opa(lbl_items[i], LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_border_width(lbl_items[i], 0, LV_PART_MAIN);
            lv_obj_set_style_text_color(lbl_items[i], lv_color_hex(0x102437), LV_PART_MAIN);
        }
    }
}

static void set_item_lines(const char *const *items, int count, int selected, bool use_selector)
{
    clear_item_lines();
    s_page_item_count = count;
    int list_top = MENU_MAIN_LIST_TOP;
    int step = MENU_MAIN_LIST_STEP;

    if (s_page != MENU_PAGE_MAIN && panel_content != NULL && lbl_info != NULL)
    {
        lv_obj_update_layout(panel_content);
        const int info_bottom = lv_obj_get_y(lbl_info) + lv_obj_get_height(lbl_info);
        const int content_h = lv_obj_get_height(panel_content);
        const int pad_bottom = (int)lv_obj_get_style_pad_bottom(panel_content, LV_PART_MAIN);
        const int usable_bottom = content_h - pad_bottom - 2;

        list_top = info_bottom + MENU_SUB_LIST_GAP;
        if (list_top < MENU_MAIN_LIST_TOP)
        {
            list_top = MENU_MAIN_LIST_TOP;
        }

        if (count > 1)
        {
            int available_span = usable_bottom - list_top;
            if (available_span < 0)
            {
                available_span = 0;
            }

            int fit_step = available_span / (count - 1);
            if (fit_step < step)
            {
                step = fit_step;
            }
            if (step < MENU_SUB_MIN_STEP)
            {
                step = MENU_SUB_MIN_STEP;
            }
        }
        else
        {
            step = 0;
        }
    }

    for (int i = 0; i < count && i < MENU_MAX_ITEMS; ++i)
    {
        char line[96];
        bool is_selected = use_selector && (i == selected);
        if (use_selector)
        {
            snprintf(line, sizeof(line), "%s %s", (i == selected) ? ">" : " ", items[i]);
        }
        else
        {
            snprintf(line, sizeof(line), "%s", items[i]);
        }
        lv_label_set_text(lbl_items[i], line);

        lv_obj_set_style_bg_opa(lbl_items[i], is_selected ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_bg_color(lbl_items[i], s_item_selected_bg, LV_PART_MAIN);
        lv_obj_set_style_border_width(lbl_items[i], is_selected ? 1 : 0, LV_PART_MAIN);
        lv_obj_set_style_border_color(lbl_items[i], s_item_selected_border, LV_PART_MAIN);
        lv_obj_set_style_radius(lbl_items[i], 8, LV_PART_MAIN);
        lv_obj_set_style_text_color(lbl_items[i], is_selected ? s_item_selected_fg : lv_color_hex(0x102437), LV_PART_MAIN);
        lv_obj_align(lbl_items[i], LV_ALIGN_TOP_LEFT, 2, list_top + i * step);
    }
}

static void load_threshold_working_values(void)
{
    const device_config_t *cfg = config_manager_get_config();
    if (cfg != NULL)
    {
        s_threshold_lower = (int)(cfg->threshold_lower + 0.5);
        s_threshold_upper = (int)(cfg->threshold_upper + 0.5);
    }
    if (s_threshold_lower < 0)
    {
        s_threshold_lower = 0;
    }
    if (s_threshold_upper > 100)
    {
        s_threshold_upper = 100;
    }
    if (s_water_level_alarm < 0)
    {
        s_water_level_alarm = 0;
    }
    if (s_water_level_alarm > 100)
    {
        s_water_level_alarm = 100;
    }
    if (s_watering_duration_sec < 1)
    {
        s_watering_duration_sec = 1;
    }
    if (s_watering_duration_sec > 600)
    {
        s_watering_duration_sec = 600;
    }
}

static void apply_threshold_save(void)
{
    if (s_threshold_lower >= s_threshold_upper)
    {
        int fixed_upper = s_threshold_lower + 1;
        if (fixed_upper > 100)
        {
            fixed_upper = 100;
            s_threshold_lower = fixed_upper - 1;
        }
        s_threshold_upper = fixed_upper;
    }

    config_manager_update_threshold((double)s_threshold_upper, (double)s_threshold_lower);
    esp_err_t ret = wifi_manager_request_threshold_sync("menu_save");
    if (ret == ESP_OK)
    {
        set_action_hint("Thresholds saved & synced");
    }
    else
    {
        set_action_hint("Thresholds saved (sync issue)");
        ESP_LOGW(TAG, "threshold sync request failed after menu save: %s", esp_err_to_name(ret));
    }
}

static void open_confirm(confirm_action_t action, const char *text, menu_page_t return_page, int return_index)
{
    s_confirm_action = action;
    s_confirm_return_page = return_page;
    s_confirm_return_index = return_index;
    strncpy(s_confirm_text, text, sizeof(s_confirm_text) - 1);
    s_confirm_text[sizeof(s_confirm_text) - 1] = '\0';
    s_page = MENU_PAGE_CONFIRM;
    report_menu_state_to_stm32("open_confirm", false);
}

static void enter_main_menu(void)
{
    s_page = MENU_PAGE_MAIN;
    s_sub_index = 0;
    s_threshold_editing = false;
    report_menu_state_to_stm32("enter_main_menu", false);
}

static void enter_submenu(menu_page_t page)
{
    s_page = page;
    s_sub_index = 0;
    s_threshold_editing = false;
    if (page == MENU_PAGE_THRESHOLD)
    {
        load_threshold_working_values();
    }
    report_menu_state_to_stm32("enter_submenu", false);
}

static void do_manual_action_start(void)
{
    open_confirm(CONFIRM_START_WATER, "Start watering?", MENU_PAGE_MANUAL, s_sub_index);
}

static void do_manual_action_stop(void)
{
    esp_err_t ret = stm32_protocol_send_control_command("water_control", "stop", 0, false, true, NULL);
    if (ret == ESP_OK)
    {
        set_action_hint("Command sent: stop watering");
    }
    else
    {
        char text[64];
        snprintf(text, sizeof(text), "Stop failed: %s", esp_err_to_name(ret));
        set_action_hint(text);
    }
}

static void do_mode_action(int idx)
{
    const char *mode = "auto";
    const char *name = "Auto Mode";
    switch (idx)
    {
    case 0:
        mode = "auto";
        name = "Auto Mode";
        break;
    case 1:
        mode = "manual";
        name = "Manual Mode";
        break;
    case 2:
        mode = "eco";
        name = "Eco Mode";
        break;
    case 3:
        mode = "debug";
        name = "Debug Mode";
        break;
    case 4:
        mode = "protect";
        name = "Protect Mode";
        break;
    default:
        return;
    }

    esp_err_t ret = stm32_protocol_send_mode_command(mode, "menu_select", true, true, NULL);
    if (ret == ESP_OK)
    {
        char text[64];
        snprintf(text, sizeof(text), "Mode switched: %s", name);
        set_action_hint(text);
    }
    else
    {
        char text[64];
        snprintf(text, sizeof(text), "Mode switch failed: %s", esp_err_to_name(ret));
        set_action_hint(text);
    }
}

static void do_network_action_start_prov(void)
{
    esp_err_t ret = wifi_manager_start_provisioning();
    if (ret == ESP_OK)
    {
        set_action_hint("Provisioning AP started");
        report_menu_state_to_stm32("exit_for_provisioning", false);
        ui_set_mode(&ui_default);
    }
    else
    {
        char text[64];
        snprintf(text, sizeof(text), "Start provisioning failed: %s", esp_err_to_name(ret));
        set_action_hint(text);
    }
}

static void do_network_action_reconnect(void)
{
    wifi_credentials_t cred = {0};
    esp_err_t ret = wifi_manager_get_credentials(&cred);
    if (ret == ESP_OK && cred.ssid[0] != '\0')
    {
        ret = wifi_manager_set_credentials(cred.ssid, cred.password, true);
    }
    if (ret == ESP_OK)
    {
        set_action_hint("Wi-Fi reconnect requested");
    }
    else
    {
        char text[64];
        snprintf(text, sizeof(text), "Reconnect failed: %s", esp_err_to_name(ret));
        set_action_hint(text);
    }
}

static void perform_confirm_action(void)
{
    switch (s_confirm_action)
    {
    case CONFIRM_START_WATER:
    {
        esp_err_t ret = stm32_protocol_send_control_command("water_control", "start", s_watering_duration_sec, false, true, NULL);
        if (ret == ESP_OK)
        {
            set_action_hint("Command sent: start watering");
        }
        else
        {
            char text[64];
            snprintf(text, sizeof(text), "Start failed: %s", esp_err_to_name(ret));
            set_action_hint(text);
        }
        break;
    }
    case CONFIRM_CLEAR_WIFI:
    {
        esp_err_t ret = wifi_manager_clear_credentials();
        if (ret == ESP_OK)
        {
            set_action_hint("Wi-Fi credentials cleared");
            (void)wifi_manager_start_provisioning();
            report_menu_state_to_stm32("exit_for_clear_wifi", false);
            ui_set_mode(&ui_default);
        }
        else
        {
            char text[64];
            snprintf(text, sizeof(text), "Clear failed: %s", esp_err_to_name(ret));
            set_action_hint(text);
        }
        break;
    }
    case CONFIRM_RESTORE_DEFAULT:
        config_manager_update_threshold(70.0, 30.0);
        set_action_hint("Default thresholds restored");
        break;
    case CONFIRM_REBOOT:
        report_menu_state_to_stm32("reboot", false);
        set_action_hint("Rebooting...");
        esp_restart();
        break;
    case CONFIRM_NONE:
    default:
        break;
    }

    s_confirm_action = CONFIRM_NONE;
}

static void track_stm_data_tick(const sensor_data_t *d)
{
    if (d == NULL || d->trace_id[0] == '\0')
    {
        return;
    }
    if (strcmp(d->trace_id, s_last_stm_trace) != 0)
    {
        strncpy(s_last_stm_trace, d->trace_id, sizeof(s_last_stm_trace) - 1);
        s_last_stm_trace[sizeof(s_last_stm_trace) - 1] = '\0';
        s_last_stm_data_tick = xTaskGetTickCount();
    }
}

static void render_main(const sensor_data_t *d)
{
    (void)d;
    if (s_main_index < 0 || s_main_index >= 6)
    {
        s_main_index = 0;
    }
    lv_label_set_text(lbl_section, "Main Menu");
    lv_label_set_text(lbl_info, "Select a menu item");
    set_item_lines(k_main_items, 6, s_main_index, true);
    apply_hint_text("Left/Right: Select  OK: Confirm  Hold: Back");
}

static void render_manual(const sensor_data_t *d)
{
    if (s_sub_index < 0 || s_sub_index >= 4)
    {
        s_sub_index = 0;
    }
    char pump_state[16] = "Unknown";
    if (d != NULL)
    {
        if (contains_token_ci(d->water, "on") || contains_token_ci(d->water, "watering"))
        {
            strcpy(pump_state, "ON");
        }
        else
        {
            strcpy(pump_state, "OFF");
        }
    }

    char item_status[48];
    snprintf(item_status, sizeof(item_status), "Pump: %s", pump_state);
    const char *items[] = {"Start Watering", "Stop Watering", item_status, "Back"};

    lv_label_set_text(lbl_section, "Manual Control");
    lv_label_set_text(lbl_info, "Confirm high-risk actions before execution");
    set_item_lines(items, 4, s_sub_index, true);
    apply_hint_text("Left/Right: Select  OK: Confirm  Hold: Back");
}

static void render_mode(const sensor_data_t *d)
{
    (void)d;
    if (s_sub_index < 0 || s_sub_index >= 6)
    {
        s_sub_index = 0;
    }
    const char *items[] = {"Auto Mode", "Manual Mode", "Eco Mode", "Debug Mode", "Protect Mode", "Back"};
    lv_label_set_text(lbl_section, "Mode Settings");
    lv_label_set_text(lbl_info, "Select operation mode");
    set_item_lines(items, 6, s_sub_index, true);
    apply_hint_text("Left/Right: Select  OK: Confirm  Hold: Back");
}

static void render_threshold(const sensor_data_t *d)
{
    (void)d;
    if (s_sub_index < 0 || s_sub_index >= 6)
    {
        s_sub_index = 0;
    }
    char item0[48];
    char item1[48];
    char item2[48];
    char item3[48];
    snprintf(item0, sizeof(item0), "Soil Lower: %d%%", s_threshold_lower);
    snprintf(item1, sizeof(item1), "Soil Upper: %d%%", s_threshold_upper);
    snprintf(item2, sizeof(item2), "Water Alarm: %d%%", s_water_level_alarm);
    snprintf(item3, sizeof(item3), "Water Duration: %ds", s_watering_duration_sec);
    const char *items[] = {item0, item1, item2, item3, "Save Settings", "Back"};

    lv_label_set_text(lbl_section, "Threshold Settings");
    if (s_threshold_editing)
    {
        lv_label_set_text(lbl_info, "Editing: Left/Right adjusts current item");
        apply_hint_text("Left/Right: Adjust  OK: Confirm  Hold: Cancel");
    }
    else
    {
        lv_label_set_text(lbl_info, "Select an item then press OK to edit");
        apply_hint_text("Left/Right: Select  OK: Edit/Save  Hold: Back");
    }
    set_item_lines(items, 6, s_sub_index, true);
}

static void render_network(const sensor_data_t *d)
{
    (void)d;
    if (s_sub_index < 0 || s_sub_index >= 4)
    {
        s_sub_index = 0;
    }
    cloud_http_status_t cloud = {0};
    (void)mqtt_client_get_status(&cloud);

    char ip[20] = "--";
    bool wifi_ok = wifi_manager_is_connected();
    if (wifi_ok)
    {
        (void)wifi_manager_get_sta_ip_string(ip, sizeof(ip));
    }

    wifi_ap_record_t ap = {0};
    bool rssi_ok = false;
    int rssi = 0;
    if (wifi_ok && esp_wifi_sta_get_ap_info(&ap) == ESP_OK)
    {
        rssi_ok = true;
        rssi = ap.rssi;
    }

    const char *cloud_text = cloud_has_fault(&cloud) ? "Fault" : "OK";
    char info[220];
    const char *signal_text = "--";
    snprintf(info, sizeof(info),
             "Wi-Fi: %s\nIP: %s\nCloud: %s\nSignal: %s",
             wifi_ok ? "Connected" : "Disconnected",
             wifi_ok ? ip : "--",
             cloud_text,
             signal_text);
    if (rssi_ok)
    {
        char rssi_text[24];
        snprintf(rssi_text, sizeof(rssi_text), "%d dBm", rssi);
        snprintf(info, sizeof(info),
                 "Wi-Fi: %s\nIP: %s\nCloud: %s\nSignal: %s",
                 wifi_ok ? "Connected" : "Disconnected",
                 wifi_ok ? ip : "--",
                 cloud_text,
                 rssi_text);
    }

    const char *items[] = {"Start Provisioning AP", "Reconnect Wi-Fi", "Clear Wi-Fi Credentials", "Back"};
    lv_label_set_text(lbl_section, "Network Settings");
    lv_label_set_text(lbl_info, info);
    set_item_lines(items, 4, s_sub_index, true);
    apply_hint_text("Left/Right: Select  OK: Confirm  Hold: Back");
}

static void render_device_status(const sensor_data_t *d)
{
    if (s_sub_index < 0 || s_sub_index >= 1)
    {
        s_sub_index = 0;
    }
    TickType_t now = xTaskGetTickCount();
    bool stm_online = (d != NULL) && (s_last_stm_data_tick > 0) &&
                      ((now - s_last_stm_data_tick) < pdMS_TO_TICKS(5000));
    bool sensor_ok = d != NULL && isfinite(d->temp) && isfinite(d->humidity) && isfinite(d->soil);
    bool water_low = d != NULL && water_low_from_text(d->water);
    const char *pump_state = "Unknown";
    if (d != NULL)
    {
        if (contains_token_ci(d->water, "on") || contains_token_ci(d->water, "watering"))
        {
            pump_state = "ON";
        }
        else if (contains_token_ci(d->water, "off") || contains_token_ci(d->water, "stop"))
        {
            pump_state = "OFF";
        }
    }
    bool alert = false;
    const device_config_t *cfg = config_manager_get_config();
    if (d != NULL)
    {
        alert = is_alert_state(d, cfg);
    }

    char info[260];
    snprintf(info, sizeof(info),
             "STM32: %s\nUART: %s\nSensor: %s\nPump: %s\nWater: %s\nBattery: %u%%\nAlarm: %s",
             stm_online ? "Online" : "Offline",
             stm_online ? "OK" : "Fault",
             sensor_ok ? "OK" : "Fault",
             pump_state,
             water_low ? "LOW" : "OK",
             d != NULL ? (unsigned int)d->battery : 0U,
             alert ? "YES" : "NO");

    const char *items[] = {"Back"};
    lv_label_set_text(lbl_section, "Device Status");
    lv_label_set_text(lbl_info, info);
    set_item_lines(items, 1, s_sub_index, true);
    apply_hint_text("OK: Back  Hold: Upper Back");
}

static void render_system_info(const sensor_data_t *d)
{
    if (s_sub_index < 0 || s_sub_index >= 3)
    {
        s_sub_index = 0;
    }
    cloud_http_status_t cloud = {0};
    (void)mqtt_client_get_status(&cloud);
    const device_config_t *cfg = config_manager_get_config();

    uint64_t sec = (uint64_t)(esp_timer_get_time() / 1000000LL);
    uint64_t h = sec / 3600ULL;
    uint64_t m = (sec % 3600ULL) / 60ULL;
    uint64_t s = sec % 60ULL;

    const char *device_id = (cloud.device_id[0] != '\0') ? cloud.device_id : (d != NULL ? d->device_id : "unknown");
    const char *plant_name = (cfg != NULL && cfg->species[0] != '\0') ? cfg->species : "Unknown";

    char info[280];
    snprintf(info, sizeof(info),
             "Device ID: %s\nPlant Name: %s\nFW Version: %s\nUptime: %02llu:%02llu:%02llu\nFree Heap: %lu B\nLast HTTP: %d",
             device_id,
             plant_name,
             esp_get_idf_version(),
             (unsigned long long)h,
             (unsigned long long)m,
             (unsigned long long)s,
             (unsigned long)esp_get_free_heap_size(),
             cloud.last_http_status);

    const char *items[] = {"Restore Defaults", "Reboot Device", "Back"};
    lv_label_set_text(lbl_section, "System Info");
    lv_label_set_text(lbl_info, info);
    set_item_lines(items, 3, s_sub_index, true);
    apply_hint_text("Left/Right: Select  OK: Confirm  Hold: Back");
}

static void render_confirm(const sensor_data_t *d)
{
    (void)d;
    lv_label_set_text(lbl_section, "Confirm");
    lv_label_set_text(lbl_info, s_confirm_text);
    const char *items[] = {"Press OK to execute"};
    set_item_lines(items, 1, 0, false);
    apply_hint_text("OK: Confirm  Hold: Cancel");
}

static void render_menu(const sensor_data_t *d)
{
    update_header(d);
    const device_config_t *cfg = config_manager_get_config();
    cloud_http_status_t cloud = {0};
    (void)mqtt_client_get_status(&cloud);
    apply_menu_theme(decide_menu_state(d, cfg, &cloud));

    switch (s_page)
    {
    case MENU_PAGE_MAIN:
        render_main(d);
        break;
    case MENU_PAGE_MANUAL:
        render_manual(d);
        break;
    case MENU_PAGE_MODE:
        render_mode(d);
        break;
    case MENU_PAGE_THRESHOLD:
        render_threshold(d);
        break;
    case MENU_PAGE_NETWORK:
        render_network(d);
        break;
    case MENU_PAGE_DEVICE_STATUS:
        render_device_status(d);
        break;
    case MENU_PAGE_SYSTEM_INFO:
        render_system_info(d);
        break;
    case MENU_PAGE_CONFIRM:
        render_confirm(d);
        break;
    default:
        break;
    }
}

static void build_layout(void)
{
    scr = lv_scr_act();
    lv_obj_clean(scr);

    lv_obj_set_style_bg_color(scr, lv_color_hex(0xE8F5FF), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0xD9F0EA), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr, 0, LV_PART_MAIN);

    const int margin = 8;
    const int header_h = 60;
    const int hint_h = 48;

    panel_header = lv_obj_create(scr);
    lv_obj_set_size(panel_header, LV_HOR_RES - margin * 2, header_h);
    lv_obj_align(panel_header, LV_ALIGN_TOP_MID, 0, margin);
    lv_obj_set_style_radius(panel_header, 14, LV_PART_MAIN);
    lv_obj_set_style_bg_color(panel_header, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel_header, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel_header, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel_header, lv_color_hex(0xD3DEEA), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel_header, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(panel_header, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(panel_header, lv_color_hex(0x6A7A8A), LV_PART_MAIN);
    lv_obj_set_style_pad_hor(panel_header, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(panel_header, 6, LV_PART_MAIN);

    lbl_title = lv_label_create(panel_header);
    lv_obj_set_style_text_font(lbl_title, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x1B2A3A), LV_PART_MAIN);
    lv_obj_set_width(lbl_title, LV_HOR_RES - (margin * 2) - 110);
    lv_label_set_long_mode(lbl_title, LV_LABEL_LONG_DOT);
    lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 0, 0);

    lbl_time = lv_label_create(panel_header);
    lv_obj_set_style_text_font(lbl_time, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_time, lv_color_hex(0x1B2A3A), LV_PART_MAIN);
    lv_obj_align(lbl_time, LV_ALIGN_TOP_RIGHT, 0, 0);

    panel_content = lv_obj_create(scr);
    lv_obj_set_size(panel_content, LV_HOR_RES - margin * 2, LV_VER_RES - (margin * 3) - header_h - hint_h);
    lv_obj_align(panel_content, LV_ALIGN_TOP_MID, 0, margin * 2 + header_h);
    lv_obj_set_style_radius(panel_content, 14, LV_PART_MAIN);
    lv_obj_set_style_bg_color(panel_content, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel_content, LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel_content, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel_content, lv_color_hex(0xD3DEEA), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel_content, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(panel_content, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(panel_content, lv_color_hex(0x617387), LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel_content, 10, LV_PART_MAIN);

    lbl_section = lv_label_create(panel_content);
    lv_obj_set_style_text_font(lbl_section, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_section, lv_color_hex(0x1C2F44), LV_PART_MAIN);
    lv_obj_align(lbl_section, LV_ALIGN_TOP_LEFT, 2, 0);

    lbl_info = lv_label_create(panel_content);
    lv_obj_set_width(lbl_info, LV_HOR_RES - margin * 2 - 24);
    lv_obj_set_style_text_font(lbl_info, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_info, lv_color_hex(0x445A72), LV_PART_MAIN);
    lv_label_set_long_mode(lbl_info, LV_LABEL_LONG_WRAP);
    lv_obj_align(lbl_info, LV_ALIGN_TOP_LEFT, 2, 22);

    const int list_top = MENU_MAIN_LIST_TOP;
    const int step = MENU_MAIN_LIST_STEP;

    for (int i = 0; i < MENU_MAX_ITEMS; ++i)
    {
        lbl_items[i] = lv_label_create(panel_content);
        lv_obj_set_width(lbl_items[i], LV_HOR_RES - margin * 2 - 24);
        lv_obj_set_style_text_font(lbl_items[i], LV_FONT_DEFAULT, LV_PART_MAIN);
        lv_obj_set_style_text_color(lbl_items[i], lv_color_hex(0x102437), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(lbl_items[i], LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(lbl_items[i], 0, LV_PART_MAIN);
        lv_obj_set_style_radius(lbl_items[i], 8, LV_PART_MAIN);
        lv_obj_set_style_pad_left(lbl_items[i], 10, LV_PART_MAIN);
        lv_obj_set_style_pad_right(lbl_items[i], 8, LV_PART_MAIN);
        lv_obj_set_style_pad_top(lbl_items[i], 3, LV_PART_MAIN);
        lv_obj_set_style_pad_bottom(lbl_items[i], 3, LV_PART_MAIN);
        lv_label_set_long_mode(lbl_items[i], LV_LABEL_LONG_CLIP);
        lv_obj_align(lbl_items[i], LV_ALIGN_TOP_LEFT, 2, list_top + i * step);
    }

    panel_hint = lv_obj_create(scr);
    lv_obj_set_size(panel_hint, LV_HOR_RES - margin * 2, hint_h);
    lv_obj_align(panel_hint, LV_ALIGN_BOTTOM_MID, 0, -margin);
    lv_obj_set_style_radius(panel_hint, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_color(panel_hint, lv_color_hex(0xF6FBFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel_hint, LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel_hint, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel_hint, lv_color_hex(0xD3DEEA), LV_PART_MAIN);
    lv_obj_set_style_pad_left(panel_hint, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_right(panel_hint, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_top(panel_hint, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(panel_hint, 8, LV_PART_MAIN);

    lbl_hint = lv_label_create(panel_hint);
    lv_obj_set_width(lbl_hint, LV_HOR_RES - margin * 2 - 24);
    lv_obj_set_style_text_font(lbl_hint, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_hint, lv_color_hex(0x213142), LV_PART_MAIN);
    lv_label_set_long_mode(lbl_hint, LV_LABEL_LONG_WRAP);
    lv_obj_align(lbl_hint, LV_ALIGN_LEFT_MID, 0, 0);

    s_item_selected_bg = lv_color_hex(0xE9F4FF);
    s_item_selected_fg = lv_color_hex(0x12304A);
    s_item_selected_border = lv_color_hex(0x8FB8E4);
}

static bool event_is_left(button_event_t event)
{
    return event == BUTTON_LEFT_SHORT_PRESS;
}

static bool event_is_right(button_event_t event)
{
    return event == BUTTON_RIGHT_SHORT_PRESS || event == BUTTON_DOUBLE_CLICK;
}

static bool event_is_confirm(button_event_t event)
{
    return event == BUTTON_MIDDLE_SHORT_PRESS || event == BUTTON_SHORT_PRESS;
}

static bool event_is_back_long(button_event_t event)
{
    return event == BUTTON_MIDDLE_LONG_PRESS || event == BUTTON_LONG_PRESS;
}

static void selection_prev(int *idx)
{
    if (idx == NULL || s_page_item_count <= 0)
    {
        return;
    }
    if (*idx <= 0)
    {
        *idx = s_page_item_count - 1;
    }
    else
    {
        (*idx)--;
    }
}

static void selection_next(int *idx)
{
    if (idx == NULL || s_page_item_count <= 0)
    {
        return;
    }
    (*idx)++;
    if (*idx >= s_page_item_count)
    {
        *idx = 0;
    }
}

static void handle_main_confirm(void)
{
    switch (s_main_index)
    {
    case 0:
        enter_submenu(MENU_PAGE_MANUAL);
        break;
    case 1:
        enter_submenu(MENU_PAGE_MODE);
        break;
    case 2:
        enter_submenu(MENU_PAGE_THRESHOLD);
        break;
    case 3:
        enter_submenu(MENU_PAGE_NETWORK);
        break;
    case 4:
        enter_submenu(MENU_PAGE_DEVICE_STATUS);
        break;
    case 5:
        enter_submenu(MENU_PAGE_SYSTEM_INFO);
        break;
    default:
        break;
    }
}

static void adjust_threshold_value(int delta)
{
    switch (s_sub_index)
    {
    case 0:
        s_threshold_lower += delta;
        if (s_threshold_lower < 0)
        {
            s_threshold_lower = 0;
        }
        if (s_threshold_lower > 99)
        {
            s_threshold_lower = 99;
        }
        break;
    case 1:
        s_threshold_upper += delta;
        if (s_threshold_upper < 1)
        {
            s_threshold_upper = 1;
        }
        if (s_threshold_upper > 100)
        {
            s_threshold_upper = 100;
        }
        break;
    case 2:
        s_water_level_alarm += delta;
        if (s_water_level_alarm < 0)
        {
            s_water_level_alarm = 0;
        }
        if (s_water_level_alarm > 100)
        {
            s_water_level_alarm = 100;
        }
        break;
    case 3:
        s_watering_duration_sec += delta;
        if (s_watering_duration_sec < 1)
        {
            s_watering_duration_sec = 1;
        }
        if (s_watering_duration_sec > 600)
        {
            s_watering_duration_sec = 600;
        }
        break;
    default:
        break;
    }
}

static void handle_sub_confirm(void)
{
    switch (s_page)
    {
    case MENU_PAGE_MANUAL:
        if (s_sub_index == 0)
        {
            do_manual_action_start();
        }
        else if (s_sub_index == 1)
        {
            do_manual_action_stop();
        }
        else if (s_sub_index == 3)
        {
            enter_main_menu();
        }
        break;
    case MENU_PAGE_MODE:
        if (s_sub_index >= 0 && s_sub_index <= 4)
        {
            do_mode_action(s_sub_index);
        }
        else if (s_sub_index == 5)
        {
            enter_main_menu();
        }
        break;
    case MENU_PAGE_THRESHOLD:
        if (s_sub_index >= 0 && s_sub_index <= 3)
        {
            s_threshold_editing = !s_threshold_editing;
        }
        else if (s_sub_index == 4)
        {
            apply_threshold_save();
        }
        else if (s_sub_index == 5)
        {
            enter_main_menu();
        }
        break;
    case MENU_PAGE_NETWORK:
        if (s_sub_index == 0)
        {
            do_network_action_start_prov();
        }
        else if (s_sub_index == 1)
        {
            do_network_action_reconnect();
        }
        else if (s_sub_index == 2)
        {
            open_confirm(CONFIRM_CLEAR_WIFI, "Clear Wi-Fi credentials?", MENU_PAGE_NETWORK, s_sub_index);
        }
        else if (s_sub_index == 3)
        {
            enter_main_menu();
        }
        break;
    case MENU_PAGE_DEVICE_STATUS:
        enter_main_menu();
        break;
    case MENU_PAGE_SYSTEM_INFO:
        if (s_sub_index == 0)
        {
            open_confirm(CONFIRM_RESTORE_DEFAULT, "Restore default settings?", MENU_PAGE_SYSTEM_INFO, s_sub_index);
        }
        else if (s_sub_index == 1)
        {
            open_confirm(CONFIRM_REBOOT, "Reboot device?", MENU_PAGE_SYSTEM_INFO, s_sub_index);
        }
        else if (s_sub_index == 2)
        {
            enter_main_menu();
        }
        break;
    default:
        break;
    }
}

void ui_menu_on_button_event(button_event_t event)
{
    s_last_input_tick = xTaskGetTickCount();

    if (s_page == MENU_PAGE_CONFIRM)
    {
        if (event_is_confirm(event))
        {
            perform_confirm_action();
            s_page = s_confirm_return_page;
            s_sub_index = s_confirm_return_index;
            report_menu_state_to_stm32("confirm_done", false);
        }
        else if (event_is_back_long(event))
        {
            set_action_hint("Cancelled");
            s_page = s_confirm_return_page;
            s_sub_index = s_confirm_return_index;
            s_confirm_action = CONFIRM_NONE;
            report_menu_state_to_stm32("confirm_cancelled", false);
        }
        return;
    }

    if (event_is_back_long(event))
    {
        if (s_threshold_editing)
        {
            s_threshold_editing = false;
            set_action_hint("Edit cancelled");
            return;
        }

        if (s_page == MENU_PAGE_MAIN)
        {
            report_menu_state_to_stm32("exit_menu", false);
            ui_set_mode(&ui_default);
        }
        else
        {
            enter_main_menu();
        }
        return;
    }

    if (s_page == MENU_PAGE_THRESHOLD && s_threshold_editing && (event_is_left(event) || event_is_right(event)))
    {
        adjust_threshold_value(event_is_right(event) ? 1 : -1);
        return;
    }

    if (event_is_left(event))
    {
        if (s_page == MENU_PAGE_MAIN)
        {
            selection_prev(&s_main_index);
        }
        else if (s_page != MENU_PAGE_CONFIRM)
        {
            selection_prev(&s_sub_index);
        }
        report_menu_state_to_stm32("navigate_prev", false);
        return;
    }

    if (event_is_right(event))
    {
        if (s_page == MENU_PAGE_MAIN)
        {
            selection_next(&s_main_index);
        }
        else if (s_page != MENU_PAGE_CONFIRM)
        {
            selection_next(&s_sub_index);
        }
        report_menu_state_to_stm32("navigate_next", false);
        return;
    }

    if (event_is_confirm(event))
    {
        if (s_page == MENU_PAGE_MAIN)
        {
            handle_main_confirm();
        }
        else
        {
            handle_sub_confirm();
        }
        report_menu_state_to_stm32("confirm", false);
    }
}

void ui_menu_prepare_entry(const sensor_data_t *latest_data)
{
    s_entry_main_index = choose_entry_main_index(latest_data);
}

static void init_menu(void)
{
    s_page = MENU_PAGE_MAIN;
    s_main_index = s_entry_main_index;
    s_sub_index = 0;
    s_page_item_count = 6;
    s_threshold_editing = false;
    s_confirm_action = CONFIRM_NONE;
    s_last_input_tick = xTaskGetTickCount();
    s_last_stm_data_tick = 0;
    s_last_stm_trace[0] = '\0';
    set_action_hint(NULL);

    build_layout();
    const sensor_data_t *latest = uart_receiver_get_latest_data();
    track_stm_data_tick(latest);
    render_menu(latest);
    report_menu_state_to_stm32("enter_menu", false);
}

static void update_menu(const sensor_data_t *data)
{
    track_stm_data_tick(data);

    TickType_t now = xTaskGetTickCount();
    if ((now - s_last_input_tick) >= pdMS_TO_TICKS(MENU_TIMEOUT_MS))
    {
        set_action_hint("Menu timeout, returning to default UI");
        report_menu_state_to_stm32("timeout_exit", false);
        ui_set_mode(&ui_default);
        return;
    }

    render_menu(data);
}

const ui_module_t ui_menu = {
    .init = init_menu,
    .update = update_menu,
};
