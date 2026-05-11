
// components/audio_processor/audio_processor.c
#include "audio_processor.h"
#include "esp_afe_vc_api.h"
#include "esp_log.h"
#include <stdlib.h>

static const char *TAG = "AudioProc";

typedef struct
{
    EventGroupHandle_t evt;
    esp_afe_vc_handle_t afe;
    int channels;
    bool reference;
    void (*output_cb)(const int16_t *, size_t);
    TaskHandle_t task;
} audio_proc_t;

static void audio_processor_task(void *arg)
{
    audio_proc_t *p = (audio_proc_t *)arg;
    size_t fetch, feed;
    fetch = esp_afe_vc_get_fetch_chunksize(p->afe);
    feed = esp_afe_vc_get_feed_chunksize(p->afe) * p->channels;
    ESP_LOGI(TAG, "Task started, feed=%u fetch=%u", (unsigned)feed, (unsigned)fetch);
    for (;;)
    {
        xEventGroupWaitBits(p->evt, AUDIO_PROCESSOR_RUNNING_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
        esp_afe_vc_data_t *res = esp_afe_vc_fetch(p->afe);
        if (res && res->ret_value == ESP_OK && p->output_cb)
        {
            p->output_cb((int16_t *)res->data, res->data_size / sizeof(int16_t));
        }
    }
}

audio_processor_handle_t audio_processor_create(int channels, bool reference)
{
    audio_proc_t *p = calloc(1, sizeof(audio_proc_t));
    if (!p)
        return NULL;
    p->evt = xEventGroupCreate();
    p->channels = channels;
    p->reference = reference;
    afe_config_t cfg = {
        .aec_init = reference,
        .se_init = true,
        .vad_init = false,
        .wakenet_init = false,
        .voice_communication_init = true,
        .voice_communication_agc_init = true,
        .pcm_config = {
            .total_ch_num = channels,
            .mic_num = reference ? channels - 1 : channels,
            .ref_num = reference ? 1 : 0,
            .sample_rate = 16000}};
    p->afe = esp_afe_vc_create_from_config(&cfg);
    if (!p->afe)
    {
        free(p);
        return NULL;
    }
    xTaskCreate(audio_processor_task, "audio_proc", 4096, p, 5, &p->task);
    return p;
}

esp_err_t audio_processor_feed(audio_processor_handle_t h, const int16_t *samples)
{
    audio_proc_t *p = (audio_proc_t *)h;
    if (!p || !samples)
        return ESP_ERR_INVALID_ARG;
    return esp_afe_vc_feed(p->afe, (int16_t *)samples);
}

void audio_processor_start(audio_processor_handle_t h)
{
    audio_proc_t *p = (audio_proc_t *)h;
    xEventGroupSetBits(p->evt, AUDIO_PROCESSOR_RUNNING_BIT);
}

void audio_processor_stop(audio_processor_handle_t h)
{
    audio_proc_t *p = (audio_proc_t *)h;
    xEventGroupClearBits(p->evt, AUDIO_PROCESSOR_RUNNING_BIT);
}

void audio_processor_set_output_cb(audio_processor_handle_t h, void (*cb)(const int16_t *, size_t))
{
    audio_proc_t *p = (audio_proc_t *)h;
    p->output_cb = cb;
}

void audio_processor_destroy(audio_processor_handle_t h)
{
    audio_proc_t *p = (audio_proc_t *)h;
    if (!p)
        return;
    vTaskDelete(p->task);
    esp_afe_vc_destroy(p->afe);
    vEventGroupDelete(p->evt);
    free(p);
}
