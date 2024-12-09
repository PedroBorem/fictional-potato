#ifndef PLUVIOMETER_UART_H
#define PLUVIOMETER_UART_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

/**
 * @brief Initializes the UART for pluviometer communication.
 * 
 * @return esp_err_t ESP_OK on success, ESP_FAIL otherwise.
 */
esp_err_t pluviometer_uart_init(void);

/**
 * @brief Reads data from the pluviometer via UART.
 * 
 * @param data Pointer to buffer to store received data.
 * @param length Maximum length of data to read.
 * @return int Number of bytes read.
 */
int pluviometer_uart_read(uint8_t *data, size_t length);

#endif // PLUVIOMETER_UART_H
