/*
 * wifi_app.h
 *
 *  Created on: 29 de jan de 2023
 *      Author: brunolima
 */

#ifndef COMPONENTS_WIFI_APP_INCLUDE_WIFI_APP_H
#define COMPONENTS_WIFI_APP_INCLUDE_WIFI_APP_H

#include "project_config.h"

esp_err_t wifi_app_init(char* wifi_ssid);

esp_err_t wifi_app_register_callback(app_callback callback);

#endif /** COMPONENTS_WIFI_APP_INCLUDE_WIFI_APP_H */
