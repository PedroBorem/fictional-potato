/*
 * comm_app.c
 *
 *  Created on: 23 de jun. de 2022
 *      Author: brunolima
 */

/**
 * @file comm_app.c
 * @date June 23, 2022
 * @brief communication class control application
*/

/* Self include */
#include "comm_app.h"

/* Components include */
#include "rf_module.h"
#include "gprs_module.h"
#include "gprs_uart.h"
#include "wifi_app.h"
#include "http_api.h"

/**\addtogroup main
 * @{
 *
 */

/**\addtogroup comm_app
 * @{
 *
 */

/* Private definitions ------------------------------------------- */
#define COMM_APP_TAG			"comm_app"
#define COMM_APP_SIZE_QUEUE		10

/**
 *	Request types sent to the control queue
 *
 */
typedef enum
{
	COMM_REQUEST_NEW_CONFIG = 0, 	/*!< Request to save a new configuration*/
	COMM_REQUEST_READ_STATUS = 1,	/*!< Status read request*/
	COMM_REQUEST_PIVOT_OFF = 2
}comm_app_request_type;

/**
 *	Structure sent to the control queue
 *
 */
typedef struct
{
	comm_app_request_type request_type; /*!< Request types sent to the control queue*/
	pivot_config input_config;			/*!< Configuration input*/
}comm_app_request;

/* Private variables  -------------------------------------------- */
//FreeRTOS
static TaskHandle_t xTask_comm_app = NULL;
static QueueHandle_t xQueue_comm_app = NULL;

static app_callback comm_app_call = NULL;

/* Private methods  ---------------------------------------------- */
void comm_app_task(void* arg);

/* Public methods ------------------------------------------------ */
bool comm_app_init(const app_callback callback)
{
	bool ret = false;
	esp_err_t err = ESP_FAIL;
	BaseType_t xReturn = pdPASS;

	err = rf_module_init();
	err &= gprs_module_init();
	err &= http_server_init();
	err &= wifi_app_init();
	if(err == ESP_OK && callback != NULL )
	{
		http_server_register_callback(callback);
		comm_app_call = callback;

		xQueue_comm_app = xQueueCreate(COMM_APP_SIZE_QUEUE, sizeof(comm_app_request));
		if(xQueue_comm_app != NULL)
		{
			xReturn = xTaskCreate(&comm_app_task,
								COMM_APP_TASK_NAME,
								COMM_APP_STACK_SIZE,
								NULL,
								COMM_APP_TASK_PRIORITY,
								&xTask_comm_app);

			if(xReturn == pdPASS || xTask_comm_app != NULL)
			{
				ret = true;
				const char gprs_id[] = "{\"register\":\"True\",\"id\":\"TesteInatel_5\"}";
				gprs_uart_send_event(gprs_id, sizeof(gprs_id));
			}
			else
			{
				ESP_LOGE(COMM_APP_TAG, "%s, failed to create Testetask: %s", __func__, DATA_APP_TASK_NAME);
			}
		}
		else
		{
			ESP_LOGE(COMM_APP_TAG, "%s, failed to create queue", __func__);
		}

	}
	else
	{
		ESP_LOGE(COMM_APP_TAG, "%s, failed to start communication application", __func__);
	}

	return ret;
}

uint16_t comm_app_get_degree(void)
{
	return rf_module_get_angle();
}

void comm_app_send_event(pivot_config pivot_status)
{
	uint16_t degree = comm_app_get_degree();
	rf_module_send_event(pivot_status);
	gprs_module_send_event(pivot_status, degree);
}

/* Private methods ----------------------------------------------- */
/**
 * @brief 	communication reception task
 * @param	arg[in] : task argument (default NULL)
 */
void comm_app_task(void* arg)
{
	comm_app_request comm_request = {};

	while(1)
	{
		//Waiting for UART event.
		if(xQueueReceive(xQueue_comm_app, (void*)&comm_request, (portTickType)portMAX_DELAY) == pdTRUE)
		{
			switch(comm_request.request_type)
			{
				case COMM_REQUEST_NEW_CONFIG:
				{
					comm_app_call(CALL_NEW_CONFIG, &comm_request.input_config);
					break;
				}
				case COMM_REQUEST_READ_STATUS:
				{
					comm_app_call(CALL_READ_STATUS, NULL);
					break;
				}
				case COMM_REQUEST_PIVOT_OFF:
				{
					comm_app_call(CALL_OFF_PIVOT, NULL);
					break;
				}
			}
		}

		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

/**@}*/ 	//comm_app
/** @}*/	//main

/* Public callback ----------------------------------------------- */
void MODULES_NOTIFY_APP(const pivot_config config_in)
{
	comm_app_request comm_request = {};

	if(config_in.power_state == 0 && config_in.rotation == 0
	&& config_in.watering_state == 0 && config_in.percentimeter == 0)
	{
		comm_request.request_type = COMM_REQUEST_READ_STATUS;
	}
	else if(config_in.power_state == PIVOT_OFF && config_in.rotation == 0
	&& config_in.watering_state == 0 && config_in.percentimeter == 0)
	{
		comm_request.request_type = COMM_REQUEST_PIVOT_OFF;
	}
	else
	{
		comm_request.request_type = COMM_REQUEST_NEW_CONFIG;
		memcpy(&comm_request.input_config, &config_in, sizeof(comm_request.input_config));
	}

	xQueueSend(xQueue_comm_app, &comm_request ,(TickType_t)1000);
}

void RF_MODULE_NOTIFY_APP(const pivot_config config_in)
{
	MODULES_NOTIFY_APP(config_in);
}

void GPRS_MODULE_NOTIFY_APP(const pivot_config config_in)
{
	MODULES_NOTIFY_APP(config_in);
}

