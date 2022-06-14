/*
 * main.c
 *
 *  Created on: 31 de mai de 2022
 *      Author: brunolima
 */

/* Configurations include */
#include "project_config.h"

#define MAIN_TAG "main"

void app_main(void)
{
	ESP_LOGI(MAIN_TAG, "App main !!");

	while (1)
	{
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
