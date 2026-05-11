#include "voice_manager.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "driver/i2s.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#if __has_include("esp_mn_models.h") && __has_include("esp_mn_speech_commands.h")
#include "esp_mn_models.h"
#include "esp_mn_speech_commands.h"
#include "model_path.h"
#define VOICE_SR_CMD_AVAILABLE 1
#else
#define VOICE_SR_CMD_AVAILABLE 0
#endif

#include "../audio_player/audio_player.h"
#include "../config.h"
#include "../stm32_protocol/stm32_protocol.h"
#include "../uart_receiver/uart_receiver.h"
#include "../wake_word_detect/wake_word_detect.h"

static const char *TAG = "VOICE_MGR";

#define VOICE_I2S_PORT I2S_NUM_1
#define VOICE_QUEUE_LEN 8
#define VOICE_TEXT_MAX 64
#define VOICE_AWAKE_WINDOW_MS 5000
#define VOICE_COOLDOWN_MS 1200
#define VOICE_DEFAULT_FEED_SAMPLES 512
#define VOICE_STATE_TASK_STACK 6144
#define VOICE_MIC_TASK_STACK 12288

enum {
    VOICE_MN_ID_QUERY_TEMP = 1001,
    VOICE_MN_ID_QUERY_HUMIDITY,
    VOICE_MN_ID_WATER_START,
    VOICE_MN_ID_WATER_STOP,
    VOICE_MN_ID_MODE_AUTO,
    VOICE_MN_ID_MODE_MANUAL,
};

typedef enum {
    VOICE_STATE_IDLE = 0,
    VOICE_STATE_AWAKE_LISTEN,
    VOICE_STATE_EXECUTE,
    VOICE_STATE_SPEAK,
    VOICE_STATE_COOLDOWN,
} voice_state_t;

typedef enum {
    VOICE_EVT_WAKE_WORD = 0,
    VOICE_EVT_TEXT_CMD,
    VOICE_EVT_MN_CMD,
} voice_evt_type_t;

typedef struct {
    voice_evt_type_t type;
    char text[VOICE_TEXT_MAX];
    int command_id;
} voice_evt_t;

typedef enum {
    VOICE_CMD_NONE = 0,
    VOICE_CMD_QUERY_TEMP,
    VOICE_CMD_QUERY_HUMIDITY,
    VOICE_CMD_WATER_START,
    VOICE_CMD_WATER_STOP,
    VOICE_CMD_MODE_AUTO,
    VOICE_CMD_MODE_MANUAL,
} voice_cmd_t;

static QueueHandle_t s_evt_queue = NULL;
static TaskHandle_t s_voice_task = NULL;
static TaskHandle_t s_mic_task = NULL;
static wake_detector_handle_t s_wake = NULL;

#if VOICE_SR_CMD_AVAILABLE
static const esp_mn_iface_t *s_mn = NULL;
static model_iface_data_t *s_mn_data = NULL;
#endif

static bool s_initialized = false;
static bool s_running = false;
static bool s_mic_ready = false;
static bool s_mn_ready = false;
static bool s_mn_commands_allocated = false;
static bool s_sr_runtime_allowed = true;
static volatile voice_state_t s_state = VOICE_STATE_IDLE;
static TickType_t s_awake_until = 0;
static TickType_t s_cooldown_until = 0;
static size_t s_feed_samples = VOICE_DEFAULT_FEED_SAMPLES;
static size_t s_mn_feed_samples = 0;

static char s_wake_model[64] = {0};
static char s_wake_words[128] = {0};
static char s_mn_model[64] = {0};

static uint32_t s_wake_count = 0;
static uint32_t s_cmd_ok_count = 0;
static uint32_t s_cmd_fail_count = 0;
static uint32_t s_mn_detect_count = 0;

