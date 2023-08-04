/*
 * system_monitoring.c
 *
 *  Created on: 4 de ago. de 2023
 *      Author: soil-dev
 */

#include "system_monitoring.h"
#include "FreeRTOS_defines.h"

static TaskHandle_t xTask_monitoring = NULL;
static app_callback system_monitoring_callback = NULL;

static void system_monitoring_task(void* arg);

static void system_monitoring_task(void* arg)
{
	while(1)
	{
		vTaskDelay(pdMS_TO_TICKS(600000)); // 10 minutes
		system_monitoring_callback("#00$", COMM_MQTT);
	}
}

void system_monitoring_start(void)
{
	xTaskCreate(&system_monitoring_task,
				SYSTEM_MONITORING_TASK_NAME,
				SYSTEM_MONITORING_TASK_SIZE,
				NULL,
				SYSTEM_MONITORING_TASK_PRIORITY,
				&xTask_monitoring);
}

void system_monitoring_stop(void)
{
	if(xTask_monitoring != NULL)
	{
		vTaskDelete(xTask_monitoring);
		xTask_monitoring = NULL;
	}
}

void system_monitoring_register_callback(const app_callback callback)
{
	system_monitoring_callback = callback;
}



