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

#include <string.h>
#include <stdbool.h>

#define SCHEDULING_TAG		"scheduling"

static TaskHandle_t xTask_scheduling = NULL;

static pivot_scheduling_date scheduling_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
static pivot_scheduling_angle scheduling_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};
static bool scheduling_date_status[CONFIG_SCHEDULING_MAX_VALUE] = {};
static bool scheduling_angle_status[CONFIG_SCHEDULING_MAX_VALUE] = {};

static uint16_t* scheduling_current_angle  = &global_angle;
static app_callback scheduling_callback = NULL;

static void scheduling_task(void* arg);

static void scheduling_task(void* arg)
{
	const uint8_t angle_off_set = 5;

	time_t scheduling_timestamp_now = 0;

	memset(scheduling_date_status, false, sizeof(scheduling_date_status));
	memset(scheduling_angle_status, false, sizeof(scheduling_angle_status));

	scheduling_timestamp_now = rtc_app_get_timestamp(true);

	if(scheduling_timestamp_now == 0)
	{
		vTaskDelay(pdMS_TO_TICKS(15000)); // Delay RTC sync
		scheduling_timestamp_now = rtc_app_get_timestamp(true);
	}

	for(uint8_t date_position = 0; date_position < CONFIG_SCHEDULING_MAX_VALUE; date_position++)
	{
		if(scheduling_timestamp_now > scheduling_date[date_position].end_date
		&& strcmp(scheduling_date[date_position].scheduling_id,"") > 0)
		{
			data_app_delete(scheduling_date[date_position].scheduling_id);
			data_app_load(DATA_TYPE_SCHEADULING_DATE, &scheduling_date);
		}
	}
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
					scheduling_date_status[date_position] = true;
					// todo adicionar a chamada do system manager 01
					data_app_save(DATA_TYPE_ACTIONS, &scheduling_date[date_position].actions,
												sizeof(scheduling_date[date_position].actions));
					rtc_app_get_timestamp(true);
					ESP_LOGI(SCHEDULING_TAG, "processing schedule by date id : %s", scheduling_date[date_position].scheduling_id);
				}
			}
			else if(scheduling_timestamp_now > scheduling_date[date_position].end_date
			&& scheduling_date[date_position].end_date != 0
			&& strcmp(scheduling_date[date_position].scheduling_id,"") > 0)
			{
				scheduling_date_status[date_position] = false;
				rtc_app_get_timestamp(true);

				if(scheduling_callback != NULL)
				{
					scheduling_callback("#30-off$", COMM_MQTT);
				}
				else
				{
					ESP_LOGE(SCHEDULING_TAG, "invalid callback");
				}
				// todo adicionar a chamada do system manager 01
				data_app_delete(scheduling_date[date_position].scheduling_id);
				data_app_load(DATA_TYPE_SCHEADULING_DATE, &scheduling_date);
			}
		}

		// angle analysis
		for(uint8_t angle_position = 0; angle_position < CONFIG_SCHEDULING_MAX_VALUE; angle_position++)
		{
			if(scheduling_timestamp_now > scheduling_angle[angle_position].start_date
			&& strcmp(scheduling_angle[angle_position].scheduling_id,"") > 0)
			{
				if(scheduling_angle_status[angle_position] == false)
				{
					scheduling_angle_status[angle_position] = true;
					data_app_save(DATA_TYPE_ACTIONS, &scheduling_angle[angle_position].actions,
							sizeof(scheduling_angle[angle_position].actions));
					// todo adicionar a chamada do system manager 01
					rtc_app_get_timestamp(true);
					ESP_LOGW(SCHEDULING_TAG, "processing schedule by angle id : %s",
							scheduling_angle[angle_position].scheduling_id);
				}
				else if( *scheduling_current_angle > (scheduling_angle[angle_position].end_angle - angle_off_set)
				&& *scheduling_current_angle < (scheduling_angle[angle_position].end_angle + angle_off_set ))
				{
					scheduling_angle_status[angle_position] = false;
					rtc_app_get_timestamp(true);
					// todo adicionar a chamada do system manager 01

					if(scheduling_callback != NULL)
					{
						//scheduling_callback("#30-off$", COMM_MQTT);
						ESP_LOGW(SCHEDULING_TAG, "todo ...");
					}
					else
					{
						ESP_LOGE(SCHEDULING_TAG, "invalid callback");
					}

					data_app_delete(scheduling_angle[angle_position].scheduling_id);
					data_app_load(DATA_TYPE_SCHEADULING_ANGLE, &scheduling_angle);

				}
			}
		}
		vTaskDelay(pdMS_TO_TICKS(5000)); // 5 seconds
	}
}

void scheduling_start(pivot_scheduling_date* in_scheduling_date, pivot_scheduling_angle* in_scheduling_angle)
{
	if(in_scheduling_date != NULL)
	{
		memcpy(scheduling_date, in_scheduling_date, sizeof(scheduling_date));
	}

	if(in_scheduling_angle != NULL)
	{
		memcpy(scheduling_angle, in_scheduling_angle, sizeof(scheduling_angle));
	}

	if(xTask_scheduling == NULL)
	{
		xTaskCreate(&scheduling_task,
				SCHEDULING_TASK_NAME,
				SCHEDULING_TASK_SIZE,
				NULL,
				SCHEDULING_TASK_PRIORITY,
				&xTask_scheduling);
	}
}

void scheduling_stop(void)
{
	if(xTask_scheduling != NULL)
	{
		vTaskDelete(xTask_scheduling);
		xTask_scheduling = NULL;
	}
}

void scheduling_register_callback(const app_callback callback)
{
	if(callback != NULL)
	{
		scheduling_callback = callback;
	}
}



