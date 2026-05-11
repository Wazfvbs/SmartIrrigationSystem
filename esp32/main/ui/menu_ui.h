#ifndef MENU_UI_H
#define MENU_UI_H

#include "ui.h"

extern const ui_module_t ui_menu;

void ui_menu_prepare_entry(const sensor_data_t *latest_data);
void ui_menu_on_button_event(button_event_t event);

#endif // MENU_UI_H
