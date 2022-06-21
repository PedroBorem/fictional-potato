/*
 * rf_uart.h
 *
 *  Created on: 21 de jun. de 2022
 *      Author: brunolima
 */

#ifndef COMPONENTS_RF_MODULE_INCLUDE_RF_UART_H_
#define COMPONENTS_RF_MODULE_INCLUDE_RF_UART_H_


#include "project_config.h"


esp_err_t rf_uart_init(void);

esp_err_t rf_uart_send_event(char* event, size_t event_size);

#endif /* COMPONENTS_RF_MODULE_INCLUDE_RF_UART_H_ */
