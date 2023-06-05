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

#define SCHEDULING_MAX_VALUE	(10)

#define HISTORY_MAX_VALUE	(20)

/* Public definitions ******************************************************/

/**
 *	Packet Identifier
 *
 */
typedef enum
{
	IDP_0 = 0,			/*!< Read actions*/
	IDP_1 = 1,			/*!< Save actions*/
	IDP_2 = 2,			/*!< Scheduling type 1 (On by date and off by date)*/
	IDP_3 = 3,			/*!< Schedule type 2 (Turn on by date and turn off by angle)*/
	IDP_4 = 4,			/*!< Schedule type 3 (Only turns off by date)*/
	IDP_5 = 5,			/*!< Schedule type 4 (Only turns off by angle)*/
	IDP_6 = 6,			/*!< Schedule delete*/
	IDP_7 = 7,			/*!< Pivot off*/
	IDP_INVALID = 255,			/*!< Schedule delete*/
}idp_type;

/**
 *	Pivot actions states
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
 *	Pivot actions parameters
 *
 */
typedef	struct __attribute__((__packed__))
{
	uint8_t power_state;	/*!< PIVOT_ON or PIVOT_OFF*/
	uint8_t rotation;		/*!< PIVOT_CW or PIVOT_CCW*/
	uint8_t watering_state;	/*!< PIVOT_DRY or PIVOT_WET*/ // @suppress("Type cannot be resolved")
	uint8_t percentimeter;	/*!< Value from 0 to 100*/
}pivot_actions;

/**
 *
 *
 */
typedef enum
{
	CONTACTOR_NA = 1,
	CONTACTOR_NF,
}contactor_type;

/**
 *
 *
 */
typedef enum
{
	PRESSURE_SWITCH_NA = 1,
	PRESSURE_SWITCH_NF,
}pressure_switch_type;

/**
 *	Configuration angles
 *
 */
typedef	struct
{
	uint8_t id;
	uint16_t start_angle;
	uint16_t end_angle;
}pivot_sectors;

/**
 *	Configuration parameters
 *
 */
typedef	struct __attribute__((__packed__))
{
	char pivot_id[30];
	char gprs_id[30];
	contactor_type contactor;
	pressure_switch_type pressure_switch;
	uint16_t pressurization_time;
	uint8_t on_off_time;
	bool eco_mode;
	time_t start_time;
	time_t end_time;
	bool sector_enabled;
	pivot_sectors sectors[4]; //todo: ajustar o valor maximo de setores
}pivot_config;

/**
 *	scheduling date parameters
 *
 */
typedef	struct __attribute__((__packed__))
{
	char scheduling_id[30];
	bool is_running;
	time_t start_date;
	time_t end_date;
	pivot_actions acionts;
}pivot_scheduling_date;

/**
 *	scheduling angle parameters
 *
 */
typedef	struct __attribute__((__packed__))
{
	char scheduling_id[30];
	bool is_running;
	time_t start_date;
	uint16_t end_angle;
	pivot_actions acionts;
}pivot_scheduling_angle;

/**
 *	scheduling angle parameters
 *
 */
typedef	struct __attribute__((__packed__))
{
	bool is_running;
	time_t start_date;
	time_t end_date;
	uint16_t start_angle;
	uint16_t end_angle;
	pivot_actions acionts;
}pivot_history;

/**
 *	Main Callback request
 *
 */
typedef enum
{
	CALL_LOAD_ACTION = 1,	/*!< Configuration read request*/
	CALL_SAVE_ACTION,
	CALL_READ_ACTION,
	CALL_SAVE_CONFIG,
	CALL_READ_CONFIG,
	CALL_SAVE_SCHEDULE_DATE,
	CALL_LOAD_SCHEDULE_DATE,
	CALL_DELETE_SCHEDULE_DATE,
	CALL_SAVE_SCHEDULE_ANGLE,
	CALL_LOAD_SCHEDULE_ANGLE,
	CALL_DELETE_SCHEDULE_ANGLE,
	CALL_LOAD_HISTORY,
	CALL_MANUAL_PIVOT,
	CALL_OFF_PIVOT
}app_call_states;

/**
 * @brief: function used with return to main application class
 *
 */
typedef void (*app_callback)(app_call_states state, void* buffer);

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

/**@}*/ 	//FreeRTOS
/** @}*/	//Task_Definitions

#endif /* COMPONENTS_UTILS_INCLUDE_PROJECT_CONFIG_H_ */
