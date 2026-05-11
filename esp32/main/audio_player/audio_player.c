// audio_player.c
#include "audio_player.h"
#include "driver/i2s.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const char *TAG = "AUDIO_PLR";

esp_err_t audio_player_init(void)
{
    i2s_config_t i2s_cfg = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = AUDIO_SAMPLE_RATE,
        .bits_per_sample = AUDIO_BITS_PER_SAMPLE,
        .channel_format = AUDIO_CHANNEL_FORMAT,
        .communication_format = AUDIO_COMM_FORMAT,
        .dma_buf_count = AUDIO_DMA_BUF_COUNT,
        .dma_buf_len = AUDIO_DMA_BUF_LEN,
        .use_apll = false,
        .intr_alloc_flags = 0};
    i2s_pin_config_t pin_cfg = {
        .bck_io_num = AUDIO_I2S_SPK_GPIO_BCLK,
        .ws_io_num = AUDIO_I2S_SPK_GPIO_WS,
        .data_out_num = AUDIO_I2S_SPK_GPIO_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE};
    esp_err_t ret;

    ret = i2s_driver_install(AUDIO_I2S_NUM, &i2s_cfg, 0, NULL);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "i2s_driver_install failed: %d", ret);
        return ret;
    }
    ret = i2s_set_pin(AUDIO_I2S_NUM, &pin_cfg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "i2s_set_pin failed: %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "I2S initialized for MAX98357A");
    return ESP_OK;
}

esp_err_t audio_player_play(const uint8_t *data, size_t length)
{
    if (!data || length == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }
    size_t bytes_written = 0;
    esp_err_t ret = i2s_write(AUDIO_I2S_NUM, data, length, &bytes_written, portMAX_DELAY);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "i2s_write failed: %d", ret);
    }
    return ret;
}

esp_err_t audio_player_play_tone(uint32_t freq_hz, uint32_t duration_ms)
{
    if (freq_hz == 0 || duration_ms == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }
    const int sample_rate = AUDIO_SAMPLE_RATE;
    const int16_t amplitude = 2000;
    size_t samples = (size_t)(sample_rate * duration_ms / 1000);
    // Allocate DMA-capable buffer for stereo samples
    size_t buf_size = samples * sizeof(uint16_t) * 2;
    uint16_t *buffer = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    if (!buffer)
    {
        ESP_LOGE(TAG, "Failed to allocate tone buffer");
        return ESP_ERR_NO_MEM;
    }
    for (size_t i = 0; i < samples; i++)
    {
        float phase = 2.0f * M_PI * freq_hz * ((float)i / sample_rate);
        uint16_t val = (uint16_t)((amplitude * sinf(phase)) + 0x8000);
        buffer[2 * i + 0] = val; // left channel
        buffer[2 * i + 1] = val; // right channel
    }
    esp_err_t ret = audio_player_play((const uint8_t *)buffer, buf_size);
    heap_caps_free(buffer);
    return ret;
}