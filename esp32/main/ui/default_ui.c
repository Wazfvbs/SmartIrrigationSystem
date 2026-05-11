// ui/default_ui.c
#include "default_ui.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "lvgl.h"

#include "../app_mqtt/app_mqtt.h"
#include "../config_manager/config_manager.h"
#include "../uart_receiver/uart_receiver.h"
#include "../wifi_manager/wifi_manager.h"

extern const lv_font_t cjk_22;

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
static lv_obj_t *lbl_state = NULL;
static lv_obj_t *lbl_time = NULL;

static lv_obj_t *cards[4] = {0};
static lv_obj_t *card_accent[4] = {0};
static lv_obj_t *lbl_card_title[4] = {0};
static lv_obj_t *lbl_card_value[4] = {0};

static lv_obj_t *panel_hint = NULL;
static lv_obj_t *lbl_hint = NULL;

static lv_obj_t *bar = NULL;
static lv_obj_t *lbl_bar_face = NULL;
static lv_obj_t *lbl_bar_net = NULL;
static lv_obj_t *lbl_bar_batt = NULL;

static const char *k_card_titles[4] = {"Temperature", "Humidity", "Soil", "Light"};
static const uint32_t k_card_accent_color[4] = {
    0x1D8FE1, // temperature
    0x00A58B, // humidity
    0x58A53C, // soil
    0xD79A1E  // light
};

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
    return contains_token_ci(water_text, "low") ||
           contains_token_ci(water_text, "empty") ||
           contains_token_ci(water_text, "lack");
}

