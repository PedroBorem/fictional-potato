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

/**\addtogroup main
 * @{
 *
 */

#define MAIN_TAG "main"

/* Private function prototype ------------------------------------ */
static bool app_init(void);
static void app_main_call(app_call_states state,const void* buffer);

/**
 * @brief	main class
 *
 */
void app_main(void)
{
	pivot_config app_config = {};

	ESP_LOGI(MAIN_TAG,"starting the system ...");
	assert(app_init());

	data_app_load_config(app_config, sizeof(app_config));
	//ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	vTaskDelay(pdMS_TO_TICKS(1000));
	actuation_app_set_config(app_config);

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

	ret &= data_app_init(&app_main_call);
	ret &= comm_app_init(&app_main_call);
	ret &= actuation_app_init(&app_main_call);

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
			pivot_config config = {};
			memcpy(&config, buffer, sizeof(config));

			ret = data_app_save_config(config, sizeof(config));
			if(ret == true)
			{
				comm_app_send_event(config);
				actuation_app_set_config(config);
			}

			break;
		}
		case CALL_READ_STATUS: // if you receive 000-000
		{
			pivot_config config_send = {};

			actuation_app_get_config(config_send, sizeof(config_send));
			comm_app_send_event(config_send);

			break;
		}
		default:
		{
			break;
		}
	}
}

/** @}*/	//main
