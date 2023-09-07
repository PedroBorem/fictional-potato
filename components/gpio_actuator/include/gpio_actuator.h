/**
 * @file gpio_actuator.h
 * @date June 20, 2022
 * @brief Board GPIO macros and pinout definitions for the GPIO Actuator module.
*/

#ifndef COMPONENTS_GPIO_ACTUATOR_INCLUDE_GPIO_ACTUATOR_H_
#define COMPONENTS_GPIO_ACTUATOR_INCLUDE_GPIO_ACTUATOR_H_

/* Configuration include */
#include "project_config.h"

/* include components */
#include "driver/gpio.h"

/**
 * @brief Macro defining the full cycle duration for the percentimeter (in milliseconds).
 */
#define GPIO_ACT_PERC_FULL_CYCLE		60000 //60 sec

/**
 * @brief GPIO pin number for turning the system on.
 */
#define GPIO_ACT_PIN_ON       		GPIO_NUM_13	/*!< Main system relay off*/

/**
 * @brief GPIO pin number for turning the system off.
 */
#define GPIO_ACT_PIN_OFF      		GPIO_NUM_12	/*!< Main system relay on*/

/**
 * @brief GPIO pin number for clockwise direction.
 */
#define GPIO_ACT_PIN_CW       		GPIO_NUM_11	/*!< Direction Clockwise*/

/**
 * @brief GPIO pin number for counter-clockwise direction.
 */
#define GPIO_ACT_PIN_CCW      		GPIO_NUM_10  /*!< Direction Counter-Clockwise*/

/**
 * @brief GPIO pin number for watering relay.
 */
#define GPIO_ACT_PIN_WATERING   	GPIO_NUM_9	/*!< Watering Relay*/

/**
 * @brief GPIO pin number for auxiliary system relay.
 */
#define GPIO_ACT_PIN_AUX      		GPIO_NUM_8	/*!< System Auxiliary Relay*/

/**
 * @brief GPIO pin number for auxiliary relay for percentimeter.
 */
#define GPIO_ACT_PIN_PERC_AUX   	GPIO_NUM_16	/*!< Auxiliary Relay for Percentimeter*/

/**
 * @brief GPIO pin number for percentimeter reading output.
 */
#define GPIO_ACT_PIN_PERC_OUT   	GPIO_NUM_35	/*!< Percentimeter*/

/**
 * @brief GPIO pin number for pump relay.
 */
#define GPIO_ACT_PIN_PUMP       	GPIO_NUM_14	/*!< Pump Relay*/

/**
 * @brief GPIO pin number for clockwise input.
 */
#define GPIO_ACT_PIN_CW_IN       	GPIO_NUM_7 /*!< Clockwise Input*/

/**
 * @brief GPIO pin number for counter-clockwise input.
 */
#define GPIO_ACT_PIN_CCW_IN      	GPIO_NUM_6	/*!< Counter-Clockwise Input*/

/**
 * @brief GPIO pin number for percentimeter read input.
 */
#define GPIO_ACT_PIN_PERC_IN     	GPIO_NUM_5	/*!< Percentimeter Read Input*/

/**
 * @brief GPIO pin number for water pressure reading input.
 */
#define GPIO_ACT_PIN_PRESS       	GPIO_NUM_4 /*!< Water pressure reading*/

/* Output Pins Group */
/**
 * @brief GPIO pin bitmask for all output pins.
 */
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
/**
 * @brief GPIO pin bitmask for all input pins.
 */
#define GPIO_INPUT_PIN_GROUP  ((1ULL<<GPIO_ACT_PIN_CW_IN)		\
								| (1ULL<<GPIO_ACT_PIN_CCW_IN)	\
								| (1ULL<<GPIO_ACT_PIN_PRESS))	\

/* Interrupt Pins Group */
/**
 * @brief GPIO pin bitmask for the percentimeter interrupt pin.
 */
#define GPIO_INT_PERC     (1ULL<<GPIO_ACT_PIN_PERC_IN)	//Percentimeter Read Input

/* Public function prototypes ------------------------------------------- */
/**
 * @brief Initializes the GPIO actuator module.
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Failed to initialize
 */
esp_err_t gpio_actuator_init(void);

esp_err_t gpio_actuator_config(pivot_config config);

/**
 * @brief Sets the pivot configuration.
 * @param config The pivot configuration to set
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Failed to set the configuration
 */
esp_err_t gpio_actuator_set(pivot_actions actions);

/**
 * @brief Gets the current pivot configuration.
 * @return The current pivot configuration
 */
pivot_actions gpio_actuator_get(void);

/**
 * @brief Shuts down all relays.
 */
void gpio_actuator_shutdown(void);

/**
 * @brief Turns on the pump relay.
 */
void gpio_actuator_pump_on(void);

/**
 * @brief Turns off the pump relay.
 */
void gpio_actuator_pump_off(void);

/**
 * @brief Turns on the pressure relay and starts pressure monitoring.
 */
void gpio_actuator_pressure_on(void);

/**
 * @brief Turns off the pressure relay and stops pressure monitoring.
 */
void gpio_actuator_pressure_off(void);

#endif /* COMPONENTS_GPIO_ACTUATOR_INCLUDE_GPIO_ACTUATOR_H_ */
