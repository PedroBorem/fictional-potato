/*
 * scheduling.c
 *
 *  Created on: 31 de jul. de 2023
 *      Author: soil-dev
 */
#include "scheduling.h"

#include "FreeRTOS_defines.h"
#include "log.h"

#include "actuation_app.h"
#include "data_app.h"
#include "comm_app.h"
#include "rtc_app.h"
#include "idp_parser.h"

#include <string.h>
#include <stdbool.h>

#define SCHEDULING_TAG		"scheduling"

static TaskHandle_t xTask_scheduling_idp_14 = NULL;
static TaskHandle_t xTask_scheduling_idp_15 = NULL;
static TaskHandle_t xTask_scheduling_idp_16 = NULL;
static TaskHandle_t xTask_scheduling_idp_17 = NULL;

static pivot_scheduling_date scheduling_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
static pivot_scheduling_off_date scheduling_off_date[CONFIG_SCHEDULING_MAX_VALUE] = {};

static pivot_scheduling_angle scheduling_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};
static pivot_scheduling_off_angle scheduling_off_angle = {};

static bool scheduling_date_status[CONFIG_SCHEDULING_MAX_VALUE] = {};
static bool scheduling_angle_status[CONFIG_SCHEDULING_MAX_VALUE] = {};

static uint16_t* scheduling_current_angle  = &global_angle;
static app_callback scheduling_callback = NULL;


static void scheduling_active(uint8_t position, char* scheduling_id, pivot_actions actions);
static void scheduling_deactivate(char* scheduling_id, bool scheduling_notify_server);

static void scheduling_task_idp_14(void* arg);
static void scheduling_task_idp_15(void* arg);
static void scheduling_task_idp_16(void* arg);
static void scheduling_task_idp_17(void* arg);


static void scheduling_active(uint8_t position, char* scheduling_id, pivot_actions actions)
{
	uint8_t idp = IDP_INVALID;
	uint16_t dwp = 0;
	char str_out[50] = {};

	// create package - send IDP 18
	idp = IDP_18;
	arg_pair_t arg_idp_18[] = {
		{ "uint8_t", &idp },
		{ "string", SCHEDULING_TAG },
		{ "string", scheduling_id},
		{ NULL, NULL }
	};

	memset(str_out, 0x00, sizeof(str_out));
	idp_parser_create_package(str_out, arg_idp_18);
	scheduling_callback(str_out, COMM_MQTT);

	// act on the equipment - send IDP 01
	idp = IDP_1;
	dwp = idp_parser_create_pwd(actions);

	arg_pair_t arg_idp_01[] =
	{
		{ "uint8_t", &idp },
		{ "string", SCHEDULING_TAG },
		{ "uint16_t", &dwp },
		{ "uint16_t", &actions.percentimeter },
		{ NULL, NULL }
	};

	memset(str_out, 0x00, sizeof(str_out));
	idp_parser_create_package(str_out,arg_idp_01);
	scheduling_callback(str_out, COMM_MQTT);

	// log dgb
	rtc_app_get_timestamp(true);
	ESP_LOGW(SCHEDULING_TAG, "processing schedule id : %s (%s)",
			scheduling_id, __func__);
}