static bool contains_ascii_ci(const char *text, const char *token)
{
    if (text == NULL || token == NULL) {
        return false;
    }

    size_t text_len = strlen(text);
    size_t token_len = strlen(token);
    if (token_len == 0 || token_len > text_len) {
        return false;
    }

    for (size_t i = 0; i <= text_len - token_len; ++i) {
        bool ok = true;
        for (size_t j = 0; j < token_len; ++j) {
            if (tolower((unsigned char)text[i + j]) != tolower((unsigned char)token[j])) {
                ok = false;
                break;
            }
        }
        if (ok) {
            return true;
        }
    }

    return false;
}

static bool text_contains(const char *text, const char *token)
{
    if (text == NULL || token == NULL) {
        return false;
    }

    if (strstr(text, token) != NULL) {
        return true;
    }

    return contains_ascii_ci(text, token);
}

static voice_cmd_t parse_command_text(const char *text)
{
    if (text == NULL || text[0] == '\0') {
        return VOICE_CMD_NONE;
    }

    if (text_contains(text, "停止浇水") ||
        (text_contains(text, "停止") && text_contains(text, "浇水")) ||
        text_contains(text, "stop watering") ||
        text_contains(text, "water off")) {
        return VOICE_CMD_WATER_STOP;
    }

    if (text_contains(text, "开始浇水") ||
        text_contains(text, "启动浇水") ||
        (text_contains(text, "开始") && text_contains(text, "浇水")) ||
        text_contains(text, "start watering") ||
        text_contains(text, "water on")) {
        return VOICE_CMD_WATER_START;
    }

    if (text_contains(text, "查询温度") ||
        text_contains(text, "现在温度") ||
        text_contains(text, "温度") ||
        text_contains(text, "temperature")) {
        return VOICE_CMD_QUERY_TEMP;
    }

    if (text_contains(text, "查询湿度") ||
        text_contains(text, "现在湿度") ||
        text_contains(text, "湿度") ||
        text_contains(text, "humidity")) {
        return VOICE_CMD_QUERY_HUMIDITY;
    }

    if (text_contains(text, "自动模式") ||
        (text_contains(text, "自动") && text_contains(text, "模式")) ||
        text_contains(text, "auto mode")) {
        return VOICE_CMD_MODE_AUTO;
    }

    if (text_contains(text, "手动模式") ||
        (text_contains(text, "手动") && text_contains(text, "模式")) ||
        text_contains(text, "manual mode")) {
        return VOICE_CMD_MODE_MANUAL;
    }

    return VOICE_CMD_NONE;
}

static voice_cmd_t map_mn_command_id(int command_id)
{
    switch (command_id) {
    case VOICE_MN_ID_QUERY_TEMP:
        return VOICE_CMD_QUERY_TEMP;
    case VOICE_MN_ID_QUERY_HUMIDITY:
        return VOICE_CMD_QUERY_HUMIDITY;
    case VOICE_MN_ID_WATER_START:
        return VOICE_CMD_WATER_START;
    case VOICE_MN_ID_WATER_STOP:
        return VOICE_CMD_WATER_STOP;
    case VOICE_MN_ID_MODE_AUTO:
        return VOICE_CMD_MODE_AUTO;
    case VOICE_MN_ID_MODE_MANUAL:
        return VOICE_CMD_MODE_MANUAL;
    default:
        return VOICE_CMD_NONE;
    }
}

static void play_ready_tone(void)
{
    (void)audio_player_play_tone(1500, 90);
}

static void play_ok_tone_by_cmd(voice_cmd_t cmd)
{
    switch (cmd) {
    case VOICE_CMD_WATER_START:
        (void)audio_player_play_tone(1300, 70);
        (void)audio_player_play_tone(1700, 80);
        break;
    case VOICE_CMD_WATER_STOP:
        (void)audio_player_play_tone(1700, 70);
        (void)audio_player_play_tone(1200, 80);
        break;
    case VOICE_CMD_QUERY_TEMP:
    case VOICE_CMD_QUERY_HUMIDITY:
        (void)audio_player_play_tone(1200, 70);
        (void)audio_player_play_tone(1450, 80);
        break;
    default:
        (void)audio_player_play_tone(1400, 80);
        (void)audio_player_play_tone(1800, 90);
        break;
    }
}

