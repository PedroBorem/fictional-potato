/*
 * gprs_module.c
 *
 *  Created on: 7 de ago de 2022
 *      Author: bruno
 */
/**
 * @file gprs_module.c
 * @date June 21, 2022
 * @brief controls the flow of reception and transmission of the gprs module
*/

/* Self include */
#include "gprs_module.h"

/* Components include */
#include "gprs_uart.h"
#include "common_parser.h"
#include "rtc_app.h"

/**\addtogroup components
 * @{
 *
 */

/**\addtogroup gprs_module
 * @{
 *
 */
/* Private definitions-------------------------------------------- */
#define	GPRS_MODULE_TAG	"gprs_module"

/* Private variables  -------------------------------------------- */

/* Private methods  ---------------------------------------------- */
void gprs_module_call(const char* buffer, size_t buffer_size);

/* Public methods ------------------------------------------------ */
esp_err_t gprs_module_init(void)
{
	esp_err_t err = ESP_FAIL;

	err = gprs_uart_init(gprs_module_call);

	return err;
}

esp_err_t gprs_module_send_event(pivot_actions config_in, uint16_t degree, const char * gprs_id)
{
	esp_err_t err = ESP_FAIL;
	time_t timestamp = rtc_app_get_timestamp();

	char event[90] = "";
	common_parser_status_to_json(config_in, timestamp, degree, gprs_id , event, sizeof(event));

	err = gprs_uart_send_event(event, sizeof(event));
	return err;
}

esp_err_t gprs_module_set_id(const char * gprs_id)
{
	esp_err_t err = ESP_FAIL;

	char send_gprs_id[70] = {};

	common_parser_register_to_json(gprs_id, send_gprs_id, sizeof(send_gprs_id));
	err = gprs_uart_send_event(send_gprs_id, sizeof(send_gprs_id));

	return err;
}

/* Private methods ----------------------------------------------- */
/**
 * @brief 	Function is triggered when something arrives in the UART
 * @param	buffer[in] : received content
 * @param	buffer_size[in] : received content size
 */
void gprs_module_call(const char* buffer, size_t buffer_size)
{
	esp_err_t err = ESP_OK;

	// Configuration Pivot
	pivot_actions config = {};

	// GPS variables
	time_t timestamp = 0;

	const char search_payload[] = "{\"payload\":\""; //TODO: OLHAR AQUI
	const char search_timestamp[] = "\"timestamp\":"; //TODO: OLHAR AQUI
	char* ptr_payload = strstr(buffer, search_payload);

	if(ptr_payload != NULL)
	{
		// if the received message is GPS data
		err = common_parser_json_to_config(&ptr_payload[12], &config);
		if(err == ESP_OK)
		{
			char* ptr_timestamp = strstr(buffer, search_timestamp);
			if(ptr_timestamp != NULL)
			{
				err = common_parser_json_to_timestamp(&ptr_timestamp[12], &timestamp);
				if(err == ESP_OK)
				{
					rtc_app_set_timestamp(timestamp);
					LOG_COMM(GPRS_MODULE_TAG, "timestamp : %lld\n", (long long int)timestamp);
					GPRS_MODULE_NOTIFY_APP(config);
				}
			}
			else
			{
				ESP_LOGE(GPRS_MODULE_TAG, "%s, Timestamp invalid format", __func__);
			}
		}
		else
		{
			ESP_LOGE(GPRS_MODULE_TAG, "%s, GPRS invalid format", __func__);
		}
	}
	else
	{

		ESP_LOGE(GPRS_MODULE_TAG, "%s, invalid received message", __func__);
	}
}

/* Public callback ----------------------------------------------- */

/*
 * This function must be implemented in the application of communication.
 */
__attribute__((weak)) void GPRS_MODULE_NOTIFY_APP(const pivot_actions config_in)
{
	UNUSED(config_in);
	return;
}

/**@}*/ 	//gprs_module
/** @}*/	//components

