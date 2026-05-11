#ifndef VOICE_MANAGER_H
#define VOICE_MANAGER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    esp_err_t voice_manager_init(void);
    esp_err_t voice_manager_start(void);
    esp_err_t voice_manager_stop(void);
    esp_err_t voice_manager_submit_text_command(const char *text);
    void voice_manager_dump_status(void);

#ifdef __cplusplus
}
#endif

#endif // VOICE_MANAGER_H

