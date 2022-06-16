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
 * @brief general project settings and settings
*/

/* Configuration include*/
#include "sdkconfig.h"

/* C base */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

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
	CALL_LOAD_CONFIG = 1,	/*!< Configuration read request*/
}app_call_states;

/**
 *	Pivot configuration states
 *
 */
typedef enum
{
	PIVOT_ON = 1,			/*!< Pivot on*/
	PIVOT_OFF = 2,			/*!< Pivot off*/
	PIVOT_ADVANCE = 3,		/*!< Pivot in advanced mode*/
	PIVOT_REVERSE = 4,		/*!< Pivot in reverse mode*/
	PIVOT_DRY = 5,			/*!< Irrigation off*/
	PIVOT_WET = 6			/*!< Irrigation on*/
}pivot_states;

/**
 *	Pivot configuration parameters
 *
 */
typedef	struct __attribute__((__packed__))
{
	uint8_t power_state;	/*!< PIVOT_ON or PIVOT_OFF*/
	uint8_t advance_mode;	/*!< PIVOT_ADVANCE or PIVOT_REVERSE*/
	uint8_t watering_state;	/*!< PIVOT_DRY or PIVOT_WET*/
	uint8_t percentimeter;	/*!< Value from 0 to 100*/
}pivot_config;

/* FreeRTOS allocated definitions ******************************************/
/* data tasks */
#define DATA_APP_TASK_NAME				"data app task"
#define DATA_APP_STACK_SIZE				( configMINIMAL_STACK_SIZE * 4 )
#define DATA_APP_TASK_PRIORITY			( tskIDLE_PRIORITY + 2 )


#endif /* COMPONENTS_UTILS_INCLUDE_PROJECT_CONFIG_H_ */