static void scheduling_deactivate(char* scheduling_id, bool scheduling_notify_server)
{
	uint8_t idp = IDP_INVALID;
	uint16_t dwp = 0;
	char str_out[50] = {};

	if(scheduling_notify_server == true)
	{
		// create package - send IDP 18
		idp = IDP_18;
		arg_pair_t arg_idp_18[] = {
			{ "uint8_t", &idp },
			{ "string", SCHEDULING_TAG },
			{ "string", scheduling_id},
			{ NULL, NULL }
		};

		memset(str_out, 0x00, sizeof(str_out));
		idp_parser_create_package(str_out, arg_idp_18);
		scheduling_callback(str_out, COMM_MQTT);
	}

	// act on the equipment - send IDP 01
	idp = IDP_1;
	dwp = idp_parser_create_pwd(pivot_actions_off);
	uint8_t percent = 0;

	arg_pair_t arg_idp_01[] =
	{
		{ "uint8_t", &idp },
		{ "string", SCHEDULING_TAG },
		{ "uint16_t", &dwp },
		{ "uint16_t", &percent },
		{ NULL, NULL }
	};

	memset(str_out, 0x00, sizeof(str_out));
	idp_parser_create_package(str_out,arg_idp_01);
	scheduling_callback(str_out, COMM_MQTT);

	// delete SCHEDULING - send IDP 13
	idp = IDP_13;

	arg_pair_t arg_idp_13[] =
	{
		{ "uint8_t", &idp },
		{ "string", SCHEDULING_TAG },
		{ "string", scheduling_id },
		{ "string", SCHEDULING_TAG },
		{ NULL, NULL }
	};

	memset(str_out, 0x00, sizeof(str_out));
	idp_parser_create_package(str_out,arg_idp_13);
	scheduling_callback(str_out, COMM_MQTT);

	// log dgb
	rtc_app_get_timestamp(true);
	ESP_LOGW(SCHEDULING_TAG, "End schedule by date id : %s (%s)",
			scheduling_id, __func__);

}


static void scheduling_task_idp_14(void* arg)
{
	time_t scheduling_timestamp_now = 0;
	memset(scheduling_date_status, false, sizeof(scheduling_date_status));

	while(1)
	{
		//get timestamp
		scheduling_timestamp_now = rtc_app_get_timestamp(false);

		// date analysis
		for(uint8_t date_position = 0; date_position < CONFIG_SCHEDULING_MAX_VALUE; date_position++)
		{
			if(scheduling_timestamp_now > scheduling_date[date_position].start_date
			&& scheduling_timestamp_now < scheduling_date[date_position].end_date
			&& strcmp(scheduling_date[date_position].scheduling_id,"") > 0)
			{
				if(scheduling_date_status[date_position] == false)
				{
					if(scheduling_callback != NULL)
					{
						scheduling_date_status[date_position] = true;

						scheduling_active(date_position,
								scheduling_date[date_position].scheduling_id,
								scheduling_date[date_position].actions);
					}
					else
					{
						ESP_LOGE(SCHEDULING_TAG, "invalid callback");
					}
				}
			}
			else if(scheduling_timestamp_now > scheduling_date[date_position].end_date
			&& scheduling_date[date_position].end_date != 0
			&& strcmp(scheduling_date[date_position].scheduling_id,"") > 0)
			{
				if(scheduling_callback != NULL)
				{
					if(scheduling_date_status[date_position] == true)
					{
						scheduling_date_status[date_position] = false;
						scheduling_deactivate(scheduling_date[date_position].scheduling_id, false);
					}
				}
				else
				{
					ESP_LOGE(SCHEDULING_TAG, "invalid callback");
				}
			}
		}

		vTaskDelay(pdMS_TO_TICKS(5000)); // 5 seconds
	}
}

