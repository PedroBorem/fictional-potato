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

/**\addtogroup main
 * @{
 *
 */

#define MAIN_TAG "main"

/* Private function prototype ------------------------------------ */
static bool app_init(void);
static void app_main_call(app_call_states state);

/**
 * @brief	main class
 *
 */
void app_main(void)
{
	ESP_LOGI(MAIN_TAG,"starting the system ...");
	assert(app_init());

#ifndef REMOVE_THIS
	uint8_t percent = 0;
	pivot_config config = {};

	while (1)
	{
		vTaskDelay(pdMS_TO_TICKS(5000));
		config.power_state = PIVOT_ON;
		config.advance_mode = PIVOT_ADVANCE;
		config.watering_state = PIVOT_DRY;
		config.percentimeter = percent;
		percent = (rand()%100);

		vTaskDelay(pdMS_TO_TICKS(1000));
		data_app_save_config(config, sizeof(config));
		vTaskDelay(pdMS_TO_TICKS(1000));

#else

	while (1)
	{
#endif

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

	return ret;
}

/**
 * @brief	callback from secondary applications to the main
 * @param	state - [in]: reason why a callback was triggered
 */
static void app_main_call(app_call_states state)
{
	switch(state)
	{
		case CALL_LOAD_CONFIG:
		{
			// TODO: notify give main task
			break;
		}
		default:
		{
			break;
		}
	}
}

/** @}*/	//main
