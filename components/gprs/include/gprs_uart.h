/**
 * @file gprs_uart.h
 * @brief Header file for GPRS UART communication functions.
 */

#ifndef COMPONENTS_GPRS_INCLUDE_GPRS_UART_H_
#define COMPONENTS_GPRS_INCLUDE_GPRS_UART_H_

#include "project_config.h"
#include "esp_err.h"

/**
 * @brief Initialize the GPRS UART module.
 *
 * This function initializes the GPRS UART module, setting up the UART communication for GPRS.
 *
 * @param callback The callback function to be called when data is received.
 *
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Fail to initialize
 *     - ESP_ERR_INVALID_ARG: Invalid callback function
 */
esp_err_t gprs_uart_init(const app_callback callback);

/**
 * @brief Send an event through the GPRS UART module.
 *
 * This function sends an event buffer through the GPRS UART module.
 *
 * @param event The event buffer to be sent.
 * @param event_size The size of the event buffer.
 *
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Fail to send the event
 */
esp_err_t gprs_uart_send_event(const char* event, size_t event_size);

#endif /* COMPONENTS_GPRS_INCLUDE_GPRS_UART_H_ */
