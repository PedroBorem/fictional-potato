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
static TaskHandle_t xTask_scheduling_app = NULL;

// represents the initial coverage angle of the pivot
static uint16_t app_start_angle = 0xFFFF;
static pivot_config main_config = {};

static pivot_scheduling_date main_scheduling_date[SCHEDULING_MAX_VALUE] = {};
static pivot_scheduling_angle main_scheduling_angle[SCHEDULING_MAX_VALUE] = {};
static bool scheduling_date_status[SCHEDULING_MAX_VALUE] = {};
static bool scheduling_angle_status[SCHEDULING_MAX_VALUE] = {};

/* Private function prototype ------------------------------------ */
static bool app_init(void);
static void app_main_call(app_call_states state, void* buffer);
static void app_sectorization_task(void* arg);
static void app_peak_hours_task(void* arg);
static void app_scheduling_task(void* arg);

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

	// set HTTP parameters
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

	// create peak hours task
	xTaskCreate(&app_scheduling_task,
				MAIN_APP_TASK_3_NAME,
				MAIN_APP_STACK_3_SIZE,
				NULL,
				MAIN_APP_TASK_3_PRIORITY,
				&xTask_scheduling_app);

	memcpy(&main_config, &current_config, sizeof(main_config));

	while (1)
	{
		// get start angle
		if(comm_app_get_degree() == 0xFFFF)
		{
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
		else
		{
			if(app_start_angle == 0xFFFF)
			{
				app_start_angle = comm_app_get_degree();
			}
			vTaskDelay(pdMS_TO_TICKS(10000));
		}
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
static void app_main_call(app_call_states state, void* buffer)
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
		case CALL_SAVE_SCHEDULE_DATE:
		{
			pivot_scheduling_date scheduling_date[SCHEDULING_MAX_VALUE] = {};
			data_app_load_scheduling(data_scheduling_date, scheduling_date, sizeof(scheduling_date));

			for(uint8_t position = 0; position < SCHEDULING_MAX_VALUE; position++)
			{
				if(strcmp(scheduling_date[position].scheduling_id, "") == 0)
				{
					memcpy(&scheduling_date[position], buffer, sizeof(scheduling_date[position]));
					data_app_save_scheduling(data_scheduling_date, scheduling_date, sizeof(scheduling_date));
					memcpy(main_scheduling_date, scheduling_date, sizeof(main_scheduling_date));

					ESP_LOGI(MAIN_TAG, "Save schedule id : %s", scheduling_date[position].scheduling_id);
					break;
				}
			}

			break;
		}
		case CALL_LOAD_SCHEDULE_DATE:
		{
			for(uint8_t position = 0; position < SCHEDULING_MAX_VALUE ; position++)
			{
				memcpy(&main_scheduling_date[position].is_running, &scheduling_date_status[position],
						sizeof(main_scheduling_date[position].is_running));
			}
			memcpy(buffer, main_scheduling_date, sizeof(main_scheduling_date));

			break;
		}
		case CALL_SAVE_SCHEDULE_ANGLE:
		{
			pivot_scheduling_angle scheduling_angle[SCHEDULING_MAX_VALUE] = {};
			data_app_load_scheduling(data_scheduling_angle, scheduling_angle, sizeof(scheduling_angle));

			for(uint8_t position = 0; position < SCHEDULING_MAX_VALUE; position++)
			{
				if(strcmp(scheduling_angle[position].scheduling_id, "") == 0)
				{
					memcpy(&scheduling_angle[position], buffer, sizeof(scheduling_angle[position]));
					data_app_save_scheduling(data_scheduling_angle, scheduling_angle, sizeof(scheduling_angle));
					memcpy(main_scheduling_angle, scheduling_angle, sizeof(main_scheduling_angle));

					ESP_LOGI(MAIN_TAG, "Save schedule id : %s", scheduling_angle[position].scheduling_id);
					break;
				}
			}
			break;
		}
		case CALL_LOAD_SCHEDULE_ANGLE:
		{
			for(uint8_t position = 0; position < SCHEDULING_MAX_VALUE ; position++)
			{
				memcpy(&main_scheduling_angle[position].is_running, &main_scheduling_angle[position],
						sizeof(main_scheduling_angle[position].is_running));
			}
			memcpy(buffer, main_scheduling_angle, sizeof(main_scheduling_angle));

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

static void app_scheduling_task(void* arg)
{
	time_t scheduling_timestamp_now = 0;
	uint16_t scheduling_angle = comm_app_get_degree();

	data_app_load_scheduling(data_scheduling_date, main_scheduling_date, sizeof(main_scheduling_date));
	data_app_load_scheduling(data_scheduling_angle, main_scheduling_angle, sizeof(main_scheduling_angle));

	memset(scheduling_date_status, false, sizeof(scheduling_date_status));
	memset(scheduling_angle_status, false, sizeof(scheduling_angle_status));

	while(1)
	{
		scheduling_timestamp_now = rtc_app_get_timestamp();

		// date analysis
		for(uint8_t date_position = 0; date_position < SCHEDULING_MAX_VALUE; date_position++)
		{
			if(scheduling_timestamp_now >= main_scheduling_date[date_position].start_date
			&& scheduling_timestamp_now <= main_scheduling_date[date_position].end_date
			&& strcmp(main_scheduling_date[date_position].scheduling_id,"") > 0)
			{
				if(scheduling_date_status[date_position] == false)
				{
					scheduling_date_status[date_position] = true;
					app_main_call(CALL_SAVE_ACTION, &main_scheduling_date[date_position].acionts);
					ESP_LOGI(MAIN_TAG, "processing schedule by date id : %s", main_scheduling_date[date_position].scheduling_id);
				}
			}
			else if(scheduling_timestamp_now > main_scheduling_date[date_position].end_date
			&& main_scheduling_date[date_position].end_date != 0)
			{
				data_app_delete_scheduling(data_scheduling_date, main_scheduling_date[date_position].scheduling_id);
				if(scheduling_date_status[date_position] == true)
				{
					scheduling_date_status[date_position] = false;
					app_main_call(CALL_OFF_PIVOT, NULL);
				}
			}
		}

		// angle analysis
		for(uint8_t angle_position = 0; angle_position < SCHEDULING_MAX_VALUE; angle_position++)
		{
			if(scheduling_timestamp_now >= main_scheduling_angle[angle_position].start_date
			&& strcmp(main_scheduling_angle[angle_position].scheduling_id,"") > 0)
			{
				// get current angle
				scheduling_angle = comm_app_get_degree();

				// TODO : se o pivot estiver parado? devemos ligar o motor?

				if(main_scheduling_angle[angle_position].start_angle < main_scheduling_angle[angle_position].end_angle)
				{
					if(scheduling_angle >= main_scheduling_angle[angle_position].start_angle
					&& scheduling_angle <= main_scheduling_angle[angle_position].end_angle)
					{
						if(scheduling_angle_status[angle_position] == false)
						{
							scheduling_angle_status[angle_position] = true;
							app_main_call(CALL_SAVE_ACTION, &main_scheduling_angle[angle_position].acionts);
							ESP_LOGI(MAIN_TAG, "processing schedule by angle id : %s",
									main_scheduling_angle[angle_position].scheduling_id);
						}
					}
					else if(scheduling_angle_status[angle_position] == true)
					{
						scheduling_angle_status[angle_position] = false;
						app_main_call(CALL_OFF_PIVOT, NULL);
						data_app_delete_scheduling(data_scheduling_angle, main_scheduling_date[angle_position].scheduling_id);
					}
				}
				else
				{
					if(scheduling_angle <= main_scheduling_angle[angle_position].start_angle
					&& scheduling_angle >= main_scheduling_angle[angle_position].end_angle)
					{
						if(scheduling_angle_status[angle_position] == false)
						{
							scheduling_angle_status[angle_position] = true;
							app_main_call(CALL_SAVE_ACTION, &main_scheduling_angle[angle_position].acionts);
							ESP_LOGI(MAIN_TAG, "processing schedule by angle id : %s",
									main_scheduling_angle[angle_position].scheduling_id);
						}
					}
					else if(scheduling_angle_status[angle_position] == true)
					{
						scheduling_angle_status[angle_position] = false;
						app_main_call(CALL_OFF_PIVOT, NULL);
						data_app_delete_scheduling(data_scheduling_angle, main_scheduling_date[angle_position].scheduling_id);
					}
				}
			}
		}
		vTaskDelay(pdMS_TO_TICKS(5000)); // 10 seconds
	}
}

/** @}*/	//main
