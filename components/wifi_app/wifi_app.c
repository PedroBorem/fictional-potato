/*
 * wifi_app.c
 *
 *  Created on: 20 de jan de 2023
 *      Author: brunolima
 */

/* Self include */
#include "wifi_app.h"

/* ESP modules include*/
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include <netdb.h>

/* Components include */
#include "http_api.h"

/* Private definitions ------------------------------------------- */
#define WIFI_TAG 	"wifi_app"

#define WIFI_SSID      			"wifi-test3"
#define WIFI_PASS      			"soiltest123"
#define WIFI_DEFAULT_IP			"192.168.0.1"
#define WIFI_DEFAULT_MASK		"255.255.255.0"
#define WIFI_CHANNEL   			7
#define WIFI_MAX_STA_CONN       10

/* Private function prototype ------------------------------------ */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data);

/* Public methods ------------------------------------------------ */
esp_err_t wifi_app_init(void)
{
	esp_err_t ret = ESP_FAIL;
    esp_netif_t* ap_netif = NULL;

    esp_netif_ip_info_t ip_info = {
		.ip.addr = ipaddr_addr(WIFI_DEFAULT_IP),
		.gw.addr = ipaddr_addr(WIFI_DEFAULT_IP),
		.netmask.addr = ipaddr_addr(WIFI_DEFAULT_MASK)
    };

    wifi_config_t wifi_config = {
		.ap = {
			.ssid = WIFI_SSID,
			.ssid_len = strlen(WIFI_SSID),
			.channel = WIFI_CHANNEL,
			.password = WIFI_PASS,
			.max_connection = WIFI_MAX_STA_CONN,
			.authmode = WIFI_AUTH_WPA_WPA2_PSK
		},
	};

    ret = esp_netif_init();
    if(ret == ESP_OK)
    {
    	ret = esp_event_loop_create_default();
    	if(ret == ESP_OK)
    	{
    		ap_netif = esp_netif_create_default_wifi_ap();

			esp_netif_dhcps_stop(ap_netif);
			esp_netif_set_ip_info(ap_netif, &ip_info);
			esp_netif_dhcps_start(ap_netif);

			wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
			ret &= esp_wifi_init(&cfg);
			ret &= esp_event_handler_instance_register(WIFI_EVENT,
														ESP_EVENT_ANY_ID,
														&wifi_event_handler,
														NULL,
														NULL);

			if (strlen(WIFI_PASS) == 0)
			{
				wifi_config.ap.authmode = WIFI_AUTH_OPEN;
			}

			ret &= esp_wifi_set_mode(WIFI_MODE_AP);
			ret &= esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
			ret &= esp_wifi_start();

			if(ret == ESP_OK)
			{
				LOG_COMM(WIFI_TAG,"WiFi started successfully");
			}
			else
			{
				ret = ESP_FAIL;
				ESP_LOGE(WIFI_TAG, "%s, WiFi Failed to start", __func__);
			}
    	}
    	else
    	{
    		ESP_LOGE(WIFI_TAG, "%s, loop create error", __func__);
    	}
    }
    else
    {
    	ESP_LOGE(WIFI_TAG, "%s, error on init netif", __func__);
    }

    return ret;
}

/* Private methods ----------------------------------------------- */

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
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
		wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
		LOG_COMM(WIFI_TAG, "station "MACSTR" join, AID=%d",
		                 MAC2STR(event->mac), event->aid);
    }
	else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
	{
		wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
		LOG_COMM(WIFI_TAG, "station "MACSTR" leave, AID=%d",
		                 MAC2STR(event->mac), event->aid);
    }
}

