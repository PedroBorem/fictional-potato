/*
 * project_config.h
 *
 *  Created on: 31 de mai de 2022
 *      Author: bruno
 */

#ifndef COMPONENTS_UTILS_INCLUDE_PROJECT_CONFIG_H_
#define COMPONENTS_UTILS_INCLUDE_PROJECT_CONFIG_H_

/**
 * @file project_config.h
 * @date June 15, 2022
 * @brief general project settings
*/

/* Configuration include*/
#include "sdkconfig.h"

/* C base */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/param.h>

/* include FreeRTOS */
#include "freertos/FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

/* include components */
#include "log.h"

/* include ESP32 modules */
#include "esp_err.h"

/* Some scripts may not require all the parameters passed. Use this to avoid
 * compiler warnings about unused variables. */
#define UNUSED(x)               (void)(sizeof(x))


/* Public definitions ******************************************************/

/**
 *	Main Callback request
 *
 */
typedef enum
{
	CALL_LOAD_ACTION = 1,	/*!< Configuration read request*/
	CALL_SAVE_ACTION,
	CALL_SAVE_CONFIG,
	CALL_READ_ACTION,
	CALL_MANUAL_PIVOT,
	CALL_OFF_PIVOT
}app_call_states;

/**
 *	Pivot configuration states
 *
 */
typedef enum
{
	PIVOT_ON = 1,			/*!< Pivot on*/
	PIVOT_OFF = 2,			/*!< Pivot off*/
	PIVOT_CW = 3,			/*!< Pivot in ClockWise mode (advanced)*/
	PIVOT_CCW = 4,			/*!< Pivot in Counter ClockWise mode (reverse)*/
	PIVOT_DRY = 5,			/*!< Irrigation off*/
	PIVOT_WET = 6,			/*!< Irrigation on*/
	PIVOT_PRESSURIZING = 7,
	PIVOT_UNKNOWN = 0	    /*!< Value not obtained yet*/
}pivot_states;

/**
 *	Pivot configuration parameters
 *
 */
typedef	struct __attribute__((__packed__))
{
	uint8_t power_state;	/*!< PIVOT_ON or PIVOT_OFF*/
	uint8_t rotation;		/*!< PIVOT_CW or PIVOT_CCW*/
	uint8_t watering_state;	/*!< PIVOT_DRY or PIVOT_WET*/ // @suppress("Type cannot be resolved")
	uint8_t percentimeter;	/*!< Value from 0 to 100*/
}pivot_config;

/**
 * @brief: function used with return to main application class
 *
 */
typedef void (*app_callback)(app_call_states state, const void* buffer);

/**\addtogroup FreeRTOS
 * @{
 *
 */

/**\addtogroup Task_Definitions
 * @{
 *
 */
#define MAIN_APP_TASK_1_NAME			"main sectorization task"
#define MAIN_APP_STACK_1_SIZE			( configMINIMAL_STACK_SIZE * 4 )
#define MAIN_APP_TASK_1_PRIORITY		( tskIDLE_PRIORITY + 4 )

#define MAIN_APP_TASK_2_NAME			"main peak hours task"
#define MAIN_APP_STACK_2_SIZE			( configMINIMAL_STACK_SIZE * 4 )
#define MAIN_APP_TASK_2_PRIORITY		( tskIDLE_PRIORITY + 2 )

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
#define ACTUATOR_CHECK_STACK_SIZE		( configMINIMAL_STACK_SIZE * 4 )
#define ACTUATOR_CHECK_TASK_PRIORITY	( tskIDLE_PRIORITY + 3 )

#define GPRS_UART_TASK_NAME				"gprs uart task"
#define GPRS_UART_STACK_SIZE			( configMINIMAL_STACK_SIZE * 10 )
#define GPRS_UART_TASK_PRIORITY			( tskIDLE_PRIORITY + 12 )

#define RF_UART_TASK_NAME				"rf uart task"
#define RF_UART_STACK_SIZE				( configMINIMAL_STACK_SIZE * 10 )
#define RF_UART_TASK_PRIORITY			( tskIDLE_PRIORITY + 12 )

/**@}*/ 	//FreeRTOS
/** @}*/	//Task_Definitions

#endif /* COMPONENTS_UTILS_INCLUDE_PROJECT_CONFIG_H_ */
