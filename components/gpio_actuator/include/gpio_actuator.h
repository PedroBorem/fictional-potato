/*
 * gpio_actuator.h
 *
 *  Created on: Jun 20, 2022
 *      Author: guilhermerossi
 */

#ifndef COMPONENTS_GPIO_ACTUATOR_INCLUDE_GPIO_ACTUATOR_H_
#define COMPONENTS_GPIO_ACTUATOR_INCLUDE_GPIO_ACTUATOR_H_

/**
 * @file gpio_actuator.h
 * @date June 20, 2022
 * @brief board gpio macros
*/

/* Configuration include */
#include "project_config.h"

/* include components */
#include "driver/gpio.h"

#define GPIO_ACT_SYS_ENABLE			0
#define GPIO_ACT_SYS_DISABLE		1

#define GPIO_ACT_PERC_FULL_CYCLE		60000 //60 sec
#define GPIO_ACT_PRESSURE_TIMEOUT    	300000 //5 min
#define GPIO_ACT_ONOFF_DELAY			2000

/* Pinout references*/
/* GPIO Outputs */
#define GPIO_ACT_PIN_ON       		GPIO_NUM_13	/*!< Main system relay off*/
#define GPIO_ACT_PIN_OFF      		GPIO_NUM_12	/*!<Main system relay on*/
#define GPIO_ACT_PIN_CW       		GPIO_NUM_11	/*!<Direction Clockwise*/
#define GPIO_ACT_PIN_CCW      		GPIO_NUM_10  /*!<Direction Counter-Clockwise*/
#define GPIO_ACT_PIN_WATERING   	GPIO_NUM_9	/*!<Watering Relay*/
#define GPIO_ACT_PIN_AUX      		GPIO_NUM_8	/*!<System Auxiliar Relay*/
#define GPIO_ACT_PIN_PERC_AUX   	GPIO_NUM_16	/*!<Auxiliar Relay for Percentimeter*/
#define GPIO_ACT_PIN_PERC_OUT   	GPIO_NUM_35	/*!<Percentimeter*/
#define GPIO_ACT_PIN_PUMP       	GPIO_NUM_14	/*!<Pump Relay*/

/* GPIO Inputs */
#define GPIO_ACT_PIN_CW_IN       	GPIO_NUM_7 /*!<Clockwise Input*/
#define GPIO_ACT_PIN_CCW_IN      	GPIO_NUM_6	/*!<Counter-Clockwise Input*/
#define GPIO_ACT_PIN_PERC_IN     	GPIO_NUM_5	/*!<Percentimeter Read Input*/
#define GPIO_ACT_PIN_PRESS       	GPIO_NUM_4 /*!<Water pressure reading*/

/* Output Pins Group */
#define GPIO_OUTPUT_PIN_GROUP  ((1ULL<<GPIO_ACT_PIN_ON)			\
								| (1ULL<<GPIO_ACT_PIN_OFF)		\
								| (1ULL<<GPIO_ACT_PIN_AUX)		\
								| (1ULL<<GPIO_ACT_PIN_CW)		\
								| (1ULL<<GPIO_ACT_PIN_CCW)		\
								| (1ULL<<GPIO_ACT_PIN_WATERING)	\
								| (1ULL<<GPIO_ACT_PIN_PERC_AUX)	\
								| (1ULL<<GPIO_ACT_PIN_PERC_OUT)	\
								| (1ULL<<GPIO_ACT_PIN_PUMP))	\

/* Input Pins Group */
#define GPIO_INPUT_PIN_GROUP  ((1ULL<<GPIO_ACT_PIN_CW_IN)		\
								| (1ULL<<GPIO_ACT_PIN_CCW_IN)	\
								| (1ULL<<GPIO_ACT_PIN_PRESS))	\

/* Interrupt Pins Group */
#define GPIO_INT_PERC     (1ULL<<GPIO_ACT_PIN_PERC_IN)	//Percentimeter Read Input

/* Public function prototypes ------------------------------------------- */
/**
 * @brief	Initial configuration of all pins.
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail to initialize
 */
esp_err_t gpio_actuator_init(void);

/**
 * @brief	Activate relays based on pivot configuration.
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail to initialize
 */
esp_err_t gpio_actuator_set(pivot_config config);

/**
 * @brief	Read actual pivot configuration.
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail to initialize
 */
pivot_config gpio_actuator_get(void);

/**
 * @brief	Disables all relays.
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail to initialize
 */
void gpio_actuator_shutdown(void);


void gpio_actuator_pump_on(void);

void gpio_actuator_pump_off(void);

void gpio_actuator_pressure_on(void);

void gpio_actuator_pressure_off(void);

#endif /* COMPONENTS_GPIO_ACTUATOR_INCLUDE_GPIO_ACTUATOR_H_ */
