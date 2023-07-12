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
 * This function initializes the Wi-Fi application with the provided SSID.
 *
 * @param wifi_ssid The SSID of the Wi-Fi network.
 * @return esp_err_t Returns ESP_OK if the initialization is successful, otherwise an error code.
 */
esp_err_t wifi_app_init(char* wifi_ssid);

/**
 * @brief Register a callback function for Wi-Fi application events.
 *
 * This function registers a callback function to be called when a Wi-Fi application event occurs.
 *
 * @param callback The callback function to be registered.
 * @return esp_err_t Returns ESP_OK if the registration is successful, otherwise an error code.
 */
esp_err_t wifi_app_register_callback(app_callback callback);

#endif /** COMPONENTS_WIFI_APP_INCLUDE_WIFI_APP_H */