static void play_error_tone(void)
{
    (void)audio_player_play_tone(420, 160);
}

static void play_timeout_tone(void)
{
    (void)audio_player_play_tone(700, 80);
    (void)audio_player_play_tone(500, 80);
}

static void set_state(voice_state_t next)
{
    if (s_state == next) {
        return;
    }
    s_state = next;
    ESP_LOGI(TAG, "state=%d", (int)s_state);
}

static esp_err_t execute_command(voice_cmd_t cmd)
{
    switch (cmd) {
    case VOICE_CMD_QUERY_TEMP: {
        const sensor_data_t *d = uart_receiver_get_latest_data();
        if (d == NULL) {
            ESP_LOGW(TAG, "voice temp query failed: no sensor data");
            return ESP_ERR_INVALID_STATE;
        }
        ESP_LOGI(TAG, "voice reply: 当前温度 %.1f C", d->temp);
        return ESP_OK;
    }
    case VOICE_CMD_QUERY_HUMIDITY: {
        const sensor_data_t *d = uart_receiver_get_latest_data();
        if (d == NULL) {
            ESP_LOGW(TAG, "voice humidity query failed: no sensor data");
            return ESP_ERR_INVALID_STATE;
        }
        ESP_LOGI(TAG, "voice reply: 当前湿度 %.1f %%", d->humidity);
        return ESP_OK;
    }
    case VOICE_CMD_WATER_START:
        ESP_LOGI(TAG, "voice reply: 开始浇水");
        return stm32_protocol_send_control_command("water_control", "start", 10, false, true, NULL);
    case VOICE_CMD_WATER_STOP:
        ESP_LOGI(TAG, "voice reply: 停止浇水");
        return stm32_protocol_send_control_command("water_control", "stop", 0, false, true, NULL);
    case VOICE_CMD_MODE_AUTO:
        ESP_LOGI(TAG, "voice reply: 切换自动模式");
        return stm32_protocol_send_mode_command("auto", "voice_command", true, true, NULL);
    case VOICE_CMD_MODE_MANUAL:
        ESP_LOGI(TAG, "voice reply: 切换手动模式");
        return stm32_protocol_send_mode_command("manual", "voice_command", true, true, NULL);
    case VOICE_CMD_NONE:
    default:
        return ESP_ERR_NOT_FOUND;
    }
}

static void enter_cooldown(bool with_error_tone)
{
    set_state(VOICE_STATE_COOLDOWN);
    s_cooldown_until = xTaskGetTickCount() + pdMS_TO_TICKS(VOICE_COOLDOWN_MS);
    if (with_error_tone) {
        play_error_tone();
    }
}

static void handle_voice_cmd(voice_cmd_t cmd, const char *source)
{
    if (cmd == VOICE_CMD_NONE) {
        s_cmd_fail_count++;
        ESP_LOGW(TAG, "voice command not recognized (source=%s)", source ? source : "unknown");
        enter_cooldown(true);
        return;
    }

    set_state(VOICE_STATE_EXECUTE);
    esp_err_t ret = execute_command(cmd);

    set_state(VOICE_STATE_SPEAK);
    if (ret == ESP_OK) {
        s_cmd_ok_count++;
        play_ok_tone_by_cmd(cmd);
    } else {
        s_cmd_fail_count++;
        ESP_LOGW(TAG, "voice command execute failed: %s", esp_err_to_name(ret));
        play_error_tone();
    }

    enter_cooldown(false);
}

static void handle_command_text(const char *text)
{
    voice_cmd_t cmd = parse_command_text(text);
    if (cmd == VOICE_CMD_NONE) {
        ESP_LOGW(TAG, "voice text not matched: \"%s\"", text ? text : "");
    }
    handle_voice_cmd(cmd, "text");
}

static void on_wake_word(const char *word)
{
    if (s_evt_queue == NULL) {
        return;
    }

    voice_evt_t evt = {
        .type = VOICE_EVT_WAKE_WORD,
    };

    if (word != NULL) {
        strncpy(evt.text, word, sizeof(evt.text) - 1);
    }

    (void)xQueueSend(s_evt_queue, &evt, 0);
}

