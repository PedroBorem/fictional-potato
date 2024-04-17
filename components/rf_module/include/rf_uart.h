/**
 * @file rf_uart.h
 * @date June 21, 2022
 * @brief RF UART module functions and callbacks.
 *
 * This file declares functions for initializing and interacting with the RF UART module.
 */

#ifndef COMPONENTS_RF_MODULE_INCLUDE_RF_UART_H_
#define COMPONENTS_RF_MODULE_INCLUDE_RF_UART_H_

/* Configuration base */
#include "project_config.h"

/* include ESP32 modules */
#include "esp_err.h"

/**
 * @brief Initialize the RF UART module.
 *
 * This function initializes the RF UART module and sets the callback function for handling received events.
 *
 * @param callback Function pointer to the module application class.
 * @return
 * 	- ESP_OK: Success
 * 	- ESP_FAIL: Fail
 * 	- ESP_ERR_INVALID_ARG: Invalid callback
 */
esp_err_t rf_uart_init(const app_callback callback);

/**
 * @brief Send events through the UART.
 *
 * This function sends events through the RF UART module.
 *
 * @param event Buffer containing the event data.
 * @param event_size Size of the event buffer.
 * @return
 * 	- ESP_OK: Success
 * 	- ESP_FAIL: Fail
 */
esp_err_t rf_uart_send_event(const char* event, size_t event_size);

#endif /* COMPONENTS_RF_MODULE_INCLUDE_RF_UART_H_ */
