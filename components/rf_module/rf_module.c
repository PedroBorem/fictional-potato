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


static uint16_t rf_angle = 0;
static time_t rf_timestamp = 0;


void rf_module_call(char* buffer, size_t buffer_size);

esp_err_t rf_module_init(void)
{
	esp_err_t err = ESP_FAIL;

	rf_uart_init(rf_module_call);

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
	return rf_angle;
}

time_t rf_module_get_timestamp(void)
{
	return rf_timestamp;
}

/// callback;
void rf_module_call(char* buffer, size_t buffer_size)
{
	esp_err_t err = ESP_FAIL;

	pivot_config config = {};
	time_t timestamp = 0;
	uint16_t angle = 0;

	//GPS179.31-17:51:12#1655920272$‹
	char search[] = "GPS";
	char* ptr = strstr(buffer, search);

	if(ptr != NULL)
	{
		err = common_parser_string_to_gnss(ptr, &angle, &timestamp);
		if(err == ESP_OK)
		{
			rf_angle = angle;
			rf_timestamp = timestamp;
		}
	}
	else
	{
		common_parser_string_to_config(buffer, &config);
		//joga pra cima para comm_app
	}
}




















