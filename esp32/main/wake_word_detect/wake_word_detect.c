#include "wake_word_detect.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "WakeDetect";

#if WAKE_WORD_SR_AVAILABLE

typedef struct {
    EventGroupHandle_t evt;
    TaskHandle_t task;

    const esp_afe_sr_iface_t *iface;
    esp_afe_sr_data_t *afe;
    srmodel_list_t *models;

    int channels;
    bool reference;

    char model_name[64];
    char wake_words[128];

    void (*cb)(const char *word);

    size_t feed_chunksize;
    size_t feed_channels;
} wake_det_t;

static const char *preferred_wakenet_model_from_menuconfig(void)
{
#if defined(CONFIG_SR_WN_WN9_HILEXIN) && CONFIG_SR_WN_WN9_HILEXIN
    return "wn9_hilexin";
#elif defined(CONFIG_SR_WN_WN9_HIESP) && CONFIG_SR_WN_WN9_HIESP
    return "wn9_hiesp";
#elif defined(CONFIG_SR_WN_WN9_XIAOAITONGXUE) && CONFIG_SR_WN_WN9_XIAOAITONGXUE
    return "wn9_xiaoaitongxue";
#elif defined(CONFIG_SR_WN_WN9_ALEXA) && CONFIG_SR_WN_WN9_ALEXA
    return "wn9_alexa";
#elif defined(CONFIG_SR_WN_WN8_HILEXIN) && CONFIG_SR_WN_WN8_HILEXIN
    return "wn8_hilexin";
#elif defined(CONFIG_SR_WN_WN8_HIESP) && CONFIG_SR_WN_WN8_HIESP
    return "wn8_hiesp";
#elif defined(CONFIG_SR_WN_WN8_ALEXA) && CONFIG_SR_WN_WN8_ALEXA
    return "wn8_alexa";
#elif defined(CONFIG_SR_WN_WN5_HILEXIN) && CONFIG_SR_WN_WN5_HILEXIN
    return "wn5_hilexin";
#elif defined(CONFIG_SR_WN_WN5X2_HILEXIN) && CONFIG_SR_WN_WN5X2_HILEXIN
    return "wn5_hilexinX2";
#elif defined(CONFIG_SR_WN_WN5X3_HILEXIN) && CONFIG_SR_WN_WN5X3_HILEXIN
    return "wn5_hilexinX3";
#elif defined(CONFIG_SR_WN_WN5_NIHAOXIAOZHI) && CONFIG_SR_WN_WN5_NIHAOXIAOZHI
    return "wn5_nihaoxiaozhi";
#elif defined(CONFIG_SR_WN_WN5X2_NIHAOXIAOZHI) && CONFIG_SR_WN_WN5X2_NIHAOXIAOZHI
    return "wn5_nihaoxiaozhiX2";
#elif defined(CONFIG_SR_WN_WN5X3_NIHAOXIAOZHI) && CONFIG_SR_WN_WN5X3_NIHAOXIAOZHI
    return "wn5_nihaoxiaozhiX3";
#else
    return NULL;
#endif
}

static const char *select_wakenet_model(srmodel_list_t *models, const char *model_name)
{
    if (models == NULL) {
        return NULL;
    }

    const char *preferred = preferred_wakenet_model_from_menuconfig();
    if (preferred != NULL && esp_srmodel_exists(models, (char *)preferred) >= 0) {
        return preferred;
    }

    if (model_name != NULL && model_name[0] != '\0') {
        if (esp_srmodel_exists(models, (char *)model_name) >= 0) {
            return model_name;
        }
        ESP_LOGW(TAG, "requested model not found: %s", model_name);
    }

    if (strcmp(WAKENET_MODEL_NAME, "NULL") != 0 &&
        esp_srmodel_exists(models, (char *)WAKENET_MODEL_NAME) >= 0) {
        return WAKENET_MODEL_NAME;
    }

    static const char *fallback_models[] = {
        "wn9_hilexin",
        "wn9_hiesp",
        "wn8_hilexin",
        "wn8_hiesp",
        "wn5_hilexin",
    };

    for (size_t i = 0; i < sizeof(fallback_models) / sizeof(fallback_models[0]); ++i) {
        if (esp_srmodel_exists(models, (char *)fallback_models[i]) >= 0) {
            return fallback_models[i];
        }
    }

    size_t wn_prefix_len = strlen(ESP_WN_PREFIX);
    for (int i = 0; i < models->num; ++i) {
        const char *name = models->model_name[i];
        if (name == NULL) {
            continue;
        }
        if (strncmp(name, ESP_WN_PREFIX, wn_prefix_len) != 0) {
            continue;
        }
        if (strstr(name, "tts") != NULL || strstr(name, "customword") != NULL) {
            continue;
        }
        return name;
    }

    return esp_srmodel_filter(models, ESP_WN_PREFIX, NULL);
}

