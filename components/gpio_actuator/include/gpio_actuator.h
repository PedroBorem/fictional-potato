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

extern float rain_per_pulse;

/**
 * @brief Macro to enable the GPIO actuator system.
 */
#define GPIO_ACT_SYS_ENABLE			0

/**
 * @brief Macro to disable the GPIO actuator system.
 */
#define GPIO_ACT_SYS_DISABLE		1

/**
 * @brief Macro defining the full cycle duration for the percentimeter (in milliseconds).
 */
#define GPIO_ACT_PERC_FULL_CYCLE		60000 //60 sec

/**
* @brief Time duration for high logic level against the barrier (in milliseconds).
*/
#define HIGH_LOGIC_LEVEL_TIME_AGAINST_BARRIER	500

/**
* @brief Time duration for high logic level outside the barrier (in milliseconds).
*/
#define HIGH_LOGIC_LEVEL_TIME_OUTSIDE_BARRIER	1000

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

/**
 * @brief GPIO pin number for rain sensor input.
 */
#define GPIO_ACT_RAIN_SENSOR_PIN      GPIO_NUM_45 /*!< Rain Sensor Input */

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
#define GPIO_INPUT_PIN_GROUP  ((1ULL<<GPIO_ACT_PIN_CW_IN)			\
								| (1ULL<<GPIO_ACT_PIN_CCW_IN)		\
								| (1ULL<<GPIO_ACT_PIN_PRESS)		\
								| (1ULL<<GPIO_ACT_RAIN_SENSOR_PIN)) \

/* Interrupt Pins Group */
/**
 * @brief GPIO pin bitmask for the percentimeter interrupt pin.
 */
#define GPIO_INT_PERC     (1ULL<<GPIO_ACT_PIN_PERC_IN)	//Percentimeter Read Input

/**
 * @brief GPIO pin bitmask for rain sensor interrupt pin.
 */
#define GPIO_INT_RAIN_SENSOR  (1ULL<<GPIO_ACT_RAIN_SENSOR_PIN)

/* Public function prototypes ------------------------------------------- */
/**
 * @brief Initializes the GPIO actuator module.
 *
 * This function initializes the GPIO actuator module, setting up GPIO pins and configurations.
 *
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Failed to initialize
 */
esp_err_t gpio_actuator_init(const app_callback callback);

/**
 * @brief Configures the GPIO actuator module with the specified configuration.
 *
 * This function configures the GPIO actuator module based on the provided configuration.
 *
 * @param config The configuration to apply
 *
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Failed to configure
 */
esp_err_t gpio_actuator_config(pivot_config config);

/**
 * @brief Sets the pivot configuration.
 *
 * This function sets the pivot configuration for the GPIO actuator module.
 *
 * @param actions The pivot actions to set
 *
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Failed to set the configuration
 */
esp_err_t gpio_actuator_set(pivot_actions actions);

/**
 * @brief Sets operating time based on barrier status for GPIO actuator module.
 *
 * Adjusts GPIO actuator runtime according to barrier status,
 * optimizing performance for efficient and responsive operation.
 *
 * @param barrier_status The barrier status
 *
 * @return
 *     - ESP_OK: Operation successful
 *     - ESP_FAIL: Failed to set time duration
 */
esp_err_t gpio_actuator_set_time_start(barrier_status barrier_status);

/**
 * @brief Gets the current pivot configuration.
 *
 * This function retrieves the current pivot configuration from the GPIO actuator module.
 *
 * @return The current pivot configuration
 */
pivot_actions gpio_actuator_get(void);

/**
 * @brief Shuts down all relays.
 *
 * This function shuts down all relays in the GPIO actuator module.
 */
void gpio_actuator_shutdown(void);

/**
 * @brief Turns on the pump relay.
 *
 * This function turns on the pump relay in the GPIO actuator module.
 */
void gpio_actuator_pump_on(void);

/**
 * @brief Turns off the pump relay.
 *
 * This function turns off the pump relay in the GPIO actuator module.
 */
void gpio_actuator_pump_off(void);

/**
 * @brief Turns on the pressure relay and starts pressure monitoring.
 *
 * This function turns on the pressure relay and starts pressure monitoring in the GPIO actuator module.
 */
void gpio_actuator_pressure_on(void);

/**
 * @brief Turns off the pressure relay and stops pressure monitoring.
 *
 * This function turns off the pressure relay and stops pressure monitoring in the GPIO actuator module.
 */
void gpio_actuator_pressure_off(void);

void set_gpio_leaving_barrier_time(pivot_physical_config barrier_config);

/**
 * @brief Initializes the rain sensor.
 *
 * This function sets up the GPIO pin and interrupt for the rain sensor.
 *
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Failed to initialize
 */
esp_err_t gpio_actuator_rain_sensor_init(void);

/**
 * @brief Calculates and logs rainfall based on the sensor pulses.
 *
 * Computes the rainfall based on pulses detected by the rain sensor
 * and updates the total accumulated rainfall.
 */
void gpio_rain_sensor_calculate_rainfall(void);

#endif /* COMPONENTS_GPIO_ACTUATOR_INCLUDE_GPIO_ACTUATOR_H_ */
