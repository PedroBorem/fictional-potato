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
	ESP_LOGI(MAIN_TAG,"starting the system ...");
	assert(app_init());

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
			}

			break;
		}
		case CALL_READ_STATUS: // if you receive 000-000
		{
			// TODO: pedir para a leitura do o status.

			// TODO : Remover esse valor simulado
			pivot_config config_send = {};
			config_send.power_state = PIVOT_ON;
			config_send.rotation = PIVOT_CW;
			config_send.watering_state = PIVOT_DRY;
			config_send.percentimeter = 100;

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
