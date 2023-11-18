/*
 * system_monitoring.c
 *
 *  Created on: 4 de ago. de 2023
 *      Author: soil-dev
 */

/// fazer um notify take e give para o monitoramento.


#include "system_monitoring.h"

#include "FreeRTOS_defines.h"
#include "log.h"

#include "actuation_app.h"
#include "data_app.h"
#include "comm_app.h"
#include "rtc_app.h"
#include "idp_parser.h"

#include <string.h>

#define SYSTEM_MONITORING_TAG	"system monitoring"

typedef enum {
	SYSTEM_PAUSE = 0,
	SYSTEM_RUNNING,
	SYSTEM_RETURN,
}system_monitoring_states;

static system_monitoring_states system_states = SYSTEM_RUNNING;
static bool system_monitoring_bacK_flag = false;


static TaskHandle_t xTask_monitoring = NULL;
static app_callback system_monitoring_callback = NULL;

static uint8_t system_monitoring_delay = 10;
static pivot_return_config system_monitoring_config = {};

static uint16_t* system_monitoring_current_angle  = &global_angle;


static void system_monitoring_actuation(void);
static void system_monitoring_task(void* arg);


static void system_monitoring_actuation(void)
{
	uint8_t idp = IDP_INVALID;
	uint16_t dwp = 0;
	char str_out[50] = {};

	pivot_actions pivot_actions = {};

	// act on the equipment (pivo_off) - send IDP 01
	actuation_app_get_actions(&pivot_actions, sizeof(pivot_actions));
	pivot_actions.power_state = PIVOT_OFF;

	idp = IDP_1;
	dwp = idp_parser_create_pwd(pivot_actions);
	uint16_t percent = 0;

	arg_pair_t arg_idp_01[] =
	{
		{ "uint8_t", &idp },
		{ "string", SYSTEM_MONITORING_TAG },
		{ "uint16_t", &dwp },
		{ "uint16_t", &percent },
		{ NULL, NULL }
	};

	memset(str_out, 0x00, sizeof(str_out));
	idp_parser_create_package(str_out,arg_idp_01);
	system_monitoring_callback(str_out, COMM_MQTT);

	if(system_monitoring_config.automatic_return == true)
	{
		vTaskDelay(pdMS_TO_TICKS(5000)); // 5 seconds

		if(system_monitoring_bacK_flag == false)
		{
			// act on the equipment - send IDP 01
			pivot_actions.power_state = PIVOT_ON;

			if(pivot_actions.rotation == PIVOT_CW)
			{
				pivot_actions.rotation = PIVOT_CCW;
			}
			else
			{
				pivot_actions.rotation = PIVOT_CW;
			}

			if(system_monitoring_config.water_return == true)
			{
				pivot_actions.rotation = PIVOT_WET;
			}
			else
			{
				pivot_actions.rotation = PIVOT_DRY;
			}

			idp = IDP_1;
			dwp = idp_parser_create_pwd(pivot_actions);
			uint16_t percent = 0;

			arg_pair_t arg_idp_02[] =
			{
				{ "uint8_t", &idp },
				{ "string", SYSTEM_MONITORING_TAG },
				{ "uint16_t", &dwp },
				{ "uint16_t", &percent },
				{ NULL, NULL }
			};

			memset(str_out, 0x00, sizeof(str_out));
			idp_parser_create_package(str_out,arg_idp_02);
			system_monitoring_callback(str_out, COMM_MQTT);

			system_monitoring_bacK_flag = true;
			system_states = SYSTEM_RETURN;
		}
		else
		{
			system_monitoring_bacK_flag = false;
			system_states = SYSTEM_PAUSE;
		}
	}
	else
	{
		system_states = SYSTEM_PAUSE;
	}
}

static void system_monitoring_task(void* arg)
{
	while(1)
	{
		if(system_monitoring_config.start_angle < system_monitoring_config.end_angle)
		{
			if(*system_monitoring_current_angle  < system_monitoring_config.start_angle
			|| *system_monitoring_current_angle > system_monitoring_config.end_angle)
			{
				if(system_states != SYSTEM_PAUSE)
				{
					system_monitoring_actuation();
				}
			}
			else
			{
				system_states = SYSTEM_RUNNING;
			}
		}
		else
		{
			if(*system_monitoring_current_angle > system_monitoring_config.start_angle
			|| *system_monitoring_current_angle < system_monitoring_config.end_angle)
			{
				if(system_states != SYSTEM_PAUSE)
				{
					system_monitoring_actuation();
				}
			}
			else
			{
				system_states = SYSTEM_RUNNING;
			}

		}


		vTaskDelay(pdMS_TO_TICKS(60000)); // 1 minutes todo aumentar depois

		/*
		if(false)
		{
			// isso passara a ser um timer...
			// send IDP 0 (current status)
			system_monitoring_callback("#00$", COMM_MQTT);

			// save current Timestamp
			timestamp_now = rtc_app_get_timestamp(false);
			data_app_save(DATA_TYPE_TIMESTAMP, &timestamp_now, sizeof(timestamp_now));

			vTaskDelay(pdMS_TO_TICKS(60000 * system_monitoring_delay)); // 10 minutes
		}
		*/
	}
}

void system_monitoring_start(const pivot_return_config return_config, uint8_t monitoring_time)
{

	if(monitoring_time > 0)
	{
		system_monitoring_delay = monitoring_time;
	}
	// start timer


	if(return_config.start_angle == 0 && return_config.end_angle == 0)
	{
		ESP_LOGI(SYSTEM_MONITORING_TAG, "Pivot configured from 0° to 360°, without barrier");
	}
	else
	{
		memcpy(&system_monitoring_config,&return_config, sizeof(system_monitoring_config));
		xTaskCreate(&system_monitoring_task,
					SYSTEM_MONITORING_TASK_NAME,
					SYSTEM_MONITORING_TASK_SIZE,
					NULL,
					SYSTEM_MONITORING_TASK_PRIORITY,
					&xTask_monitoring);
	}
}

void system_monitoring_stop(void)
{
	if(xTask_monitoring != NULL)
	{
		vTaskDelete(xTask_monitoring);
		xTask_monitoring = NULL;
	}
}

void system_monitoring_register_callback(const app_callback callback)
{
	system_monitoring_callback = callback;
}



