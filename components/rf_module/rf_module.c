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
#include "rtc_app.h"

/**\addtogroup components
 * @{
 *
 */

/**\addtogroup rf_module
 * @{
 *
 */
/* Private definitions-------------------------------------------- */
#define	RF_MODULE_TAG	"rf_module"

/* Private variables  -------------------------------------------- */
static uint16_t rf_angle = 0xFFFF;

/* Private methods  ---------------------------------------------- */
void rf_module_call(const char* buffer, size_t buffer_size);

/* Public methods ------------------------------------------------ */
esp_err_t rf_module_init(void)
{
	esp_err_t err = ESP_FAIL;

	err = rf_uart_init(rf_module_call);

	return err;
}

esp_err_t rf_module_send_event(pivot_actions config_in)
{
	esp_err_t err = ESP_FAIL;
	uint16_t degree = rf_module_get_angle();
	time_t timestamp = rtc_app_get_timestamp(true);

	char event[25] = "";
	common_parser_status_to_string(config_in, timestamp, degree, event);

	err = rf_uart_send_event(event, sizeof(event));

	return err;
}

uint16_t rf_module_get_angle(void)
{
	return rf_angle;
}

/* Private methods ----------------------------------------------- */
/**
 * @brief 	Function is triggered when something arrives in the UART
 * @param	buffer[in] : received content
 * @param	buffer_size[in] : received content size
 */
void rf_module_call(const char* buffer, size_t buffer_size)
{
	esp_err_t err = ESP_OK;

	// Configuration Pivot
	//pivot_actions config = {};

	// GPS variables
	time_t timestamp = 0;
	uint16_t angle = 0;

	const char search[] = "GPS";
	char* ptr = strstr(buffer, search);

	if(ptr != NULL)
	{
		// if the received message is GPS data
		err = common_parser_string_to_gnss(ptr, &angle, &timestamp);
		if(err == ESP_OK)
		{
			rf_angle = angle;
			rtc_app_set_timestamp(timestamp);

			LOG_COMM(RF_MODULE_TAG, "angle : %d", rf_angle);
			LOG_COMM(RF_MODULE_TAG, "timestamp : %lld\n", (long long int)timestamp);
		}
		else
		{
			ESP_LOGE(RF_MODULE_TAG, "%s, GPS invalid format", __func__);
		}
	}
	else
	{
		RF_MODULE_NOTIFY_APP(buffer);
/*
		idp_type idp = common_parser_get_idp(buffer);
		if(idp == IDP_INVALID)
		{
			// if the received message is a configuration
			err = common_parser_string_to_config(buffer, &config);
			if(err == ESP_OK)
			{
				RF_MODULE_NOTIFY_APP(config);
			}
			else
			{
				ESP_LOGE(RF_MODULE_TAG, "%s, invalid configuration", __func__);
			}
		}
		*/
	}
}

/* Public callback ----------------------------------------------- */

/*
 * This function must be implemented in the application of communication.
 */
__attribute__((weak)) void RF_MODULE_NOTIFY_APP(const void* notify_buffer)
{
	UNUSED(notify_buffer);
	return;
}

/**@}*/ 	//rf_module
/** @}*/	//components
