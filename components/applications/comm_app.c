/**
 * @file comm_app.c
 * @date June 23, 2022
 * @brief Communication class control application.
 */

/* Self include */
#include "comm_app.h"

#include "gprs_uart.h"
#include "rf_uart.h"

#include "log.h"

/* Private definitions ------------------------------------------- */

/**
 * @def COMM_APP_TAG
 * @brief Tag used for logging within the comm_app module.
 */
#define COMM_APP_TAG "comm_app"

/* Private variables  -------------------------------------------- */

/* Private methods  ---------------------------------------------- */

/* Public methods ------------------------------------------------ */

/**
 * @brief Initializes the communication class.
 * @param callback [in]: Callback function for communication events.
 * @return esp_err_t: Error code indicating the success of the initialization.
 * 
 * This function initializes various communication modules and registers a callback
 * function to be used for communication events.
 */
esp_err_t comm_app_init(const app_callback callback)
{
    esp_err_t err = ESP_OK;

    err &= rf_uart_init(callback);
    err &= gprs_uart_init(callback);

    return err;
}

/**
 * @brief Sends an IDP package using the specified communication type.
 * @param idp_pack [in]: IDP package to send.
 * @param communication [in]: Type of communication (e.g., COMM_MQTT, COMM_HTTP_POST).
 * 
 * This function sends an IDP package using the specified communication type.
 * If the communication type is HTTP, it uses the HTTP server; if MQTT, it uses GPRS and RF UART.
 */
void comm_app_send_idp_pack(const char* idp_pack, comm_type communication)
{
    char* str_copy = strdup(idp_pack);

    if (communication == COMM_MQTT)
    {
        gprs_uart_send_event(str_copy, strlen(str_copy));
        LOG_COMM(COMM_APP_TAG, "MQTT - send %s", str_copy);

    }
    else if(communication == COMM_RF)
    {
        rf_uart_send_event(str_copy, strlen(str_copy));
        LOG_COMM(COMM_APP_TAG, "RF - send %s", str_copy);

    }

    free(str_copy);
}

esp_err_t comm_app_set_main_mode_config(pivot_comm_main_mode_config config)
{
    esp_err_t err = ESP_FAIL;
    /* configuration variables */
	if(strcmp(config.comm_main_mode_config, "RF") == 0)
	{
		comm_main_mode = COMM_RF;
        err = ESP_OK;
        return err;
	}
	else
	{
        comm_main_mode = COMM_MQTT;
		ESP_LOGE(COMM_APP_TAG,"Invalid Comm Mode type configuration");
		LOG_DBG_ERROR(COMM_APP_TAG, "Invalid_comm_mode_type");
		return err;
	}
}