static void wake_task(void *arg)
{
    wake_det_t *w = (wake_det_t *)arg;

    ESP_LOGI(TAG,
             "Wake task started, model=%s words=%s feed=%u",
             w->model_name,
             w->wake_words,
             (unsigned)(w->feed_chunksize * w->feed_channels));

    for (;;) {
        xEventGroupWaitBits(w->evt, WAKE_DETECT_RUNNING_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

        afe_fetch_result_t *res = w->iface->fetch(w->afe);
        if (res == NULL) {
            continue;
        }

        if (res->ret_value == ESP_OK && res->wakeup_state == WAKENET_DETECTED && w->cb != NULL) {
            const char *word = (w->wake_words[0] != '\0') ? w->wake_words : w->model_name;
            w->cb(word);
        }
    }
}

wake_detector_handle_t wake_detector_create(int channels, bool reference, const char *model_name)
{
    if (channels <= 0) {
        ESP_LOGE(TAG, "invalid channels=%d", channels);
        return NULL;
    }

    wake_det_t *w = calloc(1, sizeof(wake_det_t));
    if (w == NULL) {
        return NULL;
    }

    w->channels = channels;
    w->reference = reference;

    w->evt = xEventGroupCreate();
    if (w->evt == NULL) {
        free(w);
        return NULL;
    }

    w->models = esp_srmodel_init("model");
    if (w->models == NULL) {
        ESP_LOGE(TAG, "esp_srmodel_init failed, partition label=model");
        vEventGroupDelete(w->evt);
        free(w);
        return NULL;
    }

    const char *selected_model = select_wakenet_model(w->models, model_name);
    if (selected_model == NULL || selected_model[0] == '\0') {
        ESP_LOGE(TAG, "no available wakenet model");
        esp_srmodel_deinit(w->models);
        vEventGroupDelete(w->evt);
        free(w);
        return NULL;
    }

    int selected_idx = esp_srmodel_exists(w->models, (char *)selected_model);
    if (selected_idx < 0 || selected_idx >= w->models->num || w->models->model_name[selected_idx] == NULL) {
        ESP_LOGE(TAG, "selected model is invalid: %s", selected_model);
        esp_srmodel_deinit(w->models);
        vEventGroupDelete(w->evt);
        free(w);
        return NULL;
    }

    char *selected_model_ptr = w->models->model_name[selected_idx];
    strncpy(w->model_name, selected_model_ptr, sizeof(w->model_name) - 1);

    char *wake_words = esp_srmodel_get_wake_words(w->models, selected_model_ptr);
    if (wake_words != NULL) {
        strncpy(w->wake_words, wake_words, sizeof(w->wake_words) - 1);
        free(wake_words);
    } else {
        ESP_LOGW(TAG, "wake words not found for model: %s", selected_model_ptr);
    }

    afe_config_t cfg = AFE_CONFIG_DEFAULT();
    cfg.aec_init = reference;
    cfg.se_init = true;
    cfg.vad_init = true;
    cfg.wakenet_init = true;
    cfg.voice_communication_init = false;
    cfg.voice_communication_agc_init = false;
    cfg.wakenet_model_name = selected_model_ptr;
    cfg.wakenet_model_name_2 = NULL;
    cfg.pcm_config.total_ch_num = channels;
    cfg.pcm_config.ref_num = reference ? 1 : 0;
    cfg.pcm_config.mic_num = channels - cfg.pcm_config.ref_num;
    cfg.pcm_config.sample_rate = 16000;

    if (cfg.pcm_config.mic_num <= 0) {
        ESP_LOGE(TAG, "invalid pcm channels: total=%d mic=%d ref=%d",
                 cfg.pcm_config.total_ch_num,
                 cfg.pcm_config.mic_num,
                 cfg.pcm_config.ref_num);
        esp_srmodel_deinit(w->models);
        vEventGroupDelete(w->evt);
        free(w);
        return NULL;
    }

    if (channels == 1) {
        cfg.wakenet_mode = DET_MODE_90;
    }

    ESP_LOGI(TAG, "using wakenet model=%s (idx=%d)", selected_model_ptr, selected_idx);

    w->iface = &ESP_AFE_SR_HANDLE;
    w->afe = w->iface->create_from_config(&cfg);
    if (w->afe == NULL) {
        ESP_LOGE(TAG, "afe create_from_config failed");
        esp_srmodel_deinit(w->models);
        vEventGroupDelete(w->evt);
        free(w);
        return NULL;
    }

    w->feed_chunksize = (size_t)w->iface->get_feed_chunksize(w->afe);
    w->feed_channels = (size_t)channels;

    BaseType_t ok = xTaskCreate(wake_task, "wake_task", 4096, w, 5, &w->task);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "xTaskCreate(wake_task) failed");
        w->iface->destroy(w->afe);
        esp_srmodel_deinit(w->models);
        vEventGroupDelete(w->evt);
        free(w);
        return NULL;
    }

    ESP_LOGI(TAG, "Wake detector ready: model=%s words=%s", w->model_name, w->wake_words);
    return w;
}

