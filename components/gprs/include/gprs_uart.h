/**
 * @file gprs_uart.h
 * @date 7 de agosto de 2022
 * @brief Header file for GPRS UART communication functions.
 */

#ifndef COMPONENTS_GPRS_INCLUDE_GPRS_UART_H_
#define COMPONENTS_GPRS_INCLUDE_GPRS_UART_H_

#include "project_config.h"
#include "esp_err.h"

/**
 * @brief Callback function used for receiving data from the GPRS UART module.
 * @param buffer The received data buffer
 * @param buffer_size The size of the received data buffer
 */
typedef void (*gprs_uart_callback)(const char* buffer, size_t buffer_size);

/**
 * @brief Initialize the GPRS UART module.
 * @param callback The callback function to be called when data is received
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Fail to initialize
 *     - ESP_ERR_INVALID_ARG: Invalid callback function
 */
esp_err_t gprs_uart_init(const gprs_uart_callback callback);

/**
 * @brief Send an event through the GPRS UART module.
 * @param event The event buffer to be sent
 * @param event_size The size of the event buffer
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Fail to send the event
 */
esp_err_t gprs_uart_send_event(const char* event, size_t event_size);

#endif /* COMPONENTS_GPRS_INCLUDE_GPRS_UART_H_ */
