// display_manager.h

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "esp_err.h"
#include "uart_receiver.h" // for sensor_data_t

esp_err_t display_manager_init(void);
esp_err_t display_manager_show_env(const sensor_data_t *data);
esp_err_t display_manager_show_message(const char *message);
esp_err_t display_manager_show_watering(void);

#endif // DISPLAY_MANAGER_H

// display_manager.h
#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "esp_err.h"
#include "uart_receiver.h"

esp_err_t display_manager_init(void);
esp_err_t display_manager_show_env(const sensor_data_t *data);
esp_err_t display_manager_show_message(const char *message);
esp_err_t display_manager_show_watering(void);

#endif // DISPLAY_MANAGER_H