esp_err_t wake_detector_feed(wake_detector_handle_t h, const int16_t *samples)
{
    wake_det_t *w = (wake_det_t *)h;
    if (w == NULL || samples == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int ret = w->iface->feed(w->afe, samples);
    return (ret > 0) ? ESP_OK : ESP_FAIL;
}

void wake_detector_start(wake_detector_handle_t h)
{
    wake_det_t *w = (wake_det_t *)h;
    if (w == NULL) {
        return;
    }
    xEventGroupSetBits(w->evt, WAKE_DETECT_RUNNING_BIT);
}

void wake_detector_stop(wake_detector_handle_t h)
{
    wake_det_t *w = (wake_det_t *)h;
    if (w == NULL) {
        return;
    }
    xEventGroupClearBits(w->evt, WAKE_DETECT_RUNNING_BIT);
}

void wake_detector_set_callback(wake_detector_handle_t h, void (*cb)(const char *word))
{
    wake_det_t *w = (wake_det_t *)h;
    if (w == NULL) {
        return;
    }
    w->cb = cb;
}

size_t wake_detector_get_feed_samples(wake_detector_handle_t h)
{
    wake_det_t *w = (wake_det_t *)h;
    if (w == NULL) {
        return 0;
    }
    return w->feed_chunksize * w->feed_channels;
}

const char *wake_detector_get_model_name(wake_detector_handle_t h)
{
    wake_det_t *w = (wake_det_t *)h;
    if (w == NULL) {
        return NULL;
    }
    return w->model_name;
}

const char *wake_detector_get_wake_words(wake_detector_handle_t h)
{
    wake_det_t *w = (wake_det_t *)h;
    if (w == NULL) {
        return NULL;
    }
    return w->wake_words;
}

void wake_detector_destroy(wake_detector_handle_t h)
{
    wake_det_t *w = (wake_det_t *)h;
    if (w == NULL) {
        return;
    }

    if (w->task != NULL) {
        vTaskDelete(w->task);
        w->task = NULL;
    }

    if (w->iface != NULL && w->afe != NULL) {
        w->iface->destroy(w->afe);
        w->afe = NULL;
    }

    if (w->models != NULL) {
        esp_srmodel_deinit(w->models);
        w->models = NULL;
    }

    if (w->evt != NULL) {
        vEventGroupDelete(w->evt);
        w->evt = NULL;
    }

    free(w);
}

#else

wake_detector_handle_t wake_detector_create(int channels, bool reference, const char *model_name)
{
    (void)channels;
    (void)reference;
    (void)model_name;
    ESP_LOGW(TAG, "ESP-SR unavailable, wake detector disabled");
    return NULL;
}

esp_err_t wake_detector_feed(wake_detector_handle_t h, const int16_t *samples)
{
    (void)h;
    (void)samples;
    return ESP_ERR_NOT_SUPPORTED;
}

void wake_detector_start(wake_detector_handle_t h)
{
    (void)h;
}

void wake_detector_stop(wake_detector_handle_t h)
{
    (void)h;
}

void wake_detector_set_callback(wake_detector_handle_t h, void (*cb)(const char *word))
{
    (void)h;
    (void)cb;
}

size_t wake_detector_get_feed_samples(wake_detector_handle_t h)
{
    (void)h;
    return 0;
}

const char *wake_detector_get_model_name(wake_detector_handle_t h)
{
    (void)h;
    return NULL;
}

const char *wake_detector_get_wake_words(wake_detector_handle_t h)
{
    (void)h;
    return NULL;
}

void wake_detector_destroy(wake_detector_handle_t h)
{
    (void)h;
}

#endif
