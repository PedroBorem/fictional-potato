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
 * @defgroup FREERTOS_DEFINES FreeRTOS Defines
 * @{
 */

/**
 * @brief Name of the main sectorization task.
 */
#define SECTORIZATION_TASK_NAME            	"sectorization task"

/**
 * @brief Stack size of the main sectorization task.
 */
#define SECTORIZATION_TASK_SIZE      		(configMINIMAL_STACK_SIZE * 4)

/**
 * @brief Priority of the main sectorization task.
 */
#define SECTORIZATION_TASK_PRIORITY     	(tskIDLE_PRIORITY + 4)

/**
 * @brief Name of the main peak hours task.
 */
#define MAIN_APP_TASK_2_NAME            	"peak hours task"

/**
 * @brief Stack size of the main peak hours task.
 */
#define MAIN_APP_STACK_2_SIZE           	(configMINIMAL_STACK_SIZE * 4)

/**
 * @brief Priority of the main peak hours task.
 */
#define MAIN_APP_TASK_2_PRIORITY        	(tskIDLE_PRIORITY + 3)

/**
 * @brief Name of the main scheduling task.
 */
#define SCHEDULING_TASK_NAME            	"scheduling task"

/**
 * @brief Stack size of the main scheduling task.
 */
#define SCHEDULING_TASK_SIZE				(configMINIMAL_STACK_SIZE * 10)

/**
 * @brief Priority of the main scheduling task.
 */
#define SCHEDULING_TASK_PRIORITY        	(tskIDLE_PRIORITY + 3)

/**
 * @brief Name of the data app task.
 */
#define DATA_APP_TASK_NAME              	"data app task"

/**
 * @brief Stack size of the data app task.
 */
#define DATA_APP_STACK_SIZE             	(configMINIMAL_STACK_SIZE * 6)

/**
 * @brief Priority of the data app task.
 */
#define DATA_APP_TASK_PRIORITY         		(tskIDLE_PRIORITY + 2)

/**
 * @brief Name of the comm app task.
 */
#define COMM_APP_TASK_NAME              	"comm app task"

/**
 * @brief Stack size of the comm app task.
 */
#define COMM_APP_STACK_SIZE             	(configMINIMAL_STACK_SIZE * 10)

/**
 * @brief Priority of the comm app task.
 */
#define COMM_APP_TASK_PRIORITY          	(tskIDLE_PRIORITY + 2)

/**
 * @brief Name of the actuation app task.
 */
#define ACTUATION_APP_TASK_NAME         	"actuation app task"

/**
 * @brief Stack size of the actuation app task.
 */
#define ACTUATION_APP_STACK_SIZE        	(configMINIMAL_STACK_SIZE * 6)

/**
 * @brief Priority of the actuation app task.
 */
#define ACTUATION_APP_TASK_PRIORITY     	(tskIDLE_PRIORITY + 3)

/**
 * @brief Name of the actuator check task.
 */
#define ACTUATOR_CHECK_TASK_NAME        	"actuator check task"

/**
 * @brief Stack size of the actuator check task.
 */
#define ACTUATOR_CHECK_STACK_SIZE       	(configMINIMAL_STACK_SIZE * 6)

/**
 * @brief Priority of the actuator check task.
 */
#define ACTUATOR_CHECK_TASK_PRIORITY    	(tskIDLE_PRIORITY + 3)

/**
 * @brief Name of the actuator percent task.
 */
#define ACTUATOR_PERCENT_TASK_NAME      	"actuator percent task"

/**
 * @brief Stack size of the actuator percent task.
 */
#define ACTUATOR_PERCENT_STACK_SIZE     	(configMINIMAL_STACK_SIZE * 6)

/**
 * @brief Priority of the actuator percent task.
 */
#define ACTUATOR_PERCENT_TASK_PRIORITY  	(tskIDLE_PRIORITY + 3)

/**
 * @brief Name of the GPRS UART task.
 */
#define GPRS_UART_TASK_NAME             	"GPRS UART task"

/**
 * @brief Stack size of the GPRS UART task.
 */
#define GPRS_UART_STACK_SIZE            	(configMINIMAL_STACK_SIZE * 10)

/**
 * @brief Priority of the GPRS UART task.
 */
#define GPRS_UART_TASK_PRIORITY         	(tskIDLE_PRIORITY + 12)

/**
 * @brief Name of the RF UART task.
 */
#define RF_UART_TASK_NAME               	"RF UART task"

/**
 * @brief Stack size of the RF UART task.
 */
#define RF_UART_STACK_SIZE              	(configMINIMAL_STACK_SIZE * 10)

/**
 * @brief Priority of the RF UART task.
 */
#define RF_UART_TASK_PRIORITY           	(tskIDLE_PRIORITY + 12)

/**
 * @brief Name of the Wi-Fi app task.
 */
#define WIFI_APP_TASK_NAME				"wifi app task"

/**
 * @brief Stack size of the Wi-Fi app task.
 */
#define WIFI_APP_STACK_SIZE				(configMINIMAL_STACK_SIZE * 4)

/**
 * @brief Priority of the Wi-Fi app task.
 */
#define WIFI_APP_TASK_PRIORITY			(tskIDLE_PRIORITY + 8)

/**
 * @brief Name of the system monitoring task.
 */
#define SYSTEM_MONITORING_TASK_NAME		"monitoring task"

/**
 * @brief Stack size of the system monitoring task.
 */
#define SYSTEM_MONITORING_TASK_SIZE		(configMINIMAL_STACK_SIZE * 6)

/**
 * @brief Priority of the system monitoring task.
 */
#define SYSTEM_MONITORING_TASK_PRIORITY	(tskIDLE_PRIORITY + 2)

/**
 * @brief Name of the Eco Mode task.
 */
#define ECO_MODE_TASK_NAME				"eco mode task"

/**
 * @brief Stack size of the Eco Mode task.
 */
#define ECO_MODE_TASK_SIZE				(configMINIMAL_STACK_SIZE * 4)

/**
 * @brief Priority of the Eco Mode task.
 */
#define ECO_MODE_TASK_PRIORITY			(tskIDLE_PRIORITY + 2)

/**
 * @brief Name of the Pluviometer UART task.
 */
#define PLUV_TASK_NAME              	"Rainfall_Task"

/**
 * @brief Stack size of the Pluviometer UART task.
 */
#define PLUV_STACK_SIZE             	(configMINIMAL_STACK_SIZE * 10) // 2048 bytes

/**
 * @brief Priority of the Pluviometer UART task.
 */
#define PLUV_TASK_PRIORITY          	(tskIDLE_PRIORITY + 5)

/** @} */ // end of FREERTOS_DEFINES group

#endif /* COMPONENTS_UTILS_INCLUDE_FREERTOS_DEFINES_H_ */
