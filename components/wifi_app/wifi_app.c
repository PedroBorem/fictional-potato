/*
 * wifi_app.c
 *
 *  Created on: 29 de jan de 2023
 *      Author: brunolima
 */
#include "wifi_app.h"

#include "project_config.h"

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"

#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/ip4_addr.h"

#include "data_app.h"
#include "http_api.h"

#include "project_commons.h"
#include "crc32.h"
#include "utils.h"

#define WIFI_APP_TAG                "wifi_app"

#define WIFI_APP_AP_SSID_BASE       "Soil "
#define WIFI_APP_AP_CHANNEL         7
#define WIFI_APP_AP_MAX_STA_CONN    10
#define WIFI_APP_AP_AUTH_MODE       WIFI_AUTH_WPA2_PSK

/** Bits on event group to hold wifi mode and status. */
#define WIFI_APP_STA_MODE_BIT           (1 << 0)
#define WIFI_APP_STATUS_CONNECTED_BIT   (1 << 1)

/** Time in milliseconds to execute wifi configuration fallback, if not sucessfully connected to AP. */
#define WIFI_CONFIG_FALLBACK_TIMEOUT    30000

typedef struct {
    bool check;
    bool force_ap;
} wifi_fallback_control_t;

static esp_err_t wifi_app_init_ap(void);

static EventGroupHandle_t wifi_app_status = NULL;
static esp_netif_t* sta_netif;

/**
 * @brief Wifi access point event handler
 * 
 * @param arg           Handler arguments.
 * @param event_base    Event type.
 * @param event_id      Event identifier.
 * @param event_data    Data. 
 */
static void wifi_app_ap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_START)
    {
        http_server_start();
    }
    else if (event_id == WIFI_EVENT_AP_STOP)
    {
        http_server_stop();
    }
    else if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*)event_data;
        ESP_LOGI(WIFI_APP_TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*)event_data;
        ESP_LOGI(WIFI_APP_TAG, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
    }
}

/**
 * @brief Generates access point password based on device MAC.
 * 
 * @param password      Buffer to store the password.
 * @param device_mac    Buffer containing the device MAC.
 */
static void wifi_app_get_ap_password(uint8_t *password, uint8_t *device_mac)
{
    uint32_t crc_table[UINT8_MAX + 1];
    uint32_t crc_result = MSF_CRC_INIT;
    uint8_t aux_buffer[sizeof(crc_result) * 2];
    char *calculate_password;

    crc32_init_table(crc_table, MSF_CRC_POLYNOM);

    // Prefix and suffix are added as salt in the hash.
    aux_buffer[0] = 20;
    memcpy(&aux_buffer[1], device_mac, MAC_ADDRESS_LEN);
    aux_buffer[7] = 22;

    crc_result = crc32_calculate(crc_result, crc_table, aux_buffer, sizeof(crc_result) * 2);

    calculate_password = generate_hex_string_from_byte_stream((uint8_t *)&crc_result, sizeof(crc_result));
    
    strcpy((char *)password, calculate_password);
    free(calculate_password);
}

/**
 * @brief Populates device access point configuration structure.
 * 
 * SSID and password are initialized with values based on device
 * access point MAC address.
 * 
 * @param wifi_config   Pointer to store the access point configuration.
 */
static void wifi_app_get_ap_config(wifi_config_t *wifi_config)
{
    wifi_config->ap.channel = WIFI_APP_AP_CHANNEL;
    wifi_config->ap.max_connection = WIFI_APP_AP_MAX_STA_CONN;
    wifi_config->ap.authmode = WIFI_APP_AP_AUTH_MODE;

    // Set up AP SSID
    sprintf((char*)(wifi_config->ap.ssid), "%s%s", WIFI_APP_AP_SSID_BASE, get_sta_mac_dash_str());
    wifi_config->ap.ssid_len = strlen((char*)(wifi_config->ap.ssid));

    // Set up AP password
    wifi_app_get_ap_password(wifi_config->ap.password, get_sta_mac_bytes());

    ESP_LOGI(WIFI_APP_TAG, "PASSWORD: %s", (char*)wifi_config->ap.password);
}

/**
 * @brief Initializes wifi access point.
 * 
 * @return Error code. 
 */
static esp_err_t wifi_app_init_ap(void)
{
    esp_err_t esp_err = ESP_OK;
    wifi_config_t wifi_config;

    ESP_LOGI(WIFI_APP_TAG, "AP init");

    xEventGroupClearBits(wifi_app_status, WIFI_APP_STA_MODE_BIT);
    xEventGroupClearBits(wifi_app_status, WIFI_APP_STATUS_CONNECTED_BIT);

    wifi_app_get_ap_config(&wifi_config);

    esp_err |= esp_wifi_stop();

    esp_err |= esp_wifi_set_mode(WIFI_MODE_AP);
    esp_err |= esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_err |= esp_wifi_start();

    return esp_err;
}

/**
 * @brief Wifi app controller initialization.
 * 
 * Initializes interfaces for station and access point.
 * 
 * @param config   Wifi configuration.
 * 
 * @return Error code. 
 */
esp_err_t wifi_app_init(void)
{
    esp_err_t esp_err = ESP_OK;
    esp_netif_t* ap_netif;
    esp_netif_ip_info_t ip_info;

    LOG_COMM(WIFI_APP_TAG, "WIFI init");

    if (wifi_app_status == NULL) {
        wifi_app_status = xEventGroupCreate();
    }

    esp_err |= esp_netif_init();
    ap_netif = esp_netif_create_default_wifi_ap();

    // Set AP gateway address
    ip_info.ip.addr = ipaddr_addr("192.168.0.1");
    ip_info.gw.addr = ipaddr_addr("192.168.0.1");
    ip_info.netmask.addr = ipaddr_addr("255.255.255.0");

    esp_netif_dhcps_stop(ap_netif);
    esp_netif_set_ip_info(ap_netif, &ip_info);
    esp_netif_dhcps_start(ap_netif);

    sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.nvs_enable = 0;

    esp_err |= esp_wifi_init(&cfg);

    // AP events
    esp_err |= esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_START, &wifi_app_ap_event_handler, NULL, NULL);
    esp_err |= esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_STOP, &wifi_app_ap_event_handler, NULL, NULL);
    esp_err |= esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &wifi_app_ap_event_handler, NULL, NULL);
    esp_err |= esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &wifi_app_ap_event_handler, NULL, NULL);

    esp_err = wifi_app_init_ap();

    return esp_err;
}
