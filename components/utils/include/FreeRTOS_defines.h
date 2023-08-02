/**
 * @file FreeRTOS_defines.h
 * @date July 11, 2023
 * @brief Defines for FreeRTOS tasks and priorities.
 *
 * This file contains defines for FreeRTOS tasks and priorities used in the application.
 */

#ifndef COMPONENTS_UTILS_INCLUDE_FREERTOS_DEFINES_H_
#define COMPONENTS_UTILS_INCLUDE_FREERTOS_DEFINES_H_

/* include FreeRTOS */
#include "freertos/FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

/**
 * @def SECTORIZATION_TASK_NAME
 * @brief Name of the main sectorization task.
 */
#define SECTORIZATION_TASK_NAME            	"sectorization task"

/**
 * @def SECTORIZATION_TASK_SIZE
 * @brief Stack size of the main sectorization task.
 */
#define SECTORIZATION_TASK_SIZE      		(configMINIMAL_STACK_SIZE * 4)

/**
 * @def SECTORIZATION_TASK_PRIORITY
 * @brief Priority of the main sectorization task.
 */
#define SECTORIZATION_TASK_PRIORITY     	(tskIDLE_PRIORITY + 4)

/**
 * @def MAIN_APP_TASK_2_NAME
 * @brief Name of the main peak hours task.
 */
#define MAIN_APP_TASK_2_NAME            	"peak hours task"

/**
 * @def MAIN_APP_STACK_2_SIZE
 * @brief Stack size of the main peak hours task.
 */
#define MAIN_APP_STACK_2_SIZE           	(configMINIMAL_STACK_SIZE * 4)

/**
 * @def MAIN_APP_TASK_2_PRIORITY
 * @brief Priority of the main peak hours task.
 */
#define MAIN_APP_TASK_2_PRIORITY        	(tskIDLE_PRIORITY + 3)

/**
 * @def SCHEDULING_TASK_NAME
 * @brief Name of the main scheduling task.
 */
#define SCHEDULING_TASK_NAME            	"scheduling task"

/**
 * @def MAIN_APP_STACK_3_SIZE
 * @brief Stack size of the main scheduling task.
 */
#define SCHEDULING_TASK_SIZE				(configMINIMAL_STACK_SIZE * 6)

/**
 * @def SCHEDULING_TASK_PRIORITY
 * @brief Priority of the main scheduling task.
 */
#define SCHEDULING_TASK_PRIORITY        	(tskIDLE_PRIORITY + 3)

/**
 * @def DATA_APP_TASK_NAME
 * @brief Name of the data app task.
 */
#define DATA_APP_TASK_NAME              	"data app task"

/**
 * @def DATA_APP_STACK_SIZE
 * @brief Stack size of the data app task.
 */
#define DATA_APP_STACK_SIZE             	(configMINIMAL_STACK_SIZE * 6)

/**
 * @def DATA_APP_TASK_PRIORITY
 * @brief Priority of the data app task.
 */
#define DATA_APP_TASK_PRIORITY         		(tskIDLE_PRIORITY + 2)

/**
 * @def COMM_APP_TASK_NAME
 * @brief Name of the comm app task.
 */
#define COMM_APP_TASK_NAME              	"comm app task"

/**
 * @def COMM_APP_STACK_SIZE
 * @brief Stack size of the comm app task.
 */
#define COMM_APP_STACK_SIZE             	(configMINIMAL_STACK_SIZE * 10)

/**
 * @def COMM_APP_TASK_PRIORITY
 * @brief Priority of the comm app task.
 */
#define COMM_APP_TASK_PRIORITY          	(tskIDLE_PRIORITY + 2)

/**
 * @def ACTUATION_APP_TASK_NAME
 * @brief Name of the actuation app task.
 */
#define ACTUATION_APP_TASK_NAME         	"actuation app task"

/**
 * @def ACTUATION_APP_STACK_SIZE
 * @brief Stack size of the actuation app task.
 */
#define ACTUATION_APP_STACK_SIZE        	(configMINIMAL_STACK_SIZE * 6)

/**
 * @def ACTUATION_APP_TASK_PRIORITY
 * @brief Priority of the actuation app task.
 */
#define ACTUATION_APP_TASK_PRIORITY     	(tskIDLE_PRIORITY + 3)

/**
 * @def ACTUATOR_CHECK_TASK_NAME
 * @brief Name of the actuator check task.
 */
#define ACTUATOR_CHECK_TASK_NAME        	"actuator check task"

/**
 * @def ACTUATOR_CHECK_STACK_SIZE
 * @brief Stack size of the actuator check task.
 */
#define ACTUATOR_CHECK_STACK_SIZE       	(configMINIMAL_STACK_SIZE * 6)

/**
 * @def ACTUATOR_CHECK_TASK_PRIORITY
 * @brief Priority of the actuator check task.
 */
#define ACTUATOR_CHECK_TASK_PRIORITY    	(tskIDLE_PRIORITY + 3)

/**
 * @def ACTUATOR_PERCENT_TASK_NAME
 * @brief Name of the actuator percent task.
 */
#define ACTUATOR_PERCENT_TASK_NAME      	"actuator percent task"

/**
 * @def ACTUATOR_PERCENT_STACK_SIZE
 * @brief Stack size of the actuator percent task.
 */
#define ACTUATOR_PERCENT_STACK_SIZE     	(configMINIMAL_STACK_SIZE * 6)

/**
 * @def ACTUATOR_PERCENT_TASK_PRIORITY
 * @brief Priority of the actuator percent task.
 */
#define ACTUATOR_PERCENT_TASK_PRIORITY  	(tskIDLE_PRIORITY + 3)

/**
 * @def GPRS_UART_TASK_NAME
 * @brief Name of the GPRS UART task.
 */
#define GPRS_UART_TASK_NAME             	"GPRS UART task"

/**
 * @def GPRS_UART_STACK_SIZE
 * @brief Stack size of the GPRS UART task.
 */
#define GPRS_UART_STACK_SIZE            	(configMINIMAL_STACK_SIZE * 10)

/**
 * @def GPRS_UART_TASK_PRIORITY
 * @brief Priority of the GPRS UART task.
 */
#define GPRS_UART_TASK_PRIORITY         	(tskIDLE_PRIORITY + 12)

/**
 * @def RF_UART_TASK_NAME
 * @brief Name of the RF UART task.
 */
#define RF_UART_TASK_NAME               	"RF UART task"

/**
 * @def RF_UART_STACK_SIZE
 * @brief Stack size of the RF UART task.
 */
#define RF_UART_STACK_SIZE              	(configMINIMAL_STACK_SIZE * 10)

/**
 * @def RF_UART_TASK_PRIORITY
 * @brief Priority of the RF UART task.
 */
#define RF_UART_TASK_PRIORITY           	(tskIDLE_PRIORITY + 12)

#define WIFI_APP_TASK_NAME				"wifi app task"
#define WIFI_APP_STACK_SIZE				( configMINIMAL_STACK_SIZE * 4 )
#define WIFI_APP_TASK_PRIORITY			( tskIDLE_PRIORITY + 8 )

#endif /* COMPONENTS_UTILS_INCLUDE_FREERTOS_DEFINES_H_ */