static void scheduling_task_idp_15(void* arg)
{
	const uint8_t angle_off_set = 5;
	time_t scheduling_timestamp_now = 0;

	memset(scheduling_angle_status, false, sizeof(scheduling_angle_status));

	while(1)
	{
		//get timestamp
		scheduling_timestamp_now = rtc_app_get_timestamp(false);

		// angle analysis
		for(uint8_t angle_position = 0; angle_position < CONFIG_SCHEDULING_MAX_VALUE; angle_position++)
		{
			if(scheduling_timestamp_now > scheduling_angle[angle_position].start_date
			&& strcmp(scheduling_angle[angle_position].scheduling_id,"") > 0)
			{
				if(scheduling_angle_status[angle_position] == false)
				{
					if(scheduling_callback != NULL)
					{
						scheduling_angle_status[angle_position] = true;

						scheduling_active(angle_position,
								scheduling_angle[angle_position].scheduling_id,
								scheduling_angle[angle_position].actions);
					}
				}
				else if( *scheduling_current_angle > (scheduling_angle[angle_position].end_angle - angle_off_set)
				&& *scheduling_current_angle < (scheduling_angle[angle_position].end_angle + angle_off_set ))
				{
					if(scheduling_callback != NULL)
					{
						if(scheduling_date_status[angle_position] == true)
						{
							scheduling_date_status[angle_position] = false;
							scheduling_deactivate(scheduling_angle[angle_position].scheduling_id, false);
						}
					}
					else
					{
						ESP_LOGE(SCHEDULING_TAG, "invalid callback");
					}
				}
			}
		}

		vTaskDelay(pdMS_TO_TICKS(5000)); // 5 seconds
	}
}

static void scheduling_task_idp_16(void* arg)
{
	time_t scheduling_timestamp_now = 0;

	while(1)
	{
		//get timestamp
		scheduling_timestamp_now = rtc_app_get_timestamp(false);

		// angle analysis
		for(uint8_t date_position = 0; date_position < CONFIG_SCHEDULING_MAX_VALUE; date_position++)
		{
			if(scheduling_timestamp_now > scheduling_off_date[date_position].end_date
			&& scheduling_off_date[date_position].end_date != 0
			&& strcmp(scheduling_off_date[date_position].scheduling_id,"") > 0)
			{
				if(scheduling_callback != NULL)
				{
					scheduling_deactivate(scheduling_off_date[date_position].scheduling_id, true);
				}
				else
				{
					ESP_LOGE(SCHEDULING_TAG, "invalid callback");
				}
			}
		}

		vTaskDelay(pdMS_TO_TICKS(5000)); // 5 seconds
	}
}

static void scheduling_task_idp_17(void* arg)
{
	const uint8_t angle_off_set = 5;

	while(1)
	{
		if( *scheduling_current_angle > (scheduling_off_angle.end_angle - angle_off_set)
		&& *scheduling_current_angle < (scheduling_off_angle.end_angle + angle_off_set )
		&& strcmp(scheduling_off_angle.scheduling_id,"") > 0)
		{
			if(scheduling_callback != NULL)
			{
				scheduling_deactivate(scheduling_off_angle.scheduling_id, true);
			}
			else
			{
				ESP_LOGE(SCHEDULING_TAG, "invalid callback");
			}
		}

		vTaskDelay(pdMS_TO_TICKS(5000)); // 5 seconds
	}
}

