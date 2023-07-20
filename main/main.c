/*
 * main.c
 *
 *  Created on: 31 de mai de 2022
 *      Author: brunolima
 */

/* Applications include */
#include "rtc_app.h"
#include "project_config.h"
#include "FreeRTOS_defines.h"
#include "log.h"
#include "system_manager.h"

/* C base */
#include <string.h>

#include "actuation_app.h"
#include "comm_app.h"
#include "data_app.h"


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

