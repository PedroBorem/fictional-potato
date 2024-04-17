/**
 * @file wifi_app.h
 * @date January 29, 2023
 * @brief Wi-Fi application functions and callbacks.
 *
 * This file contains the Wi-Fi application initialization and callback registration functions.
 */

#ifndef COMPONENTS_WIFI_APP_INCLUDE_WIFI_APP_H
#define COMPONENTS_WIFI_APP_INCLUDE_WIFI_APP_H

#include "project_config.h"
#include "esp_err.h"

/**
 * @brief Initialize the Wi-Fi application.
 *
 * This function initializes the Wi-Fi application, including setting up Wi-Fi configurations.
 * @return esp_err_t Returns ESP_OK on success, an error code otherwise.
 */
esp_err_t wifi_app_init(void);

/**
 * @brief Set Wi-Fi configuration parameters.
 *
 * This function sets the Wi-Fi SSID and password for connecting to a Wi-Fi network.
 *
 * @param wifi_ssid The Wi-Fi SSID.
 * @param wifi_pass The Wi-Fi password.
 */
void wifi_app_set_config(char* wifi_ssid, char* wifi_pass);

/**
 * @brief Reload the Wi-Fi configuration and restart the Wi-Fi application.
 */
void wifi_reloader(void);

#endif /** COMPONENTS_WIFI_APP_INCLUDE_WIFI_APP_H */
