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

#include "rf_module.h"

/**\addtogroup main
 * @{
 *
 */

#define MAIN_TAG "main"

/* Private function prototype ------------------------------------ */
static bool app_init(void);
static void app_main_call(app_call_states state, void* buffer);

/**
 * @brief	main class
 *
 */
void app_main(void)
{
	ESP_LOGI(MAIN_TAG,"starting the system ...");
	assert(app_init());

	rf_module_init();
	while (1)
	{
		vTaskDelay(pdMS_TO_TICKS(1000));
		ESP_LOGI(MAIN_TAG,"starting the system ...");
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

	return ret;
}

/**
 * @brief	callback from secondary applications to the main
 * @param	state - [in]: reason why a callback was triggered
 */
static void app_main_call(app_call_states state, void* buffer)
{
	switch(state)
	{
		case CALL_LOAD_CONFIG:
		{
			// TODO: notify give main task
			break;
		}
		case CALL_NEW_CONGIG:
		{
			pivot_config config = {};
			memcpy(&config, buffer, sizeof(config));
			data_app_save_config(config, sizeof(config));
			break;
		}
		default:
		{
			break;
		}
	}
}

/** @}*/	//main
