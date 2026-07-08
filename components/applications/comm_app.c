/**
 * @file comm_app.c
 * @date June 23, 2022
 * @brief Communication class control application.
 */

/* Self include */
#include "comm_app.h"

#include "gprs_uart.h"
#include "idp_parser.h"
#include "rf_uart.h"

#include "log.h"

#include <stdlib.h>
#include <string.h>

/* Private definitions ------------------------------------------- */

/**
 * @def COMM_APP_TAG
 * @brief Tag used for logging within the comm_app module.
 */
#define COMM_APP_TAG "comm_app"

/* Private variables  -------------------------------------------- */

/* Private methods  ---------------------------------------------- */

/**
 * @brief Checks whether an IDP packet should be hidden from raw communication logs.
 *
 * Heartbeat packets are intentionally kept out of the serial console while the
 * higher-level heartbeat status logs remain enabled.
 *
 * @param idp_pack IDP payload to evaluate.
 * @return true when the payload should not be printed verbatim.
 */
static bool comm_app_hide_raw_log_callback(const char *idp_pack)
{
    return idp_parser_is_payload_from_idp(idp_pack, IDP_42);
}

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

    if (callback == NULL)
    {
        ESP_LOGE(COMM_APP_TAG, "%s, invalid callback", __func__);
        return ESP_ERR_INVALID_ARG;
    }

    err = rf_uart_init(callback);
    if (err != ESP_OK)
    {
        ESP_LOGE(COMM_APP_TAG, "%s, failed to initialize RF UART: %d", __func__, err);
        return err;
    }

    err = gprs_uart_init(callback);
    if (err != ESP_OK)
    {
        ESP_LOGE(COMM_APP_TAG, "%s, failed to initialize GPRS UART: %d", __func__, err);
        return err;
    }

    gprs_uart_register_raw_log_callback(comm_app_hide_raw_log_callback);

    ESP_LOGI(COMM_APP_TAG, "Serial communication initialized: RF and GPRS UART");

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
    bool hide_raw_log = comm_app_hide_raw_log_callback(str_copy);

    if (str_copy == NULL)
    {
        return;
    }

    if (communication == COMM_MQTT)
    {
        gprs_uart_send_event(str_copy, strlen(str_copy));
        if (!hide_raw_log)
        {
            LOG_COMM(COMM_APP_TAG, "MQTT - send %s", str_copy);
        }

    }
    else if(communication == COMM_RF)
    {
        rf_uart_send_event(str_copy, strlen(str_copy));
        if (!hide_raw_log)
        {
            LOG_COMM(COMM_APP_TAG, "RF - send %s", str_copy);
        }

    }
    else
    {
        ESP_LOGW(COMM_APP_TAG, "Unsupported communication target: %d", communication);
    }

    free(str_copy);
}

/**
 * @brief Configures the Wi-Fi settings.
 * @param wifi_ssid [in]: Wi-Fi SSID.
 * @param wifi_pass [in]: Wi-Fi password.
 * 
 * This function configures the Wi-Fi settings for the communication module.
 */
void comm_app_wifi_config(char* wifi_ssid, char* wifi_pass)
{
    UNUSED(wifi_ssid);
    UNUSED(wifi_pass);
    ESP_LOGW(COMM_APP_TAG, "Wi-Fi configuration ignored: HTTP/Wi-Fi is disabled in this firmware stage");
}

/**
 * @brief Reload the Wi-Fi configuration and restart the Wi-Fi application.
 */
void comm_app_wifi_reloader(void)
{
    ESP_LOGW(COMM_APP_TAG, "Wi-Fi reload ignored: HTTP/Wi-Fi is disabled in this firmware stage");
}

esp_err_t comm_app_set_main_mode_config(pivot_comm_main_mode_config config)
{
	if(strcmp(config.comm_main_mode_config, "RF") == 0)
	{
		comm_main_mode = COMM_RF;
        return ESP_OK;
	}
	else if(strcmp(config.comm_main_mode_config, "MQTT") == 0)
	{
        comm_main_mode = COMM_MQTT;
        return ESP_OK;
	}
	else
	{
		ESP_LOGE(COMM_APP_TAG,"Invalid Comm Mode type configuration");
		return ESP_ERR_INVALID_ARG;
	}
}
