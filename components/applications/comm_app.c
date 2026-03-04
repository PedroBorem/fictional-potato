/**
 * @file comm_app.c
 * @date June 23, 2022
 * @brief Communication class control application.
 */

/* Self include */
#include "comm_app.h"

#include "gprs_uart.h"
#include "rf_uart.h"

#include "wifi_app.h"
#include "http_api.h"

#include "log.h"

#include <string.h>

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
    esp_err_t step_err = ESP_OK;

    step_err = rf_uart_init(callback);
    if (step_err != ESP_OK)
    {
        ESP_LOGE(COMM_APP_TAG, "RF init failed (%d)", (int)step_err);
        err = step_err;
    }

    step_err = gprs_uart_init(callback);
    if (step_err != ESP_OK)
    {
        ESP_LOGE(COMM_APP_TAG, "GPRS init failed (%d)", (int)step_err);
        err = step_err;
    }

    step_err = http_server_register_callback(callback);
    if (step_err != ESP_OK)
    {
        ESP_LOGE(COMM_APP_TAG, "HTTP callback registration failed (%d)", (int)step_err);
        err = step_err;
    }

    step_err = wifi_app_init();
    if (step_err != ESP_OK)
    {
        ESP_LOGE(COMM_APP_TAG, "Wi-Fi init failed (%d)", (int)step_err);
        err = step_err;
    }

    step_err = http_server_init();
    if (step_err != ESP_OK)
    {
        ESP_LOGE(COMM_APP_TAG, "HTTP server init failed (%d)", (int)step_err);
        err = step_err;
    }

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
    if (idp_pack == NULL)
    {
        ESP_LOGE(COMM_APP_TAG, "Invalid package pointer (NULL).");
        return;
    }

    size_t pack_len = strlen(idp_pack);
    if (pack_len == 0U)
    {
        ESP_LOGW(COMM_APP_TAG, "Ignoring empty package.");
        return;
    }

    if (communication == COMM_HTTP_POST || communication == COMM_HTTP_GET)
    {
        http_server_send_resp((char*)idp_pack);
        LOG_COMM(COMM_APP_TAG, "HTTP - send %s", idp_pack);
    }
    else if (communication == COMM_MQTT)
    {
        esp_err_t ret = gprs_uart_send_event(idp_pack, pack_len);
        if (ret == ESP_OK)
        {
            LOG_COMM(COMM_APP_TAG, "MQTT - send %s", idp_pack);
        }
        else
        {
            ESP_LOGE(COMM_APP_TAG, "MQTT send failed (%d): %s", (int)ret, idp_pack);
        }

    }
    else if(communication == COMM_RF)
    {
        esp_err_t ret = rf_uart_send_event(idp_pack, pack_len);
        if (ret == ESP_OK)
        {
            LOG_COMM(COMM_APP_TAG, "RF - send %s", idp_pack);
        }
        else
        {
            ESP_LOGE(COMM_APP_TAG, "RF send failed (%d): %s", (int)ret, idp_pack);
        }

    }
    else
    {
        ESP_LOGW(COMM_APP_TAG, "Invalid communication type (%d) for package: %s", (int)communication, idp_pack);
    }
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
    wifi_app_set_config(wifi_ssid, wifi_pass);
}

/**
 * @brief Reload the Wi-Fi configuration and restart the Wi-Fi application.
 */
void comm_app_wifi_reloader(void)
{
	wifi_reloader();
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
		LOG_DBG_ERROR(COMM_APP_TAG, "Invalid_comm_mode_type");
		return ESP_FAIL;
	}
}
