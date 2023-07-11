/*
 * FreeRTOS_defines.h
 *
 *  Created on: 11 de jul. de 2023
 *      Author: bruno
 */

#ifndef COMPONENTS_UTILS_INCLUDE_FREERTOS_DEFINES_H_
#define COMPONENTS_UTILS_INCLUDE_FREERTOS_DEFINES_H_

/* include FreeRTOS */
#include "freertos/FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#define MAIN_APP_TASK_1_NAME			"main sectorization task"
#define MAIN_APP_STACK_1_SIZE			( configMINIMAL_STACK_SIZE * 4 )
#define MAIN_APP_TASK_1_PRIORITY		( tskIDLE_PRIORITY + 4 )

#define MAIN_APP_TASK_2_NAME			"main peak hours task"
#define MAIN_APP_STACK_2_SIZE			( configMINIMAL_STACK_SIZE * 4 )
#define MAIN_APP_TASK_2_PRIORITY		( tskIDLE_PRIORITY + 3 )

#define MAIN_APP_TASK_3_NAME			"main scheduling task"
#define MAIN_APP_STACK_3_SIZE			( configMINIMAL_STACK_SIZE * 6 )
#define MAIN_APP_TASK_3_PRIORITY		( tskIDLE_PRIORITY + 3 )

#define DATA_APP_TASK_NAME				"data app task"
#define DATA_APP_STACK_SIZE				( configMINIMAL_STACK_SIZE * 6 )
#define DATA_APP_TASK_PRIORITY			( tskIDLE_PRIORITY + 2 )

#define COMM_APP_TASK_NAME				"comm app task"
#define COMM_APP_STACK_SIZE				( configMINIMAL_STACK_SIZE * 10 )
#define COMM_APP_TASK_PRIORITY			( tskIDLE_PRIORITY + 2 )

#define ACTUATION_APP_TASK_NAME			"actuation app task"
#define ACTUATION_APP_STACK_SIZE		( configMINIMAL_STACK_SIZE * 6 )
#define ACTUATION_APP_TASK_PRIORITY		( tskIDLE_PRIORITY + 3 )

#define ACTUATOR_CHECK_TASK_NAME		"actuator check task"
#define ACTUATOR_CHECK_STACK_SIZE		( configMINIMAL_STACK_SIZE * 6 )
#define ACTUATOR_CHECK_TASK_PRIORITY	( tskIDLE_PRIORITY + 3 )

#define ACTUATOR_PERCENT_TASK_NAME		"actuator percent task"
#define ACTUATOR_PERCENT_STACK_SIZE		( configMINIMAL_STACK_SIZE * 6 )
#define ACTUATOR_PERCENT_TASK_PRIORITY	( tskIDLE_PRIORITY + 3 )

#define GPRS_UART_TASK_NAME				"gprs uart task"
#define GPRS_UART_STACK_SIZE			( configMINIMAL_STACK_SIZE * 10 )
#define GPRS_UART_TASK_PRIORITY			( tskIDLE_PRIORITY + 12 )

#define RF_UART_TASK_NAME				"rf uart task"
#define RF_UART_STACK_SIZE				( configMINIMAL_STACK_SIZE * 10 )
#define RF_UART_TASK_PRIORITY			( tskIDLE_PRIORITY + 12 )

#define WIFI_APP_TASK_NAME				"wifi app task"
#define WIFI_APP_STACK_SIZE				( configMINIMAL_STACK_SIZE * 4 )
#define WIFI_APP_TASK_PRIORITY			( tskIDLE_PRIORITY + 8 )

#endif /* COMPONENTS_UTILS_INCLUDE_FREERTOS_DEFINES_H_ */