#if VOICE_SR_CMD_AVAILABLE
static void post_mn_command_event(int command_id)
{
    if (s_evt_queue == NULL) {
        return;
    }

    voice_evt_t evt = {
        .type = VOICE_EVT_MN_CMD,
        .command_id = command_id,
    };

    if (xQueueSend(s_evt_queue, &evt, 0) != pdTRUE) {
        ESP_LOGW(TAG, "drop mn command event, queue full");
    }
}

static esp_err_t add_one_mn_command(int command_id, const char *command)
{
    esp_err_t ret = esp_mn_commands_add(command_id, command);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "skip invalid mn command: id=%d text=%s", command_id, command);
    }
    return ret;
}

static esp_err_t init_multinet_commands(void)
{
    srmodel_list_t *models = esp_srmodel_init("model");
    if (models == NULL) {
        ESP_LOGW(TAG, "esp_srmodel_init failed for multinet");
        return ESP_FAIL;
    }

    char *mn_name = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_CHINESE);
    if (mn_name == NULL) {
        mn_name = esp_srmodel_filter(models, ESP_MN_PREFIX, NULL);
    }

    if (mn_name == NULL) {
        ESP_LOGW(TAG, "no multinet model found");
        esp_srmodel_deinit(models);
        return ESP_ERR_NOT_FOUND;
    }

    strncpy(s_mn_model, mn_name, sizeof(s_mn_model) - 1);

    s_mn = esp_mn_handle_from_name(s_mn_model);
    if (s_mn == NULL) {
        ESP_LOGE(TAG, "esp_mn_handle_from_name failed: %s", s_mn_model);
        esp_srmodel_deinit(models);
        return ESP_FAIL;
    }

    s_mn_data = s_mn->create(s_mn_model, 6000);
    if (s_mn_data == NULL) {
        ESP_LOGE(TAG, "multinet create failed: %s", s_mn_model);
        esp_srmodel_deinit(models);
        return ESP_FAIL;
    }

    esp_srmodel_deinit(models);

    s_mn_feed_samples = (size_t)s_mn->get_samp_chunksize(s_mn_data);
    ESP_LOGI(TAG, "multinet model=%s feed_samples=%u", s_mn_model, (unsigned)s_mn_feed_samples);

    if (esp_mn_commands_alloc(s_mn, s_mn_data) != ESP_OK) {
        ESP_LOGE(TAG, "esp_mn_commands_alloc failed");
        s_mn->destroy(s_mn_data);
        s_mn_data = NULL;
        s_mn = NULL;
        return ESP_FAIL;
    }
    s_mn_commands_allocated = true;

    int accepted = 0;
    accepted += (add_one_mn_command(VOICE_MN_ID_QUERY_TEMP, "查询温度") == ESP_OK);
    accepted += (add_one_mn_command(VOICE_MN_ID_QUERY_TEMP, "现在温度") == ESP_OK);
    accepted += (add_one_mn_command(VOICE_MN_ID_QUERY_HUMIDITY, "查询湿度") == ESP_OK);
    accepted += (add_one_mn_command(VOICE_MN_ID_QUERY_HUMIDITY, "现在湿度") == ESP_OK);
    accepted += (add_one_mn_command(VOICE_MN_ID_WATER_START, "开始浇水") == ESP_OK);
    accepted += (add_one_mn_command(VOICE_MN_ID_WATER_START, "启动浇水") == ESP_OK);
    accepted += (add_one_mn_command(VOICE_MN_ID_WATER_STOP, "停止浇水") == ESP_OK);
    accepted += (add_one_mn_command(VOICE_MN_ID_MODE_AUTO, "自动模式") == ESP_OK);
    accepted += (add_one_mn_command(VOICE_MN_ID_MODE_MANUAL, "手动模式") == ESP_OK);
    if (accepted == 0) {
        ESP_LOGW(TAG, "multinet command phrases are not accepted by current model, disable mn");
        (void)esp_mn_commands_free();
        s_mn_commands_allocated = false;
        s_mn->destroy(s_mn_data);
        s_mn_data = NULL;
        s_mn = NULL;
        s_mn_ready = false;
        s_mn_feed_samples = 0;
        s_mn_model[0] = '\0';
        return ESP_ERR_NOT_SUPPORTED;
    }

    esp_mn_error_t *err = esp_mn_commands_update();
    if (err != NULL) {
        ESP_LOGW(TAG, "multinet command update has %d unparsed phrases", err->num);
    }

    ESP_LOGI(TAG, "multinet command set ready: accepted=%d", accepted);
    s_mn_ready = true;
    return ESP_OK;
}