void scheduling_start(idp_type scheduling_idp, void* scheduling_data)
{
	if(scheduling_data == NULL)
	{
		ESP_LOGE(SCHEDULING_TAG, "Scheduling parameter is NULL");
		return;
	}

	time_t scheduling_timestamp_now = rtc_app_get_timestamp(false);

	switch (scheduling_idp)
	{
		case IDP_14:
		{
			memcpy(scheduling_date, scheduling_data, sizeof(scheduling_date));

			for(uint8_t date_position = 0; date_position < CONFIG_SCHEDULING_MAX_VALUE; date_position++)
			{
				if(scheduling_timestamp_now > scheduling_date[date_position].end_date
				&& strcmp(scheduling_date[date_position].scheduling_id,"") > 0)
				{
					data_app_delete(scheduling_date[date_position].scheduling_id);
					data_app_load(DATA_TYPE_SCHEDULING_DATE, &scheduling_date);
				}
			}

			if(xTask_scheduling_idp_14 == NULL)
			{
				xTaskCreate(&scheduling_task_idp_14,
						"task_idp_14",
						SCHEDULING_TASK_SIZE,
						NULL,
						SCHEDULING_TASK_PRIORITY,
						&xTask_scheduling_idp_14);
			}
			break;
		}
		case IDP_15:
		{
			memcpy(scheduling_angle, scheduling_data, sizeof(scheduling_angle));

			for(uint8_t angle_position = 0; angle_position < CONFIG_SCHEDULING_MAX_VALUE; angle_position++)
			{
				if((scheduling_timestamp_now > scheduling_angle[angle_position].start_date)
				&& (scheduling_timestamp_now - scheduling_angle[angle_position].start_date) > 3600
				&& (strcmp(scheduling_angle[angle_position].scheduling_id,"") > 0))
				{
					data_app_delete(scheduling_angle[angle_position].scheduling_id);
					data_app_load(DATA_TYPE_SCHEDULING_ANGLE, &scheduling_angle);
				}
			}

			if(xTask_scheduling_idp_15 == NULL)
			{
				xTaskCreate(&scheduling_task_idp_15,
						"task_idp_15",
						SCHEDULING_TASK_SIZE,
						NULL,
						SCHEDULING_TASK_PRIORITY,
						&xTask_scheduling_idp_15);
			}
			break;
		}
		case IDP_16:
		{
			memcpy(scheduling_off_date, scheduling_data, sizeof(scheduling_off_date));

			for(uint8_t date_position = 0; date_position < CONFIG_SCHEDULING_MAX_VALUE; date_position++)
			{
				if(scheduling_timestamp_now > scheduling_off_date[date_position].end_date
				&& strcmp(scheduling_off_date[date_position].scheduling_id,"") > 0)
				{
					data_app_delete(scheduling_off_date[date_position].scheduling_id);
					data_app_load(DATA_TYPE_SCHEDULING_DATE, &scheduling_off_date);
				}
			}

			if(xTask_scheduling_idp_16 == NULL)
			{
				xTaskCreate(&scheduling_task_idp_16,
						"task_idp_16",
						SCHEDULING_TASK_SIZE,
						NULL,
						SCHEDULING_TASK_PRIORITY,
						&xTask_scheduling_idp_16);
			}
			break;
		}
		case IDP_17:
		{
			memcpy(&scheduling_off_angle, scheduling_data, sizeof(scheduling_off_angle));

			if(xTask_scheduling_idp_17 == NULL)
			{
				xTaskCreate(&scheduling_task_idp_17,
						"task_idp_17",
						SCHEDULING_TASK_SIZE,
						NULL,
						SCHEDULING_TASK_PRIORITY,
						&xTask_scheduling_idp_17);
			}
			break;
		}
		default:
		{
			ESP_LOGE(SCHEDULING_TAG, "invalid scheduling idp %s", __func__);
			break;
		}
	}
}

void scheduling_stop(idp_type scheduling_idp)
{
	switch (scheduling_idp)
	{
		case IDP_14:
		{
			if(xTask_scheduling_idp_14 != NULL)
			{
				vTaskDelete(xTask_scheduling_idp_14);
				xTask_scheduling_idp_14 = NULL;
			}
			break;
		}
		case IDP_15:
		{
			if(xTask_scheduling_idp_15 != NULL)
			{
				vTaskDelete(xTask_scheduling_idp_15);
				xTask_scheduling_idp_15 = NULL;
			}
			break;
		}
		case IDP_16:
		{
			if(xTask_scheduling_idp_16 != NULL)
			{
				vTaskDelete(xTask_scheduling_idp_16);
				xTask_scheduling_idp_16 = NULL;
			}
			break;
		}
		case IDP_17:
		{
			if(xTask_scheduling_idp_17 != NULL)
			{
				vTaskDelete(xTask_scheduling_idp_17);
				xTask_scheduling_idp_17 = NULL;
			}
			break;
		}
		default:
		{
			ESP_LOGE(SCHEDULING_TAG, "invalid scheduling idp %s", __func__);
			break;
		}
	}
}

void scheduling_register_callback(const app_callback callback)
{
	if(callback != NULL)
	{
		scheduling_callback = callback;
	}
}
