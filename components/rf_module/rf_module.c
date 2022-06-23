/*
 * rf_module.c
 *
 *  Created on: 21 de jun. de 2022
 *      Author: brunolima
 */

/**
 * @file rf_module.c
 * @date June 21, 2022
 * @brief controls the flow of reception and transmission of the RF module
*/

/* Self include */
#include "rf_module.h"

/* Components include */
#include "rf_uart.h"
#include "common_parser.h"

/**\addtogroup components
 * @{
 *
 */

/**\addtogroup rf_uart
 * @{
 *
 */

esp_err_t rf_module_init(void)
{
	esp_err_t err = ESP_FAIL;

	rf_uart_init();

	return err;
}

esp_err_t rf_module_send_event(pivot_config config_in)
{
	esp_err_t err = ESP_FAIL;
	char event[100] = "";


	common_parser_status_to_string(	config_in,
									rf_module_get_angle(),
									rf_module_get_timestamp(),
									event);

	rf_uart_send_event(event, sizeof(event));

	return err;
}


uint16_t rf_module_get_angle(void)
{
	uint16_t angle = 180;

	return angle;
}

time_t rf_module_get_timestamp(void)
{
	time_t timestamp = 100000;

	return timestamp;
}


/// callback;
void rf_module_call(char* buffer, size_t buffer_size)
{
	//converte oque chegou e manda para cima.
}




