static void deinit_multinet_commands(void)
{
    if (s_mn != NULL && s_mn_data != NULL) {
        s_mn->clean(s_mn_data);
    }

    if (s_mn_commands_allocated) {
        (void)esp_mn_commands_free();
        s_mn_commands_allocated = false;
    }

    if (s_mn != NULL && s_mn_data != NULL) {
        s_mn->destroy(s_mn_data);
    }

    s_mn_data = NULL;
    s_mn = NULL;
    s_mn_ready = false;
    s_mn_feed_samples = 0;
    s_mn_model[0] = '\0';
}

static void process_multinet_detection(const int16_t *samples, size_t sample_count)
{
    if (!s_mn_ready || s_mn == NULL || s_mn_data == NULL) {
        return;
    }

    if (s_state != VOICE_STATE_AWAKE_LISTEN) {
        return;
    }

    if (sample_count != s_mn_feed_samples) {
        return;
    }

    esp_mn_state_t state = s_mn->detect(s_mn_data, (int16_t *)samples);
    if (state == ESP_MN_STATE_DETECTED) {
        esp_mn_results_t *results = s_mn->get_results(s_mn_data);
        if (results != NULL && results->num > 0) {
            int command_id = results->command_id[0];
            s_mn_detect_count++;
            ESP_LOGI(TAG,
                     "multinet detected command_id=%d phrase_id=%d prob=%.3f",
                     command_id,
                     results->phrase_id[0],
                     results->prob[0]);
            post_mn_command_event(command_id);
        }
        s_mn->clean(s_mn_data);
    } else if (state == ESP_MN_STATE_TIMEOUT) {
        s_mn->clean(s_mn_data);
    }
}
#endif

