/*
 * main.c
 *
 *  Created on: 31 de mai de 2022
 *      Author: brunolima
 */

/**
 * @file main.c
 * @date June 15, 2022
 * @brief system main class
*/

/* Applications include */
#include "rtc_app.h"
#include "data_app.h"
#include "comm_app.h"
#include "actuation_app.h"
#include "project_config.h"

/**\addtogroup main
 * @{
 *
 */

#define MAIN_TAG "main"

/* Private variables ------------------------------------ */
static TaskHandle_t xTask_sectorization_app = NULL;
static TaskHandle_t xTask_peak_hours_app = NULL;

// represents the initial coverage angle of the pivot
static uint16_t app_start_angle = 0;
static pivot_config main_config = {};

/* Private function prototype ------------------------------------ */
static bool app_init(void);
static void app_main_call(app_call_states state,const void* buffer);
static void app_sectorization_task(void* arg);
static void app_peak_hours_task(void* arg);

/**
 * @brief	main class
 *
 */
void app_main(void)
{
	pivot_config current_config = {};
	pivot_actions current_action = {};

	ESP_LOGI(MAIN_TAG,"starting the system ...");
	assert(app_init());

	// get configurations
	data_app_load_config(&current_config, sizeof(current_config));
	actuation_app_get_config(&current_action, sizeof(current_action));

	// set http parameters
	comm_app_set_config(current_config);

	//rtc_app_get_timestamp();
	esp_reset_reason_t reset_cause = esp_reset_reason();
	if(reset_cause == ESP_RST_POWERON || reset_cause == ESP_RST_BROWNOUT)
	{
		// TODO : critica de tempo
		data_app_load_actions(&current_action, sizeof(current_action));

		LOG_DATA(MAIN_TAG, "");
		LOG_DATA(MAIN_TAG, " ------ NVS Current Config ------");
		LOG_DATA(MAIN_TAG, " Power state: %d", current_action.power_state);
		LOG_DATA(MAIN_TAG, " Advance mode: %d", current_action.rotation);
		LOG_DATA(MAIN_TAG, " Watering state: %d", current_action.watering_state);
		LOG_DATA(MAIN_TAG, " Percentimeter %.3d %%", current_action.percentimeter);
		LOG_DATA(MAIN_TAG, " --------------------------------\n");

		vTaskDelay(pdMS_TO_TICKS(500));

		if(current_action.power_state != PIVOT_OFF)
		{
			actuation_app_set_config(current_action, false);
		}
	}

	// get start angle
	app_start_angle = comm_app_get_degree();

	// create sectorization task
	xTaskCreate(&app_sectorization_task,
				MAIN_APP_TASK_1_NAME,
				MAIN_APP_STACK_1_SIZE,
				NULL,
				MAIN_APP_TASK_1_PRIORITY,
				&xTask_sectorization_app);

	// create peak hours task
	xTaskCreate(&app_peak_hours_task,
				MAIN_APP_TASK_2_NAME,
				MAIN_APP_STACK_2_SIZE,
				NULL,
				MAIN_APP_TASK_2_PRIORITY,
				&xTask_peak_hours_app);

	memcpy(&main_config, &current_config, sizeof(main_config));

	while (1)
	{
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

/**
 * @brief	start all sub-applications
 * @return
 * 	- true: applications started successfully
 * 	- false: application startup failure
 */
static bool app_init(void)
{
	bool ret = true;

	ret &= rtc_app_init();
	ret &= actuation_app_init(&app_main_call);
	if(data_app_init() == ESP_FAIL)
	{
		ret = false;
	}
	ret &= comm_app_init(&app_main_call);

	return ret;
}

/**
 * @brief	callback from secondary applications to the main
 * @param	state - [in]: reason why a callback was triggered
 */
static void app_main_call(app_call_states state,const void* buffer)
{
	esp_err_t ret = ESP_FAIL;

	switch(state)
	{
		case CALL_LOAD_ACTION:
		{
			pivot_actions actions = {};
			pivot_config config = {};

			ret = data_app_load_actions(&actions, sizeof(actions));
			ret = data_app_load_config(&config, sizeof(config));

			if(ret == ESP_OK)
			{
				comm_app_set_actions(actions, config, app_start_angle, comm_app_get_degree());
			}

			break;
		}
		case CALL_SAVE_ACTION:
		{
			pivot_actions new_actions = {};
			memcpy(&new_actions, buffer, sizeof(new_actions));

			ret = data_app_save_actions(&new_actions, sizeof(new_actions));
			if(ret == ESP_OK)
			{
				actuation_app_set_config(new_actions, false);
				comm_app_send_event(new_actions);
			}
			else if(new_actions.rotation == PIVOT_UNKNOWN)
			{
				comm_app_send_event(new_actions);
			}

			break;
		}
		case CALL_SAVE_CONFIG:
		{
			pivot_config new_config = {};
			memcpy(&new_config, buffer, sizeof(new_config));

			ret = data_app_save_config(&new_config, sizeof(new_config));
			if(ret == ESP_OK)
			{
				comm_app_set_config(new_config);
				memcpy(&main_config, &new_config, sizeof(main_config));
			}

			break;
		}
		case CALL_MANUAL_PIVOT:
		{
			pivot_actions manual_config = {};
			memcpy(&manual_config, buffer, sizeof(manual_config));

			actuation_app_set_config(manual_config, true);
			comm_app_send_event(manual_config);

			break;
		}
		case CALL_READ_ACTION: // if you receive 000-000
		{
			pivot_actions config_send = {};

			actuation_app_get_config(&config_send, sizeof(config_send));
			comm_app_send_event(config_send);

			break;
		}
		case CALL_OFF_PIVOT:
		{
			pivot_actions current_action = {};

			data_app_load_actions(&current_action, sizeof(current_action));
			vTaskDelay(pdMS_TO_TICKS(500));

			if(current_action.power_state != PIVOT_OFF)
			{
				current_action.power_state = PIVOT_OFF;
				actuation_app_set_config(current_action, false);
				data_app_save_actions(&current_action, sizeof(current_action));
			}

			vTaskDelay(pdMS_TO_TICKS(2000));

			//get current status
			actuation_app_get_config(&current_action, sizeof(current_action));
			comm_app_send_event(current_action);
			break;
		}
		default:
		{
			break;
		}
	}
}

static void app_sectorization_task(void* arg)
{
	uint16_t current_angle = 0;
	bool pump_is_on = false;
	uint8_t pump_flag = 0;

	while(1)
	{
		if(main_config.sector_enabled == true)
		{
			current_angle = comm_app_get_degree();

			//TODO trocar o 4 por define
			for(uint8_t angles = 0; angles < 4; angles++)
			{
				if(main_config.sectors[angles].start_angle != 0
						&& main_config.sectors[angles].end_angle != 0)
				{
					if(current_angle >= main_config.sectors[angles].start_angle
							&& current_angle <= main_config.sectors[angles].end_angle)
					{
						if(pump_is_on == false)
						{
							ESP_LOGI(MAIN_TAG,"Pump ON (%d)", current_angle);
							actuation_app_set_pump(true);
							pump_is_on = true;
						}
					}
					else
					{
						pump_flag++;
					}
				}
			}

			if(pump_flag == 4 && pump_is_on == true)
			{
				ESP_LOGI(MAIN_TAG,"Pump OFF");
				actuation_app_set_pump(false);
				pump_is_on = false;
			}

			pump_flag = 0;
		}

		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

static void app_peak_hours_task(void* arg)
{
	bool alredy_off = false;

	struct tm rtcinfo = {0};
	time_t current_time = 0;

	pivot_actions current_action = {};
	eTaskState TaskState;

	while(1)
	{
		if(main_config.eco_mode == true)
		{
			rtc_app_get_date_time(&rtcinfo);
			current_time = ((rtcinfo.tm_hour * 3600) + (rtcinfo.tm_min * 60));

			if(main_config.start_time < main_config.end_time)
			{
				if(current_time >= main_config.start_time
				&& current_time <= main_config.end_time)
				{
					if(alredy_off == false)
					{
						// TODO talvez ler dos pinos antes de gravar
						// TODO desligar o sistema completo
						actuation_app_shutdown();
						alredy_off = true;

						TaskState = eTaskGetState( xTask_sectorization_app );

						if(TaskState == eRunning
						|| TaskState == eReady)
						{
							vTaskSuspend(xTask_sectorization_app);
						}
					}
				}
				else if(alredy_off == true)
				{
					data_app_load_actions(&current_action, sizeof(current_action));
					vTaskDelay(pdMS_TO_TICKS(500));

					actuation_app_set_config(current_action, false);
					alredy_off = false;

					TaskState = eTaskGetState( xTask_sectorization_app );

					if(TaskState == eSuspended
					|| TaskState == eBlocked)
					{
						vTaskResume(xTask_sectorization_app);
					}
				}
			}
			else
			{
				if(current_time >= main_config.end_time
				&& current_time <= main_config.start_time)
				{
					if(alredy_off == false)
					{
						// TODO talvez ler dos pinos antes de gravar
						// TODO desligar o sistema completo
						actuation_app_shutdown();
						alredy_off = true;

						TaskState = eTaskGetState( xTask_sectorization_app );

						if(TaskState == eRunning
						|| TaskState == eReady)
						{
							vTaskSuspend(xTask_sectorization_app);
						}
					}
				}
				else if(alredy_off == true)
				{
					data_app_load_actions(&current_action, sizeof(current_action));
					vTaskDelay(pdMS_TO_TICKS(500));

					actuation_app_set_config(current_action, false);
					alredy_off = false;

					TaskState = eTaskGetState( xTask_sectorization_app );

					if(TaskState == eSuspended
					|| TaskState == eBlocked)
					{
						vTaskResume(xTask_sectorization_app);
					}
				}
			}
		}
		vTaskDelay(pdMS_TO_TICKS(15000)); // 15 seconds
	}
}

/** @}*/	//main
