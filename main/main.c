/*
 * main.c
 *
 *  Created on: 31 de mai de 2022
 *      Author: brunolima
 */

/* Applications include */
#include "FreeRTOS_defines.h"
#include "system_manager.h"

/**
 * @brief	main class
 *
 */
void app_main(void)
{
	system_manager_init();
	while (1)
	{
		//system_manager_callback("#01-soil_1-361-080$", COMM_REMOTE);
		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