static void voice_state_task(void *arg)
{
    (void)arg;
    voice_evt_t evt = {0};

    while (s_running) {
        if (xQueueReceive(s_evt_queue, &evt, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (evt.type == VOICE_EVT_WAKE_WORD) {
                s_wake_count++;
                if (s_state == VOICE_STATE_IDLE) {
                    set_state(VOICE_STATE_AWAKE_LISTEN);
                    s_awake_until = xTaskGetTickCount() + pdMS_TO_TICKS(VOICE_AWAKE_WINDOW_MS);
                    ESP_LOGI(TAG, "wake detected: %s", evt.text);
                    play_ready_tone();
                } else if (s_state == VOICE_STATE_AWAKE_LISTEN) {
                    s_awake_until = xTaskGetTickCount() + pdMS_TO_TICKS(VOICE_AWAKE_WINDOW_MS);
                    ESP_LOGI(TAG, "wake detected again, extend command window");
                }
            } else if (evt.type == VOICE_EVT_TEXT_CMD) {
                if (s_state == VOICE_STATE_AWAKE_LISTEN) {
                    handle_command_text(evt.text);
                } else if (s_wake == NULL) {
                    ESP_LOGI(TAG, "wake detector unavailable, execute text command directly");
                    handle_command_text(evt.text);
                } else {
                    ESP_LOGI(TAG, "ignore text command when not awake: \"%s\"", evt.text);
                }
            } else if (evt.type == VOICE_EVT_MN_CMD) {
                if (s_state == VOICE_STATE_AWAKE_LISTEN) {
                    voice_cmd_t cmd = map_mn_command_id(evt.command_id);
                    handle_voice_cmd(cmd, "multinet");
                } else {
                    ESP_LOGI(TAG, "ignore mn command when not awake: id=%d", evt.command_id);
                }
            }
        }

        TickType_t now = xTaskGetTickCount();
        if (s_state == VOICE_STATE_AWAKE_LISTEN && now > s_awake_until) {
            ESP_LOGI(TAG, "voice command window timeout");
            play_timeout_tone();
            enter_cooldown(false);
        } else if (s_state == VOICE_STATE_COOLDOWN && now > s_cooldown_until) {
            set_state(VOICE_STATE_IDLE);
        }
    }

    s_voice_task = NULL;
    vTaskDelete(NULL);
}

static void mic_feed_task(void *arg)
{
    (void)arg;

    if (s_wake == NULL || !s_mic_ready) {
        ESP_LOGW(TAG, "wake detector unavailable or mic not ready, mic feed task exits");
        s_mic_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    if (s_feed_samples == 0) {
        s_feed_samples = VOICE_DEFAULT_FEED_SAMPLES;
    }

    size_t bytes_target = s_feed_samples * sizeof(int16_t);
    int16_t *samples = calloc(s_feed_samples, sizeof(int16_t));
    if (samples == NULL) {
        ESP_LOGE(TAG, "alloc mic feed buffer failed");
        s_mic_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "mic feed task start, feed_samples=%u", (unsigned)s_feed_samples);
    while (s_running) {
        size_t bytes_read = 0;
        esp_err_t ret = i2s_read(VOICE_I2S_PORT, samples, bytes_target, &bytes_read, pdMS_TO_TICKS(100));
        if (ret != ESP_OK || bytes_read != bytes_target) {
            continue;
        }

        (void)wake_detector_feed(s_wake, samples);

#if VOICE_SR_CMD_AVAILABLE
        process_multinet_detection(samples, s_feed_samples);
#endif
    }

    free(samples);
    s_mic_task = NULL;
    vTaskDelete(NULL);
}

static esp_err_t mic_i2s_init(void)
{
    i2s_config_t i2s_cfg = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,
        .sample_rate = AUDIO_INPUT_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = false,
        .intr_alloc_flags = 0,
    };

    i2s_pin_config_t pin_cfg = {
        .bck_io_num = AUDIO_I2S_MIC_GPIO_SCK,
        .ws_io_num = AUDIO_I2S_MIC_GPIO_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = AUDIO_I2S_MIC_GPIO_DIN,
    };

    esp_err_t ret = i2s_driver_install(VOICE_I2S_PORT, &i2s_cfg, 0, NULL);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "i2s_driver_install(RX) failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2s_set_pin(VOICE_I2S_PORT, &pin_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2s_set_pin(RX) failed: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

esp_err_t voice_manager_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    s_evt_queue = xQueueCreate(VOICE_QUEUE_LEN, sizeof(voice_evt_t));
    if (s_evt_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = mic_i2s_init();
    s_mic_ready = (ret == ESP_OK);
    if (!s_mic_ready) {
        ESP_LOGW(TAG, "mic_i2s_init failed, voice input disabled: %s", esp_err_to_name(ret));
    }

    // Some ESP-SR models are unstable on no-PSRAM boards; skip wake init to avoid boot panic.
#if CONFIG_SPIRAM
    s_sr_runtime_allowed = esp_psram_is_initialized();
#else
    s_sr_runtime_allowed = false;
#endif
    if (!s_sr_runtime_allowed) {
        ESP_LOGW(TAG, "PSRAM not initialized, disable wake detector to keep system boot stable");
    }

    s_wake = s_sr_runtime_allowed ? wake_detector_create(1, false, NULL) : NULL;
    if (s_wake == NULL) {
        ESP_LOGW(TAG, "wake_detector_create failed, only text command injection available");
    } else {
        wake_detector_set_callback(s_wake, on_wake_word);
        s_feed_samples = wake_detector_get_feed_samples(s_wake);
        if (s_feed_samples == 0) {
            s_feed_samples = VOICE_DEFAULT_FEED_SAMPLES;
        }

        const char *model = wake_detector_get_model_name(s_wake);
        const char *words = wake_detector_get_wake_words(s_wake);
        if (model != NULL) {
            strncpy(s_wake_model, model, sizeof(s_wake_model) - 1);
        }
        if (words != NULL) {
            strncpy(s_wake_words, words, sizeof(s_wake_words) - 1);
        }
        ESP_LOGI(TAG, "wake model=%s words=%s", s_wake_model, s_wake_words);
    }

#if VOICE_SR_CMD_AVAILABLE
    if (s_mic_ready && s_wake != NULL) {
        ret = init_multinet_commands();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "multinet init failed: %s", esp_err_to_name(ret));
        } else if (s_mn_feed_samples > 0 && s_mn_feed_samples != s_feed_samples) {
            ESP_LOGW(TAG,
                     "multinet feed mismatch: mn=%u wake=%u, disable mn for safety",
                     (unsigned)s_mn_feed_samples,
                     (unsigned)s_feed_samples);
            deinit_multinet_commands();
        }
    }
#else
    ESP_LOGW(TAG, "multinet headers unavailable at compile-time");
#endif

    s_initialized = true;
    set_state(VOICE_STATE_IDLE);
    return ESP_OK;
}

