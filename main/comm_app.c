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
#include "common_parser.h"

//TODO: remover esse include
#include "data_app.h"

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
 *	Structure sent to the control queue
 *
 */
typedef struct
{
	idp_type request_idp; 			/*!< Request types sent to the control queue*/
	char request_buffer[100];			/*!< Configuration input*/
}comm_app_request;

/* Private variables  -------------------------------------------- */
//FreeRTOS
static TaskHandle_t xTask_comm_app = NULL;
static QueueHandle_t xQueue_comm_app = NULL;

// callback with main
static app_callback comm_app_call = NULL;

// gprs id
static char comm_app_grps_id[30] = {};

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

	if(err == ESP_OK && callback != NULL )
	{
		pivot_config current_config = {};

		wifi_app_register_callback(callback);
		http_server_register_callback(callback);

		comm_app_call = callback;

		callback(CALL_READ_CONFIG, &current_config);
		err &= wifi_app_init(current_config.pivot_id);
		err &= http_server_init();

		xQueue_comm_app = xQueueCreate(COMM_APP_SIZE_QUEUE, sizeof(comm_app_request));
		if(xQueue_comm_app != NULL && err == ESP_OK)
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
			}
			else
			{
				ESP_LOGE(COMM_APP_TAG, "%s, failed to create task: %s", __func__, COMM_APP_TASK_NAME);
			}
		}
		else
		{
			ESP_LOGE(COMM_APP_TAG, "%s, failed to create HTTP/WIFI", __func__);
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

void comm_app_send_event(pivot_actions pivot_status)
{
	uint16_t degree = comm_app_get_degree();
	rf_module_send_event(pivot_status);
	gprs_module_send_event(pivot_status, degree, comm_app_grps_id);
}

void comm_app_set_config(const pivot_config config)
{
	http_server_set_str_config(config);

	if( gprs_module_set_id(config.gprs_id) == ESP_OK)
	{
		memset(comm_app_grps_id, 0x00, sizeof(comm_app_grps_id));
		memcpy(comm_app_grps_id, config.gprs_id, sizeof(comm_app_grps_id));
	}
}

void comm_app_set_actions(const pivot_actions action, const pivot_config config, uint16_t start_angle, uint16_t end_angle)
{
	http_server_set_str_actions(action, config, start_angle, end_angle);
}

void comm_app_send_actions(void)
{
	http_server_alert_actions();
}

void comm_app_send_idp_resp(idp_type idp, char* pivo_id, char* scheaduling_id)
{
	char resp_mqtt[50] = {};

	common_parser_ipm_resp(idp, pivo_id, resp_mqtt, scheaduling_id);
	gprs_module_send_idp(resp_mqtt);
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
		if(xQueueReceive(xQueue_comm_app, (void*)&comm_request, (TickType_t)portMAX_DELAY) == pdTRUE)
		{
			switch(comm_request.request_idp)
			{
				case IDP_0:
				{
					comm_app_call(CALL_READ_ACTION, NULL);
					break;
				}
				case IDP_1:
				{
					pivot_actions action = {};
					common_parser_string_to_action(comm_request.request_buffer, &action);
					comm_app_call(CALL_SAVE_ACTION, &action);
					break;
				}
				case IDP_2:
				{
					pivot_config config = {};//todo: alterar isso
					data_app_load_config(&config, sizeof(config));

					pivot_scheduling_date scheduling_date = {};
					common_parser_string_to_scheaduling_date(comm_request.request_buffer, &scheduling_date);
					comm_app_call(CALL_SAVE_SCHEDULE_DATE, &scheduling_date);
					comm_app_send_idp_resp(comm_request.request_idp,config.gprs_id, scheduling_date.scheduling_id);
					break;
				}
				case IDP_3:
				{
					pivot_config config = {}; //todo: alterar isso
					data_app_load_config(&config, sizeof(config));

					pivot_scheduling_angle scheduling_angle = {};
					common_parser_string_to_scheaduling_angle(comm_request.request_buffer, &scheduling_angle);
					comm_app_call(CALL_SAVE_SCHEDULE_ANGLE, &scheduling_angle);
					comm_app_send_idp_resp(comm_request.request_idp, config.gprs_id, scheduling_angle.scheduling_id);
					break;
				}
				case IDP_4:
				{
					pivot_config config = {}; //todo: alterar isso
					data_app_load_config(&config, sizeof(config));

					pivot_scheduling_date scheduling_date = {};
					common_parser_string_to_scheaduling_date(comm_request.request_buffer, &scheduling_date);
					comm_app_call(CALL_SAVE_SCHEDULE_DATE, &scheduling_date);
					comm_app_send_idp_resp(comm_request.request_idp, config.gprs_id, scheduling_date.scheduling_id);
					break;
				}
				case IDP_5:
				{
					break;
				}
				case IDP_6:
				{
					pivot_config config = {}; //todo: alterar isso
					data_app_load_config(&config, sizeof(config));

					pivot_scheduling_date scheduling_date = {};
					common_parser_string_to_scheaduling_date(comm_request.request_buffer, &scheduling_date);
					comm_app_call(CALL_DELETE_SCHEDULE_DATE, &scheduling_date.scheduling_id);
					comm_app_call(CALL_DELETE_SCHEDULE_ANGLE, &scheduling_date.scheduling_id);

					comm_app_send_idp_resp(comm_request.request_idp, config.gprs_id, scheduling_date.scheduling_id);
					break;
				}
				case IDP_7:
				{
					comm_app_call(CALL_OFF_PIVOT, NULL);
					break;
				}
				case IDP_INVALID:
				{
					break;
				}
				default:
				{
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
void MODULES_NOTIFY_APP(void* notify_buffer)
{

}

void RF_MODULE_NOTIFY_APP(void* notify_buffer)
{
	comm_app_request comm_request = {};
	idp_type idp = common_parser_get_idp(notify_buffer);

	memcpy(comm_request.request_buffer, notify_buffer, sizeof(comm_request.request_buffer));

	if(idp == IDP_INVALID)
	{
		pivot_actions action = {};

		if(common_parser_string_to_action(notify_buffer, &action) == ESP_OK)
		{
			if(action.power_state == 0 && action.rotation == 0
			&& action.watering_state == 0 && action.percentimeter == 0)
			{
				//comm_request.request_idp = IDP_0;
				comm_app_call(CALL_READ_ACTION, NULL);
			}
			else if(action.power_state == PIVOT_OFF && action.rotation == 0
			&& action.watering_state == 0 && action.percentimeter == 0)
			{
				//comm_request.request_idp = IDP_7;
				comm_app_call(CALL_OFF_PIVOT, NULL);
			}
			else
			{
				comm_app_call(CALL_SAVE_ACTION, &action);
				//comm_request.request_idp = IDP_1;
				//comm_request.request_buffer = &action;
			}
		}
		else
		{
			ESP_LOGE(COMM_APP_TAG, "%s, Invalid RF msg", __func__);
		}

		return;
	}
	else
	{
		comm_request.request_idp = idp;
	}

	if(xQueue_comm_app != NULL)
	{
		xQueueSend(xQueue_comm_app, &comm_request ,(TickType_t)1000);
	}
	else
	{
		ESP_LOGW(COMM_APP_TAG, "%s, queue not yet created", __func__);
	}
}

void GPRS_MODULE_NOTIFY_APP(void* notify_buffer)
{
	comm_app_request comm_request = {};
	idp_type idp = common_parser_get_idp(notify_buffer);

	if(idp != IDP_INVALID)
	{
		comm_request.request_idp = idp;
		memcpy(comm_request.request_buffer, notify_buffer, sizeof(comm_request.request_buffer));

		if(xQueue_comm_app != NULL)
		{
			xQueueSend(xQueue_comm_app, &comm_request ,(TickType_t)1000);
		}
		else
		{
			ESP_LOGW(COMM_APP_TAG, "%s, queue not yet created", __func__);
		}
	}
	else
	{
		pivot_actions action = {};

		memcpy(&action, notify_buffer, sizeof(action));

		if(action.power_state == 0 && action.rotation == 0
		&& action.watering_state == 0 && action.percentimeter == 0)
		{
			comm_app_call(CALL_READ_ACTION, NULL);
		}
		else if(action.power_state == PIVOT_OFF && action.rotation == 0
		&& action.watering_state == 0 && action.percentimeter == 0)
		{
			comm_app_call(CALL_OFF_PIVOT, NULL);
		}
		else
		{
			comm_app_call(CALL_SAVE_ACTION, &action);
		}
	}
}

