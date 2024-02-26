/*
 * comm_app.h
 *
 *  Created on: 23 de jun. de 2022
 *      Author: brunolima
 */

#ifndef COMPONENTS_APPLICATIONS_INCLUDE_COMM_APP_H_
#define COMPONENTS_APPLICATIONS_INCLUDE_COMM_APP_H_

/**
 * @file comm_app.h
 * @date June 23, 2022
 * @brief Communication class control application.
*/

#include "project_config.h"

#include "esp_err.h"

/**
 * @brief Initializes all communication modules.
 * @param callback [in]: Function used with return to the main application class.
 * @return
 *  - ESP_OK: Success
 *  - ESP_FAIL: Fail
 * 
 * This function initializes all communication modules and registers a callback
 * function to be used in the main application class.
 */
esp_err_t comm_app_init(const app_callback callback);

/**
 * @brief Sends an IDP package using the specified communication type.
 * @param idp_pack [in]: IDP package to send.
 * @param communication [in]: Type of communication (e.g., COMM_MQTT, COMM_WIFI).
 * 
 * This function sends an IDP package using the specified communication type.
 */
void comm_app_send_idp_pack(const char* idp_pack, comm_type communication);

/**
 * @brief Configures the Wi-Fi settings.
 * @param wifi_ssid [in]: Wi-Fi SSID.
 * @param wifi_pass [in]: Wi-Fi password.
 * 
 * This function configures the Wi-Fi settings for the communication module.
 */
void comm_app_wifi_config(char* wifi_ssid, char* wifi_pass);

/**
 * @brief Reload the Wi-Fi configuration and restart the Wi-Fi application.
 */
void comm_app_wifi_reloader(void);

#endif /* COMPONENTS_APPLICATIONS_INCLUDE_COMM_APP_H_ */
