/*
 * wifi_app.h
 *
 *  Created on: 29 de jan de 2023
 *      Author: brunolima
 */
#ifndef WIFI_APP
#define WIFI_APP

#include "esp_err.h"
#include "esp_wifi.h"

#include <stdbool.h>

typedef enum {
    WIFI_APP_MODE_AP = 0,
} wifi_app_mode_t;

typedef enum {
    WIFI_APP_STATUS_DISCONNECTED = 0,
    WIFI_APP_STATUS_CONNECTED,
} wifi_app_status_t;

esp_err_t wifi_app_init(void);

#endif /** WIFI_APP */
