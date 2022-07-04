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
#include "project_config.h"

#define SYS_ENABLE			0
#define SYS_DISABLE			1

#define PERC_FULL_CYCLE		10

/* Pinout references*/
/* GPIO Outputs */
#define PIN_ON       	3	//Main system relay off
#define PIN_OFF      	1	//Main system relay on
#define PIN_AUX      	15	//System Auxiliar Relay
#define PIN_CW       	0	//Direction Clockwise
#define PIN_CCW      	22	//Direction Counter-Clockwise
#define PIN_WATERING   	23	//Watering Relay
#define PIN_PERC_AUX    2	//Auxiliar Relay for Percentimeter
#define PIN_PERC_OUT    4	//Percentimeter
#define PIN_PUMP        25	//Pump Relay

/* GPIO Inputs */
#define PIN_CW_IN       36 	//Clockwise Input
#define PIN_CCW_IN      37	//Counter-Clockwise Input
#define PIN_PRESS       38 	//?
#define PIN_PERC_IN     39	//Percentimeter Read Input

/* LoRA SPI */
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

/* Output Pins Group */
#define GPIO_OUTPUT_PIN_GROUP  ((1ULL<<PIN_ON) | (1ULL<<PIN_OFF) \
							  | (1ULL<<PIN_AUX) | (1ULL<<PIN_CW) \
							  | (1ULL<<PIN_CCW) | (1ULL<<PIN_WATERING) \
							  | (1ULL<<PIN_PERC_AUX) | (1ULL<<PIN_PERC_OUT) \
							  | (1ULL<<PIN_PUMP))

/* Input Pins Group */
#define GPIO_INPUT_PIN_GROUP  ((1ULL<<PIN_CW_IN) | (1ULL<<PIN_CCW_IN) \
							  | (1ULL<<PIN_PRESS) | (1ULL<<PIN_PERC_IN))

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

void gpio_actuator_shutdown(void);
void vPercTimerOnExpire(xTimerHandle pxTimer);
void vPercTimerOffExpire(xTimerHandle pxTimer);

#endif /* COMPONENTS_GPIO_ACTUATOR_INCLUDE_GPIO_ACTUATOR_H_ */
