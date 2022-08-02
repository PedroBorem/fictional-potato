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
#include "driver/gpio.h"
#include <time.h>

#define SYS_ENABLE			0
#define SYS_DISABLE			1

#define PERC_FULL_CYCLE		60000 //60 sec
#define PRESSURE_TIMEOUT    300000 //10 sec
/* Pinout references*/
/* GPIO Outputs */
#define PIN_ON       	GPIO_NUM_3	//Main system relay off
#define PIN_OFF      	GPIO_NUM_1	//Main system relay on
#define PIN_AUX      	GPIO_NUM_15	//System Auxiliar Relay
#define PIN_CW       	GPIO_NUM_0	//Direction Clockwise
#define PIN_CCW      	GPIO_NUM_22	//Direction Counter-Clockwise
#define PIN_WATERING   	GPIO_NUM_23	//Watering Relay
#define PIN_PERC_AUX    GPIO_NUM_2	//Auxiliar Relay for Percentimeter
#define PIN_PERC_OUT    GPIO_NUM_4	//Percentimeter
#define PIN_PUMP        GPIO_NUM_25	//Pump Relay

/* GPIO Inputs */
#define PIN_CW_IN       GPIO_NUM_36 //Clockwise Input
#define PIN_CCW_IN      GPIO_NUM_37	//Counter-Clockwise Input
#define PIN_PERC_IN     GPIO_NUM_38	//Percentimeter Read Input
#define PIN_PRESS       GPIO_NUM_39 //Water pressure reading

/* LoRA SPI */
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

/* Output Pins Group */
#define GPIO_OUTPUT_PIN_GROUP  ((1ULL<<PIN_ON) | (1ULL<<PIN_AUX) | (1ULL<<PIN_CW) | (1ULL<<PIN_CCW) | (1ULL<<PIN_WATERING) | (1ULL<<PIN_PERC_AUX) | (1ULL<<PIN_PERC_OUT) | (1ULL<<PIN_PUMP))
//(1ULL<<PIN_OFF) | this pin breaks uart for logs

/* Input Pins Group */
#define GPIO_INPUT_PIN_GROUP  ((1ULL<<PIN_CW_IN) | (1ULL<<PIN_CCW_IN) | (1ULL<<PIN_PRESS))

/* Interrupt Pins Group */
#define GPIO_INT_PERC     (1ULL<<PIN_PERC_IN)	//Percentimeter Read Input

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

/**
 * @brief	Perc On timer expiration callback.
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail to initialize
 */
void vPercTimerOnExpire(xTimerHandle pxTimer);

/**
 * @brief	Perc Off timer expiration callback.
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail to initialize
 */
void vPercTimerOffExpire(xTimerHandle pxTimer);

#endif /* COMPONENTS_GPIO_ACTUATOR_INCLUDE_GPIO_ACTUATOR_H_ */
