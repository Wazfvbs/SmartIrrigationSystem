#ifndef STM32_PROTOCOL_H
#define STM32_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"
#include "cJSON.h"

#define STM32_TRACE_ID_MAX_LEN 32
#define STM32_PROTO_TIME_LEN 20

esp_err_t stm32_protocol_build_trace_id(char *out, size_t out_len);
esp_err_t stm32_protocol_get_timestamp(char *out, size_t out_len);

esp_err_t stm32_protocol_send(const char *msg_type,
                              bool require_ack,
                              const cJSON *payload,
                              const char *trace_id);

esp_err_t stm32_protocol_send_control_command(const char *cmd,
                                              const char *action,
                                              int duration_sec,
                                              bool force,
                                              bool require_ack,
                                              const char *trace_id);

esp_err_t stm32_protocol_send_mode_command(const char *mode,
                                           const char *reason,
                                           bool force,
                                           bool require_ack,
                                           const char *trace_id);

esp_err_t stm32_protocol_send_config_command(const cJSON *config_payload,
                                             bool require_ack,
                                             const char *trace_id);
esp_err_t stm32_protocol_send_threshold_config(double lower,
                                               double upper,
                                               bool require_ack,
                                               const char *trace_id);

esp_err_t stm32_protocol_send_sync_info(const char *sync_time,
                                        const char *config_version,
                                        const char *strategy_version,
                                        const char *plant_name,
                                        const char *species);

esp_err_t stm32_protocol_send_command_context(const char *command_source,
                                              const char *priority,
                                              bool allow_override,
                                              bool require_ack,
                                              const char *operator_id,
                                              const char *remark,
                                              const char *trace_id);

esp_err_t stm32_protocol_send_menu_command(const char *menu_action,
                                           const char *menu_page,
                                           int main_index,
                                           int sub_index,
                                           bool threshold_editing,
                                           bool require_ack,
                                           const char *trace_id);

#endif // STM32_PROTOCOL_H
