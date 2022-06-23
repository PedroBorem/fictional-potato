/*
 * rf_uart.h
 *
 *  Created on: 21 de jun. de 2022
 *      Author: brunolima
 */

#ifndef COMPONENTS_RF_MODULE_INCLUDE_RF_UART_H_
#define COMPONENTS_RF_MODULE_INCLUDE_RF_UART_H_


#include "project_config.h"


/**
 * @brief:
 *
 */
typedef void (*rf_uart_callback)(char* buffer, size_t buffer_size);


esp_err_t rf_uart_init(rf_uart_callback callback);

esp_err_t rf_uart_send_event(char* event, size_t event_size);

#endif /* COMPONENTS_RF_MODULE_INCLUDE_RF_UART_H_ */
