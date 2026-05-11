// components/audio_processor/audio_processor.h
#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_afe_sr_models.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define AUDIO_PROCESSOR_RUNNING_BIT (1 << 0)

    /** Opaque handle for audio processor */
    typedef void *audio_processor_handle_t;

    /**
     * @brief Create and initialize the audio processor
     * @param channels number of audio channels
     * @param reference set true if reference channel (AEC)
     * @return handle or NULL on error
     */
    audio_processor_handle_t audio_processor_create(int channels, bool reference);

    /**
     * @brief Feed PCM data into the processor
     * @param h handle from audio_processor_create
     * @param samples pointer to int16_t samples, length = chunk_size*channels
     */
    esp_err_t audio_processor_feed(audio_processor_handle_t h, const int16_t *samples);

    /**
     * @brief Start processing (enables task)
     */
    void audio_processor_start(audio_processor_handle_t h);

    /**
     * @brief Stop processing
     */
    void audio_processor_stop(audio_processor_handle_t h);

    /**
     * @brief Register callback invoked with processed PCM data
     * @param h handle
     * @param cb function pointer: cb(data, length)
     */
    void audio_processor_set_output_cb(audio_processor_handle_t h, void (*cb)(const int16_t *data, size_t length));

    /**
     * @brief Destroy processor and free resources
     */
    void audio_processor_destroy(audio_processor_handle_t h);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_PROCESSOR_H