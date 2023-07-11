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
#include "FreeRTOS_defines.h"
#include "log.h"

/* Private definitions ------------------------------------------- */
#define WIFI_TAG 	"wifi_app"

#define WIFI_DEFAULT_IP			"192.168.0.1"
#define WIFI_DEFAULT_MASK		"255.255.255.0"
#define WIFI_CHANNEL   			7
#define WIFI_MAX_STA_CONN       5

#define WIFI_APP_SIZE_QUEUE_EVENT	5

/* Private variables ------------------------------------ */
static char wifi_global_ssid[35] = {};
static app_callback wifi_app_callback = NULL;

/* freertos variables */
static TaskHandle_t xTask_wifi_app = NULL;
static QueueHandle_t xQueue_wifi_app = NULL;

static esp_netif_t* wifi_ap_netif = NULL;

/* Private function prototype ------------------------------------ */
static esp_err_t wifi_app_start(char* wifi_ssid);
static void wifi_reloader(void);
static void wifi_app_task(void * arg);
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data);

/* Public methods ------------------------------------------------ */
esp_err_t wifi_app_init(char* wifi_ssid)
{
	esp_err_t ret = ESP_FAIL;

	xQueue_wifi_app = xQueueCreate( WIFI_APP_SIZE_QUEUE_EVENT, WIFI_APP_SIZE_QUEUE_EVENT );

	if( xQueue_wifi_app != NULL )
	{
		xTaskCreate(&wifi_app_task,
					WIFI_APP_TASK_NAME,
					WIFI_APP_STACK_SIZE,
					NULL,
					WIFI_APP_TASK_PRIORITY,
					&xTask_wifi_app );

		if( xTask_wifi_app != NULL )
		{
			ret = wifi_app_start(wifi_ssid);
		}
		else
		{
			ret = ESP_FAIL;
		}
	}

    return ret;
}

esp_err_t wifi_app_register_callback(app_callback callback)
{
	esp_err_t ret = ESP_FAIL;

	if(callback != NULL)
	{
		wifi_app_callback = callback;
		ret = ESP_OK;
	}

	return ret;
}


/* Private methods ----------------------------------------------- */
esp_err_t wifi_app_start(char* wifi_ssid)
{
	esp_err_t ret = ESP_FAIL;

	esp_netif_ip_info_t ip_info = {
		.ip.addr = ipaddr_addr(WIFI_DEFAULT_IP),
		.gw.addr = ipaddr_addr(WIFI_DEFAULT_IP),
		.netmask.addr = ipaddr_addr(WIFI_DEFAULT_MASK)
	};

	wifi_config_t wifi_config = {};
	const char* wifi_pass = "soiltech";

	memset(wifi_global_ssid, 0x00, strlen(wifi_global_ssid));
	memcpy(wifi_global_ssid, wifi_ssid, strlen(wifi_ssid));

	strcpy((char*)(wifi_config.ap.ssid), wifi_ssid);
	wifi_config.ap.ssid_len = strlen(wifi_ssid);
	wifi_config.ap.channel = WIFI_CHANNEL;
	strcpy((char*)(wifi_config.ap.password), wifi_pass);
	wifi_config.ap.max_connection = WIFI_MAX_STA_CONN;
	wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

	ret = esp_netif_init();
	if(ret == ESP_OK)
	{
		ret = esp_event_loop_create_default();
		if(ret == ESP_OK)
		{
			if(wifi_ap_netif != NULL)
			{
				esp_netif_destroy_default_wifi(wifi_ap_netif);
			}
			wifi_ap_netif = esp_netif_create_default_wifi_ap();

			esp_netif_dhcps_stop(wifi_ap_netif);
			esp_netif_set_ip_info(wifi_ap_netif, &ip_info);
			esp_netif_dhcps_start(wifi_ap_netif);

			wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
			ret &= esp_wifi_init(&cfg);
			ret &= esp_event_handler_instance_register(WIFI_EVENT,
														ESP_EVENT_ANY_ID,
														&wifi_event_handler,
														NULL,
														NULL);

			if (strlen(wifi_pass) == 0)
			{
				wifi_config.ap.authmode = WIFI_AUTH_OPEN;
			}

			ret &= esp_wifi_set_mode(WIFI_MODE_AP);
			ret &= esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
			ret &= esp_wifi_start();

			if(ret == ESP_OK)
			{
				LOG_COMM(WIFI_TAG,"WiFi started successfully");
				ESP_LOGW(WIFI_TAG,"WiFI SSID : %s", (char*)wifi_config.ap.ssid);
				ESP_LOGW(WIFI_TAG,"WiFI PASS : %s", (char*)wifi_config.ap.password);
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

static void wifi_reloader(void)
{
	uint8_t wifi_app_queue_req = 0;
	xQueueSend(xQueue_wifi_app, &wifi_app_queue_req, portMAX_DELAY );
}

static void wifi_app_task(void * arg)
{
	uint8_t wifi_app_queue_req;
	while(1)
	{
		if( xQueueReceive(xQueue_wifi_app, &wifi_app_queue_req, portMAX_DELAY ) == pdTRUE )
		{
			if(wifi_app_callback != NULL)
			{
				pivot_config current_config = {};
				wifi_app_callback(CALL_READ_CONFIG, &current_config);

				if(strcmp(current_config.pivot_id, wifi_global_ssid) != 0)
				{
					esp_wifi_stop();
					esp_wifi_deinit();
					esp_event_loop_delete_default();
					esp_netif_deinit();

					wifi_app_start(current_config.pivot_id);
				}
			}
			else
			{
				ESP_LOGE(WIFI_TAG,"unregistered HTTP callback");
			}
		}

		vTaskDelay(pdMS_TO_TICKS(100));
	}

	return;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
	 if (event_id == WIFI_EVENT_AP_STACONNECTED)
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
		wifi_reloader();
    }
}