esp_err_t voice_manager_start(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_running) {
        return ESP_OK;
    }

    s_running = true;
    if (s_wake != NULL) {
        wake_detector_start(s_wake);
    }

    BaseType_t ok = xTaskCreate(voice_state_task, "voice_state", VOICE_STATE_TASK_STACK, NULL, 6, &s_voice_task);
    if (ok != pdPASS) {
        s_running = false;
        return ESP_ERR_NO_MEM;
    }

    if (s_wake != NULL && s_mic_ready) {
        ok = xTaskCreate(mic_feed_task, "voice_mic", VOICE_MIC_TASK_STACK, NULL, 6, &s_mic_task);
        if (ok != pdPASS) {
            s_running = false;
            wake_detector_stop(s_wake);
            if (s_voice_task != NULL) {
                vTaskDelete(s_voice_task);
                s_voice_task = NULL;
            }
            return ESP_ERR_NO_MEM;
        }
    } else {
        ESP_LOGW(TAG, "mic feed task skipped: wake=%d mic_ready=%d", s_wake != NULL, s_mic_ready);
    }

    ESP_LOGI(TAG, "voice manager started");
    return ESP_OK;
}

esp_err_t voice_manager_stop(void)
{
    if (!s_running) {
        return ESP_OK;
    }

    s_running = false;
    if (s_wake != NULL) {
        wake_detector_stop(s_wake);
    }

#if VOICE_SR_CMD_AVAILABLE
    if (s_mn != NULL && s_mn_data != NULL) {
        s_mn->clean(s_mn_data);
    }
#endif

    set_state(VOICE_STATE_IDLE);
    return ESP_OK;
}

esp_err_t voice_manager_submit_text_command(const char *text)
{
    if (s_evt_queue == NULL || text == NULL || text[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    voice_evt_t evt = {
        .type = VOICE_EVT_TEXT_CMD,
    };
    strncpy(evt.text, text, sizeof(evt.text) - 1);

    if (xQueueSend(s_evt_queue, &evt, 0) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

void voice_manager_dump_status(void)
{
    ESP_LOGI(TAG,
             "init=%d running=%d sr_allowed=%d wake=%d mic=%d mn=%d state=%d wake_count=%" PRIu32 " mn_detect=%" PRIu32
             " cmd_ok=%" PRIu32 " cmd_fail=%" PRIu32 " feed=%u wake_model=%s mn_model=%s",
             s_initialized,
             s_running,
             s_sr_runtime_allowed,
             s_wake != NULL,
             s_mic_ready,
             s_mn_ready,
             (int)s_state,
             s_wake_count,
             s_mn_detect_count,
             s_cmd_ok_count,
             s_cmd_fail_count,
             (unsigned)s_feed_samples,
             s_wake_model,
             s_mn_model);
}
