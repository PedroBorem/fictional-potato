/*
 * rf_uart.h
 *
 *  Created on: 21 de jun. de 2022
 *      Author: brunolima
 */

#ifndef COMPONENTS_RF_MODULE_INCLUDE_RF_UART_H_
#define COMPONENTS_RF_MODULE_INCLUDE_RF_UART_H_

/**
 * @file common_parser.h
 * @date June 21, 2022
 * @brief RF module uart control
*/

#include "project_config.h"

/**
 * @brief: function used with return to rf_module class
 *
 */
typedef void (*rf_uart_callback)(char* buffer, size_t buffer_size);

/**
 * @brief	start the RF UART
 * @param 	callback[in]: function pointer to module application class
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail
 * 	- ESP_ERR_INVALID_ARG: invalid callback
 */
esp_err_t rf_uart_init(rf_uart_callback callback);

/**
 * @brief	send events in the UART
 * @param 	event[in]: buffer sent
 * @param 	event_size[in]: buffer size
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail
 */
esp_err_t rf_uart_send_event(const char* event, size_t event_size);

#endif /* COMPONENTS_RF_MODULE_INCLUDE_RF_UART_H_ */
