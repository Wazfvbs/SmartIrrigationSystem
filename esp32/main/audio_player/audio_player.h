// audio_player.h
#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <stddef.h>
#include "esp_err.h"
#include "driver/i2s.h"
#include "../config.h"
// MAX98357A I2S pin definitions
// I2S configuration
#define AUDIO_I2S_NUM (I2S_NUM_0)
#define AUDIO_SAMPLE_RATE (44100)
#define AUDIO_BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_16BIT
#define AUDIO_CHANNEL_FORMAT I2S_CHANNEL_FMT_RIGHT_LEFT
#define AUDIO_COMM_FORMAT I2S_COMM_FORMAT_I2S_MSB
#define AUDIO_DMA_BUF_COUNT (4)
#define AUDIO_DMA_BUF_LEN (512)

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Initialize I2S peripheral for MAX98357A audio output
     */
    esp_err_t audio_player_init(void);

    /**
     * @brief Play raw PCM data (16-bit stereo)
     * @param data Pointer to PCM buffer
     * @param length Length in bytes
     */
    esp_err_t audio_player_play(const uint8_t *data, size_t length);

    /**
     * @brief Generate and play a tone of given frequency and duration
     *        (blocking call)
     * @param freq_hz Frequency in Hz
     * @param duration_ms Duration in milliseconds
     */
    esp_err_t audio_player_play_tone(uint32_t freq_hz, uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_PLAYER_H
