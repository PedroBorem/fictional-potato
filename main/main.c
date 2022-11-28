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
#include "data_app.h"
#include "comm_app.h"
#include "actuation_app.h"
#include "rtc_app.h"

/**\addtogroup main
 * @{
 *
 */

#define MAIN_TAG "main"

/* Private variables ------------------------------------ */
static TaskHandle_t xTask_app = NULL;


/* Private function prototype ------------------------------------ */
static bool app_init(void);
static void app_main_call(app_call_states state,const void* buffer);
static void app_sectorization_task(void* arg);

/**
 * @brief	main class
 *
 */
void app_main(void)
{
	uint16_t angles[4] = {};
	ESP_LOGI(MAIN_TAG,"starting the system ...");
	assert(app_init());

	//const pivot_config config_in = {PIVOT_OFF, PIVOT_CW, PIVOT_DRY, 0};

	//actuation_app_set_config(config, true);

	// mock input
	angles[0] = 25; //initial 1
	angles[1] = 80; //final 1

	angles[2] = 180; //initial 1
	angles[3] = 250; //final 1

	// create sectorization task
	xTaskCreate(&app_sectorization_task,
				MAIN_APP_TASK_NAME,
				MAIN_APP_STACK_SIZE,
				angles,
				MAIN_APP_TASK_PRIORITY,
				&xTask_app);

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
	ret &= data_app_init(&app_main_call);
	ret &= comm_app_init(&app_main_call);

	return ret;
}

/**
 * @brief	callback from secondary applications to the main
 * @param	state - [in]: reason why a callback was triggered
 */
static void app_main_call(app_call_states state,const void* buffer)
{
	bool ret = false;

	switch(state)
	{
		case CALL_LOAD_CONFIG:
		{
			// TODO: notify give main task
			break;
		}
		case CALL_NEW_CONFIG:
		{
			pivot_config new_config = {};
			memcpy(&new_config, buffer, sizeof(new_config));

			ret = data_app_save_config(new_config, sizeof(new_config));
			if(ret == true)
			{
				actuation_app_set_config(new_config, false);
				comm_app_send_event(new_config);
			}
			else if(new_config.rotation == PIVOT_UNKNOWN)
			{
				comm_app_send_event(new_config);
			}

			break;
		}
		case CALL_MANUAL_PIVOT:
		{
			pivot_config manual_config = {};
			memcpy(&manual_config, buffer, sizeof(manual_config));

			actuation_app_set_config(manual_config, true);
			comm_app_send_event(manual_config);

			break;
		}
		case CALL_READ_STATUS: // if you receive 000-000
		{
			pivot_config config_send = {};

			actuation_app_get_config(&config_send, sizeof(config_send));
			comm_app_send_event(config_send);

			break;
		}
		case CALL_OFF_PIVOT:
		{
			pivot_config config_off = {
			   .power_state = PIVOT_OFF,
			   .rotation = PIVOT_CW,
			   .watering_state = PIVOT_DRY,
			   .percentimeter = 0
			};

			//set off configuration
			actuation_app_set_config(config_off, false);
			vTaskDelay(pdMS_TO_TICKS(2000));

			//get current status
			actuation_app_get_config(&config_off, sizeof(config_off));
			comm_app_send_event(config_off);
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
	uint16_t angles[4] = {};
	uint16_t current_angle = 0;
	bool pump_is_on = false;
	memcpy(angles, arg, sizeof(angles));

	while(1)
	{
		current_angle = comm_app_get_degree();
		if(current_angle >= angles[0] && current_angle <= angles[1])
		{
			if(pump_is_on == false)
			{
				ESP_LOGI(MAIN_TAG,"Pump ON (%d)", current_angle);
				actuation_app_set_pump(true);
				pump_is_on = true;
			}
		}
		else if(current_angle >= angles[2] && current_angle <= angles[3])
		{
			if(pump_is_on == false)
			{
				ESP_LOGI(MAIN_TAG,"Pump ON (%d)", current_angle);
				actuation_app_set_pump(true);
				pump_is_on = true;
			}
		}
		else if(pump_is_on == true)
		{
			ESP_LOGI(MAIN_TAG,"Pump OFF");
			actuation_app_set_pump(false);
			pump_is_on = false;
		}

		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

/** @}*/	//main
