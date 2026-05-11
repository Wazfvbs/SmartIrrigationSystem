#include "ui.h"

#include <stddef.h>

#include "../uart_receiver/uart_receiver.h"
#include "default_ui.h"
#include "menu_ui.h"

const ui_module_t *current_ui = NULL;

void ui_set_mode(const ui_module_t *ui)
{
    if (ui == NULL || ui->init == NULL)
    {
        return;
    }
    current_ui = ui;
    current_ui->init();
}

static bool event_is_menu_enter(button_event_t event)
{
    return event == BUTTON_SHORT_PRESS || event == BUTTON_MIDDLE_SHORT_PRESS;
}

void ui_handle_button_event(button_event_t event)
{
    if (current_ui == &ui_menu)
    {
        ui_menu_on_button_event(event);
        return;
    }

    if (current_ui == &ui_default && event_is_menu_enter(event))
    {
        ui_menu_prepare_entry(uart_receiver_get_latest_data());
        ui_set_mode(&ui_menu);
    }
}
