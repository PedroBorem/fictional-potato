/**
 * @file comm_app.c
 * @date June 23, 2022
 * @brief communication class control application
*/

/* Self include */
#include "comm_app.h"

#include "gprs_uart.h"
#include "rf_uart.h"

#include "wifi_app.h"
#include "http_api.h"

#include "log.h"

/* Private definitions ------------------------------------------- */
#define COMM_APP_TAG			"comm_app"

/* Private variables  -------------------------------------------- */

/* Private methods  ---------------------------------------------- */

/* Public methods ------------------------------------------------ */
esp_err_t comm_app_init(const app_callback callback)
{
	esp_err_t err = ESP_OK;

	err &= rf_uart_init(callback);
	err &= gprs_uart_init(callback);

	err &= http_server_register_callback(callback);

	err &= wifi_app_init();
	err &= http_server_init();

	return err;
}

void comm_app_send_idp_pack(const char* idp_pack, comm_type communication)
{
	char* str_copy = strdup(idp_pack);

	LOG_COMM(COMM_APP_TAG, "send %s", str_copy);

	if(communication == COMM_HTTP_POST
	|| communication == COMM_HTTP_GET )
	{
		http_server_send_resp(str_copy);
	}
	else if(communication == COMM_MQTT)
	{
		gprs_uart_send_event(str_copy, strlen(str_copy));
		rf_uart_send_event(str_copy, strlen(str_copy));
	}

	free(str_copy);
}

void comm_app_wifi_config(char* wifi_ssid, char* wifi_pass)
{
	wifi_app_set_config(wifi_ssid, wifi_pass);
}



