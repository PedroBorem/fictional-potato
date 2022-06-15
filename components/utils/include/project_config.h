/*
 * project_config.h
 *
 *  Created on: 31 de mai de 2022
 *      Author: bruno
 */

#ifndef COMPONENTS_UTILS_INCLUDE_PROJECT_CONFIG_H_
#define COMPONENTS_UTILS_INCLUDE_PROJECT_CONFIG_H_

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

/* pivot configuration states */
typedef enum
{
	PIVOT_ON = 1,
	PIVOT_OFF = 2,
	PIVOT_ADVANCE = 3,
	PIVOT_REVERSE = 4,
	PIVOT_DRY = 5,
	PIVOT_WET = 6
}pivot_states;

/* pivot configuration parameters */
typedef	struct
{
	uint8_t power_state;	// PIVOT_ON or PIVOT_OFF
	uint8_t advance_mode;	// PIVOT_ADVANCE or PIVOT_REVERSE
	uint8_t watering_state;	// PIVOT_DRY or PIVOT_WET
	uint8_t percentimeter;	// Value from 0 to 100
}pivot_config;


#endif /* COMPONENTS_UTILS_INCLUDE_PROJECT_CONFIG_H_ */
