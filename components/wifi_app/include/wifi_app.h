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
    WIFI_APP_MODE_STA,
} wifi_app_mode_t;

typedef enum {
    WIFI_APP_STATUS_DISCONNECTED = 0,
    WIFI_APP_STATUS_CONNECTED,
} wifi_app_status_t;

typedef struct {
    char *ssid;
    char* password;
    bool dhcp_en;
    char* ip;
    char* net_mask;
    char* gateway;
    char* dns_1;
    char* dns_2;
} wifi_app_config_t;

esp_err_t wifi_app_init(const wifi_app_config_t *wifi_app_config);
esp_err_t wifi_app_update_config(const wifi_app_config_t *wifi_app_config);
esp_err_t wifi_app_force_mode_change(void);

wifi_app_mode_t wifi_app_get_mode(void);
wifi_app_status_t wifi_app_get_sta_status(void);

#endif /** WIFI_APP */
