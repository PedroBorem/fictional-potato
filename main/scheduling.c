/*
 * scheduling.c
 *
 *  Created on: 31 de jul. de 2023
 *      Author: soil-dev
 */

#include "scheaduling.h"
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
static pivot_scheduling_off_angle scheduling_off_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};

static bool scheduling_date_status[CONFIG_SCHEDULING_MAX_VALUE] = {};
static bool scheduling_angle_status[CONFIG_SCHEDULING_MAX_VALUE] = {};

static uint16_t* scheduling_current_angle  = &global_angle;
static app_callback scheduling_callback = NULL;

static void scheduling_task_idp_14(void* arg);
static void scheduling_task_idp_15(void* arg);
static void scheduling_task_idp_16(void* arg);
static void scheduling_task_idp_17(void* arg);


static void scheduling_task_idp_14(void* arg)
{
	char pivo_id[30] = "scheduling";
	char str_out[50] = {};

	time_t scheduling_timestamp_now = 0;
	idp_type idp = IDP_18;

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

						// create package - send IDP 18
						arg_pair_t arg_pairs[] = {
							{ "uint8_t", &idp },
							{ "string", pivo_id },
							{ "string", scheduling_date[date_position].scheduling_id},
							{ NULL, NULL }
						};

						idp_parser_create_package(str_out, arg_pairs);
						scheduling_callback(str_out, COMM_MQTT);

						// act on the equipment
						actuation_app_set_actions(scheduling_date[date_position].actions, false);

						data_app_save(DATA_TYPE_ACTIONS, &scheduling_date[date_position].actions,
													sizeof(scheduling_date[date_position].actions));
						rtc_app_get_timestamp(true);
						ESP_LOGW(SCHEDULING_TAG, "processing schedule by date id : %s",
								scheduling_date[date_position].scheduling_id);
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
					scheduling_date_status[date_position] = false;
					rtc_app_get_timestamp(true);

					// off pivot
					scheduling_date[date_position].actions.power_state = PIVOT_OFF;
					actuation_app_set_actions(scheduling_date[date_position].actions, false);

					data_app_delete(scheduling_date[date_position].scheduling_id);
					data_app_load(DATA_TYPE_SCHEADULING_DATE, &scheduling_date);

					ESP_LOGW(SCHEDULING_TAG, "End schedule by date id : %s",
							scheduling_date[date_position].scheduling_id);
				}
				else
				{
					ESP_LOGE(SCHEDULING_TAG, "invalid callback");
				}
			}
		}

		vTaskDelay(pdMS_TO_TICKS(5000)); // 5 seconds

		// save current Timestamp
		data_app_save(DATA_TYPE_TIMESTAMP, &scheduling_timestamp_now, sizeof(scheduling_timestamp_now));
	}
}

static void scheduling_task_idp_15(void* arg)
{
	const uint8_t angle_off_set = 5;
	char pivo_id[30] = "scheduling";
	char str_out[50] = {};

	time_t scheduling_timestamp_now = 0;
	idp_type idp = IDP_18;

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

						// create package - send IDP 18
						arg_pair_t arg_pairs[] = {
							{ "uint8_t", &idp },
							{ "string", pivo_id },
							{ "string", scheduling_angle[angle_position].scheduling_id},
							{ NULL, NULL }
						};

						idp_parser_create_package(str_out, arg_pairs);
						scheduling_callback(str_out, COMM_MQTT);

						// act on the equipment
						actuation_app_set_actions(scheduling_angle[angle_position].actions, false);
						data_app_save(DATA_TYPE_ACTIONS, &scheduling_angle[angle_position].actions,
													sizeof(scheduling_angle[angle_position].actions));
						rtc_app_get_timestamp(true);
						ESP_LOGW(SCHEDULING_TAG, "processing schedule by angle id : %s",
								scheduling_angle[angle_position].scheduling_id);
					}
				}
				else if( *scheduling_current_angle > (scheduling_angle[angle_position].end_angle - angle_off_set)
				&& *scheduling_current_angle < (scheduling_angle[angle_position].end_angle + angle_off_set ))
				{
					if(scheduling_callback != NULL)
					{
						scheduling_angle_status[angle_position] = false;
						rtc_app_get_timestamp(true);

						// off pivot
						scheduling_angle[angle_position].actions.power_state = PIVOT_OFF;
						actuation_app_set_actions(scheduling_angle[angle_position].actions, false);

						data_app_delete(scheduling_angle[angle_position].scheduling_id);
						data_app_load(DATA_TYPE_SCHEADULING_ANGLE, &scheduling_angle);

						ESP_LOGW(SCHEDULING_TAG, "End schedule by angle id : %s",
								scheduling_date[angle_position].scheduling_id);
					}
					else
					{
						ESP_LOGE(SCHEDULING_TAG, "invalid callback");
					}
				}
			}
		}

		vTaskDelay(pdMS_TO_TICKS(5000)); // 5 seconds

		// save current Timestamp
		data_app_save(DATA_TYPE_TIMESTAMP, &scheduling_timestamp_now, sizeof(scheduling_timestamp_now));
	}
}

static void scheduling_task_idp_16(void* arg)
{
	char pivo_id[30] = "scheduling";
	char str_out[50] = {};

	time_t scheduling_timestamp_now = 0;
	pivot_actions actions = {};

	uint8_t idp = IDP_18;

	while(1)
	{
		//get timestamp
		scheduling_timestamp_now = rtc_app_get_timestamp(false);

		// angle analysis
		for(uint8_t date_position = 0; date_position < CONFIG_SCHEDULING_MAX_VALUE; date_position++)
		{
			if(scheduling_timestamp_now > scheduling_date[date_position].end_date
			&& scheduling_date[date_position].end_date != 0
			&& strcmp(scheduling_date[date_position].scheduling_id,"") > 0)
			{
				if(scheduling_callback != NULL)
				{
					// create package - send IDP 18
					arg_pair_t arg_pairs[] = {
						{ "uint8_t", &idp },
						{ "string", pivo_id },
						{ "string", scheduling_date[date_position].scheduling_id},
						{ NULL, NULL }
					};

					idp_parser_create_package(str_out, arg_pairs);
					scheduling_callback(str_out, COMM_MQTT);

					// off pivot
					actions.power_state = PIVOT_OFF;
					actuation_app_set_actions(actions, false);


					data_app_delete(scheduling_off_date[date_position].scheduling_id);
					data_app_load(DATA_TYPE_SCHEADULING_OFF_DATE, &scheduling_off_date);

					rtc_app_get_timestamp(true);
					ESP_LOGW(SCHEDULING_TAG, "processing schedule by off date id : %s",
							scheduling_date[date_position].scheduling_id);
				}
				else
				{
					ESP_LOGE(SCHEDULING_TAG, "invalid callback");
				}
			}
		}

		vTaskDelay(pdMS_TO_TICKS(5000)); // 5 seconds

		// save current Timestamp
		data_app_save(DATA_TYPE_TIMESTAMP, &scheduling_timestamp_now, sizeof(scheduling_timestamp_now));
	}
}

static void scheduling_task_idp_17(void* arg)
{
	while(1)
	{
		// todo implementar
		vTaskDelay(pdMS_TO_TICKS(50000)); // 5 seconds
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
					data_app_load(DATA_TYPE_SCHEADULING_DATE, &scheduling_date);
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
					data_app_load(DATA_TYPE_SCHEADULING_ANGLE, &scheduling_angle);
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
					data_app_load(DATA_TYPE_SCHEADULING_DATE, &scheduling_off_date);
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
			memcpy(scheduling_off_angle, scheduling_data, sizeof(scheduling_off_angle));

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