static ui_state_t decide_ui_state(const sensor_data_t *d,
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

static void apply_state_theme(ui_state_t state)
{
    lv_color_t bg_top = lv_color_hex(0xE8F5FF);
    lv_color_t bg_bottom = lv_color_hex(0xD9F0EA);
    lv_color_t chip_bg = lv_color_hex(0x1A8F6D);
    lv_color_t chip_fg = lv_color_hex(0xFFFFFF);
    lv_color_t hint_bg = lv_color_hex(0xF6FBFF);
    lv_color_t hint_fg = lv_color_hex(0x213142);
    lv_color_t bar_bg = lv_color_hex(0xD4E4F8);

    switch (state)
    {
    case UI_STATE_PROVISIONING:
        bg_top = lv_color_hex(0xFFF7E8);
        bg_bottom = lv_color_hex(0xEAF3FF);
        chip_bg = lv_color_hex(0xE18A13);
        hint_bg = lv_color_hex(0xFFF3DD);
        bar_bg = lv_color_hex(0xF7E2BD);
        break;
    case UI_STATE_NETWORK_FAULT:
        bg_top = lv_color_hex(0xFFEDEE);
        bg_bottom = lv_color_hex(0xFFE2E4);
        chip_bg = lv_color_hex(0xD64545);
        hint_bg = lv_color_hex(0xFFE1E6);
        bar_bg = lv_color_hex(0xF5CDD2);
        break;
    case UI_STATE_ALERT:
        bg_top = lv_color_hex(0xFFF7EB);
        bg_bottom = lv_color_hex(0xFFEFD9);
        chip_bg = lv_color_hex(0xD46A1F);
        hint_bg = lv_color_hex(0xFFEED2);
        bar_bg = lv_color_hex(0xF6D9B2);
        break;
    case UI_STATE_NO_DATA:
        bg_top = lv_color_hex(0xEEF2F7);
        bg_bottom = lv_color_hex(0xE4EBF3);
        chip_bg = lv_color_hex(0x6E7E92);
        hint_bg = lv_color_hex(0xEDF2F8);
        bar_bg = lv_color_hex(0xD7E0EC);
        break;
    case UI_STATE_NORMAL:
    default:
        break;
    }

    lv_obj_set_style_bg_color(scr, bg_top, LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(scr, bg_bottom, LV_PART_MAIN);

    lv_obj_set_style_bg_color(lbl_state, chip_bg, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_state, chip_fg, LV_PART_MAIN);

    lv_obj_set_style_bg_color(panel_hint, hint_bg, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_hint, hint_fg, LV_PART_MAIN);

    lv_obj_set_style_bg_color(bar, bar_bg, LV_PART_MAIN);
}

static void update_cards(const sensor_data_t *d, const device_config_t *cfg)
{
    lv_label_set_text(lbl_card_title[0], k_card_titles[0]);
    lv_label_set_text(lbl_card_title[1], k_card_titles[1]);
    lv_label_set_text(lbl_card_title[3], k_card_titles[3]);
    if (cfg != NULL)
    {
        lv_label_set_text_fmt(lbl_card_title[2], "Soil (L<%.0f%%)", (float)cfg->threshold_lower);
    }
    else
    {
        lv_label_set_text(lbl_card_title[2], "Soil");
    }

    if (d == NULL)
    {
        lv_label_set_text(lbl_card_value[0], "--");
        lv_label_set_text(lbl_card_value[1], "--");
        lv_label_set_text(lbl_card_value[2], "--");
        lv_label_set_text(lbl_card_value[3], "--");
        return;
    }

    lv_label_set_text_fmt(lbl_card_value[0], "%.1f C", d->temp);
    lv_label_set_text_fmt(lbl_card_value[1], "%.1f %%", d->humidity);
    lv_label_set_text_fmt(lbl_card_value[2], "%.1f %%", d->soil);
    lv_label_set_text_fmt(lbl_card_value[3], "%lu lux", (unsigned long)d->light);
}

static void update_header_time(void)
{
    if (lbl_time == NULL)
    {
        return;
    }

    char full_ts[20] = {0};
    if (wifi_manager_get_local_time_string(full_ts, sizeof(full_ts)) == ESP_OK)
    {
        char hhmm[6] = {0};
        if (strlen(full_ts) >= 16)
        {
            memcpy(hhmm, &full_ts[11], 5);
            hhmm[5] = '\0';
            lv_label_set_text(lbl_time, hhmm);
            return;
        }
    }

    lv_label_set_text(lbl_time, "--:--");
}

static void init_default(void)
{
    scr = lv_scr_act();
    lv_obj_clean(scr);

    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr, 0, LV_PART_MAIN);

    const int margin = 8;
    const int header_h = 54;
    const int hint_h = 48;
    const int bar_h = 40;
    const int card_top = margin + header_h + margin;
    const int card_w = (LV_HOR_RES - (margin * 3)) / 2;
    const int card_h = (LV_VER_RES - card_top - hint_h - bar_h - (margin * 3)) / 2;

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
    lv_obj_set_style_pad_ver(panel_header, 8, LV_PART_MAIN);

    lbl_title = lv_label_create(panel_header);
    lv_obj_set_style_text_font(lbl_title, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x1B2A3A), LV_PART_MAIN);
    lv_obj_set_width(lbl_title, LV_HOR_RES - (margin * 2) - 170);
    lv_label_set_long_mode(lbl_title, LV_LABEL_LONG_DOT);
    lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 0, 0);

    lbl_state = lv_label_create(panel_header);
    lv_obj_set_style_text_font(lbl_state, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_radius(lbl_state, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_left(lbl_state, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_right(lbl_state, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_top(lbl_state, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(lbl_state, 5, LV_PART_MAIN);
    lv_obj_align(lbl_state, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    lbl_time = lv_label_create(panel_header);
    lv_obj_set_style_text_font(lbl_time, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_time, lv_color_hex(0x1B2A3A), LV_PART_MAIN);
    lv_obj_align(lbl_time, LV_ALIGN_TOP_RIGHT, 0, 0);

    for (int i = 0; i < 4; ++i)
    {
        int col = i % 2;
        int row = i / 2;
        int x = margin + col * (card_w + margin);
        int y = card_top + row * (card_h + margin);

        cards[i] = lv_obj_create(scr);
        lv_obj_set_size(cards[i], card_w, card_h);
        lv_obj_align(cards[i], LV_ALIGN_TOP_LEFT, x, y);
        lv_obj_set_style_radius(cards[i], 14, LV_PART_MAIN);
        lv_obj_set_style_bg_color(cards[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(cards[i], LV_OPA_90, LV_PART_MAIN);
        lv_obj_set_style_border_width(cards[i], 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(cards[i], lv_color_hex(0xD3DEEA), LV_PART_MAIN);
        lv_obj_set_style_shadow_width(cards[i], 10, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(cards[i], LV_OPA_20, LV_PART_MAIN);
        lv_obj_set_style_shadow_color(cards[i], lv_color_hex(0x617387), LV_PART_MAIN);
        lv_obj_set_style_pad_all(cards[i], 10, LV_PART_MAIN);

        card_accent[i] = lv_obj_create(cards[i]);
        lv_obj_set_size(card_accent[i], 4, card_h - 20);
        lv_obj_align(card_accent[i], LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_style_border_width(card_accent[i], 0, LV_PART_MAIN);
        lv_obj_set_style_radius(card_accent[i], 3, LV_PART_MAIN);
        lv_obj_set_style_bg_color(card_accent[i], lv_color_hex(k_card_accent_color[i]), LV_PART_MAIN);

        lbl_card_title[i] = lv_label_create(cards[i]);
        lv_obj_set_style_text_font(lbl_card_title[i], LV_FONT_DEFAULT, LV_PART_MAIN);
        lv_obj_set_style_text_color(lbl_card_title[i], lv_color_hex(0x4A5C72), LV_PART_MAIN);
        lv_obj_align(lbl_card_title[i], LV_ALIGN_TOP_LEFT, 12, 0);

        lbl_card_value[i] = lv_label_create(cards[i]);
        lv_obj_set_width(lbl_card_value[i], card_w - 26);
        lv_obj_set_style_text_font(lbl_card_value[i], &cjk_22, LV_PART_MAIN);
        lv_obj_set_style_text_color(lbl_card_value[i], lv_color_hex(0x102437), LV_PART_MAIN);
        lv_obj_set_style_text_align(lbl_card_value[i], LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_label_set_long_mode(lbl_card_value[i], LV_LABEL_LONG_CLIP);
        lv_obj_align(lbl_card_value[i], LV_ALIGN_BOTTOM_LEFT, 12, 2);
    }

    panel_hint = lv_obj_create(scr);
    lv_obj_set_size(panel_hint, LV_HOR_RES - margin * 2, hint_h);
    lv_obj_align(panel_hint, LV_ALIGN_BOTTOM_MID, 0, -(bar_h + margin));
    lv_obj_set_style_radius(panel_hint, 12, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel_hint, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel_hint, lv_color_hex(0xD3DEEA), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel_hint, LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_pad_left(panel_hint, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_right(panel_hint, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_top(panel_hint, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(panel_hint, 8, LV_PART_MAIN);

    lbl_hint = lv_label_create(panel_hint);
    lv_obj_set_width(lbl_hint, LV_HOR_RES - margin * 2 - 24);
    lv_obj_set_style_text_font(lbl_hint, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_label_set_long_mode(lbl_hint, LV_LABEL_LONG_WRAP);
    lv_obj_align(lbl_hint, LV_ALIGN_LEFT_MID, 0, 0);

    bar = lv_obj_create(scr);
    lv_obj_set_size(bar, LV_HOR_RES, bar_h);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_radius(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(bar, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_right(bar, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_top(bar, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(bar, 8, LV_PART_MAIN);

    lbl_bar_face = lv_label_create(bar);
    lv_obj_set_style_text_font(lbl_bar_face, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_bar_face, lv_color_hex(0x1B2A3A), LV_PART_MAIN);
    lv_obj_align(lbl_bar_face, LV_ALIGN_LEFT_MID, 0, 0);

    lbl_bar_net = lv_label_create(bar);
    lv_obj_set_style_text_font(lbl_bar_net, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_bar_net, lv_color_hex(0x1B2A3A), LV_PART_MAIN);
    lv_obj_set_width(lbl_bar_net, LV_HOR_RES - 140);
    lv_label_set_long_mode(lbl_bar_net, LV_LABEL_LONG_DOT);
    lv_obj_align(lbl_bar_net, LV_ALIGN_LEFT_MID, 30, 0);

    lbl_bar_batt = lv_label_create(bar);
    lv_obj_set_style_text_font(lbl_bar_batt, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_bar_batt, lv_color_hex(0x1B2A3A), LV_PART_MAIN);
    lv_obj_align(lbl_bar_batt, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_label_set_text(lbl_state, "BOOTING");
    lv_label_set_text(lbl_time, "--:--");
    lv_label_set_text(lbl_bar_face, "..");
    lv_label_set_text(lbl_bar_net, "Initializing...");
    lv_label_set_text(lbl_bar_batt, "--");
    lv_label_set_text(lbl_hint, "Waiting for first telemetry packet...");
    update_cards(NULL, NULL);
    apply_state_theme(UI_STATE_NO_DATA);
}

static void update_default(const sensor_data_t *d)
{
    const device_config_t *cfg = config_manager_get_config();
    cloud_http_status_t cloud = {0};
    (void)mqtt_client_get_status(&cloud);

    const char *device_name = (cfg != NULL && cfg->device_name[0] != '\0') ? cfg->device_name : "ESP32";
    const char *species = (cfg != NULL && cfg->species[0] != '\0') ? cfg->species : "Unknown";
    lv_label_set_text_fmt(lbl_title, "%s  |  %s", device_name, species);
    update_header_time();

    update_cards(d, cfg);

    char net_text[160] = {0};
    char hint[240] = {0};
    char batt_text[24] = "--";
    char ip[20] = {0};
    char ap_ssid[33] = {0};
    char ap_pass[65] = {0};

    ui_state_t state = decide_ui_state(d, cfg, &cloud);
    if (d != NULL)
    {
        snprintf(batt_text, sizeof(batt_text), "BAT %u%%", (unsigned int)d->battery);
    }
    lv_label_set_text(lbl_bar_batt, batt_text);

    if (state == UI_STATE_PROVISIONING)
    {
        (void)wifi_manager_get_provision_ap_ssid(ap_ssid, sizeof(ap_ssid));
        (void)wifi_manager_get_provision_ap_password(ap_pass, sizeof(ap_pass));
        snprintf(net_text, sizeof(net_text), "AP %s", ap_ssid[0] != '\0' ? ap_ssid : "SmartIrrigation");
        snprintf(hint, sizeof(hint),
                 "Connect AP: %s   PWD: %s   Then open http://192.168.4.1",
                 ap_ssid[0] != '\0' ? ap_ssid : "SmartIrrigation",
                 ap_pass[0] != '\0' ? ap_pass : "12345678");
        lv_label_set_text(lbl_state, "PROVISION");
        lv_label_set_text(lbl_bar_face, "AP");
    }
    else if (state == UI_STATE_NO_DATA)
    {
        snprintf(net_text, sizeof(net_text), "Waiting sensor data");
        snprintf(hint, sizeof(hint), "No telemetry frame from STM32 yet.");
        lv_label_set_text(lbl_state, "NO DATA");
        lv_label_set_text(lbl_bar_face, "..");
    }
    else if (state == UI_STATE_NETWORK_FAULT)
    {
        bool wifi_ok = wifi_manager_is_connected();
        if (wifi_ok)
        {
            if (wifi_manager_get_sta_ip_string(ip, sizeof(ip)) == ESP_OK)
            {
                snprintf(net_text, sizeof(net_text), "WiFi OK %s, cloud issue", ip);
            }
            else
            {
                snprintf(net_text, sizeof(net_text), "WiFi OK, cloud issue");
            }
            snprintf(hint, sizeof(hint), "Cloud error: %s (HTTP:%d)",
                     cloud.last_error_text[0] != '\0' ? cloud.last_error_text : "unknown",
                     cloud.last_http_status);
        }
        else
        {
            snprintf(net_text, sizeof(net_text), "WiFi disconnected");
            snprintf(hint, sizeof(hint), "Wi-Fi reconnecting...");
        }
        lv_label_set_text(lbl_state, "NETWORK");
        lv_label_set_text(lbl_bar_face, "ER");
    }
    else if (state == UI_STATE_ALERT)
    {
        if (wifi_manager_get_sta_ip_string(ip, sizeof(ip)) == ESP_OK)
        {
            snprintf(net_text, sizeof(net_text), "WiFi %s", ip);
        }
        else
        {
            snprintf(net_text, sizeof(net_text), "WiFi connected");
        }
        snprintf(hint, sizeof(hint),
                 "Attention: soil %.1f%%  threshold %.1f%%  water=%s",
                 d != NULL ? d->soil : 0.0f,
                 cfg != NULL ? (float)cfg->threshold_lower : 0.0f,
                 d != NULL ? d->water : "unknown");
        lv_label_set_text(lbl_state, "ALERT");
        lv_label_set_text(lbl_bar_face, "AL");
    }
    else
    {
        if (wifi_manager_get_sta_ip_string(ip, sizeof(ip)) == ESP_OK)
        {
            snprintf(net_text, sizeof(net_text), "WiFi %s", ip);
        }
        else
        {
            snprintf(net_text, sizeof(net_text), "WiFi connected");
        }
        snprintf(hint, sizeof(hint),
                 "Cloud U:%lu/%lu  P:%lu/%lu  CMD:%lu",
                 (unsigned long)cloud.upload_ok_count,
                 (unsigned long)cloud.upload_fail_count,
                 (unsigned long)cloud.poll_ok_count,
                 (unsigned long)cloud.poll_fail_count,
                 (unsigned long)cloud.recv_cmd_count);
        lv_label_set_text(lbl_state, "RUNNING");
        lv_label_set_text(lbl_bar_face, "OK");
    }

    lv_label_set_text(lbl_bar_net, net_text);
    lv_label_set_text(lbl_hint, hint);
    apply_state_theme(state);

    for (int i = 0; i < 4; ++i)
    {
        lv_obj_set_style_bg_color(cards[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_border_color(cards[i], lv_color_hex(0xD3DEEA), LV_PART_MAIN);
        lv_obj_set_style_bg_color(card_accent[i], lv_color_hex(k_card_accent_color[i]), LV_PART_MAIN);
    }
    if (state == UI_STATE_ALERT)
    {
        lv_obj_set_style_bg_color(cards[2], lv_color_hex(0xFFEADB), LV_PART_MAIN);
        lv_obj_set_style_border_color(cards[2], lv_color_hex(0xE5A06A), LV_PART_MAIN);
        lv_obj_set_style_bg_color(card_accent[2], lv_color_hex(0xD46A1F), LV_PART_MAIN);
    }
}

const ui_module_t ui_default = {
    .init = init_default,
    .update = update_default,
};
