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
#include "gpio_actuator.h"
#define MAIN_APP_TAG			"data_app"
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

	/*---------------GPIO test--------------*/
	pivot_config pivot_test;
	pivot_test.power_state = PIVOT_ON;
	pivot_test.rotation = PIVOT_CW;
	pivot_test.watering_state = PIVOT_WET;
	pivot_test.percentimeter = 40;

	gpio_actuator_init();

	gpio_actuator_set(pivot_test);
	/*--------------------------------------*/

	ESP_LOGE(MAIN_APP_TAG, "%s, ALIVE", __func__);

	while (1)
	{
//		ESP_LOGE(MAIN_APP_TAG, "%s, ALIVE", __func__);
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
