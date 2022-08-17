/*
 * gprs_uart.h
 *
 *  Created on: 7 de ago de 2022
 *      Author: bruno
 */

#ifndef COMPONENTS_GPRS_INCLUDE_GPRS_UART_H_
#define COMPONENTS_GPRS_INCLUDE_GPRS_UART_H_


/**
 * @file common_parser.h
 * @date June 21, 2022
 * @brief gprs_uart uart control
*/

#include "project_config.h"

/**
 * @brief: function used with return to rf_module class
 *
 */
typedef void (*gprs_uart_callback)(const char* buffer, size_t buffer_size);

/**
 * @brief	start the gprs_uart UART
 * @param 	callback[in]: function pointer to module application class
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail
 * 	- ESP_ERR_INVALID_ARG: invalid callback
 */
esp_err_t gprs_uart_init(const gprs_uart_callback callback);

/**
 * @brief	send events in the UART
 * @param 	event[in]: buffer sent
 * @param 	event_size[in]: buffer size
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail
 */
esp_err_t gprs_uart_send_event(const char* event, size_t event_size);


#endif /* COMPONENTS_GPRS_INCLUDE_GPRS_UART_H_ */
