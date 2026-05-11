#ifndef UI_H
#define UI_H

#include "../uart_receiver/uart_receiver.h"

typedef struct
{
    void (*init)(void);
    void (*update)(const sensor_data_t *data);
} ui_module_t;

extern const ui_module_t ui_default;
extern const ui_module_t ui_menu;

extern const ui_module_t *current_ui;

void ui_set_mode(const ui_module_t *ui);
void ui_handle_button_event(button_event_t event);

#endif // UI_H
