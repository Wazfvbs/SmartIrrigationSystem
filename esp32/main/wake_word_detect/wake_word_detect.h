#ifndef WAKE_WORD_DETECT_H
#define WAKE_WORD_DETECT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#if __has_include("esp_afe_sr_models.h") && __has_include("model_path.h")
#include "esp_afe_sr_models.h"
#include "model_path.h"
#define WAKE_WORD_SR_AVAILABLE 1
#else
#define WAKE_WORD_SR_AVAILABLE 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define WAKE_DETECT_RUNNING_BIT (1 << 0)

typedef void *wake_detector_handle_t;

wake_detector_handle_t wake_detector_create(int channels, bool reference, const char *model_name);
esp_err_t wake_detector_feed(wake_detector_handle_t h, const int16_t *samples);
void wake_detector_start(wake_detector_handle_t h);
void wake_detector_stop(wake_detector_handle_t h);
void wake_detector_set_callback(wake_detector_handle_t h, void (*cb)(const char *word));
size_t wake_detector_get_feed_samples(wake_detector_handle_t h);
const char *wake_detector_get_model_name(wake_detector_handle_t h);
const char *wake_detector_get_wake_words(wake_detector_handle_t h);
void wake_detector_destroy(wake_detector_handle_t h);

#ifdef __cplusplus
}
#endif

#endif
