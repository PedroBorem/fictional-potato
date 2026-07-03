/**
 * @file gpio_actuator.h
 * @brief GPIO pinout and driver API for the new four-channel actuation product.
 */

#ifndef COMPONENTS_GPIO_ACTUATOR_INCLUDE_GPIO_ACTUATOR_H_
#define COMPONENTS_GPIO_ACTUATOR_INCLUDE_GPIO_ACTUATOR_H_

#include "driver/gpio.h"
#include "esp_err.h"
#include "project_config.h"

/**
 * @brief Relay board active level.
 *
 * The current relay board is active-low, matching the original hardware base.
 */
#define GPIO_ACT_RELAY_ENABLE  0
#define GPIO_ACT_RELAY_DISABLE 1

/**
 * @brief Relay pair for actuation channel 1.
 */
#define GPIO_ACT_PIN_CHANNEL_1_ON     GPIO_NUM_13
#define GPIO_ACT_PIN_CHANNEL_1_OFF    GPIO_NUM_12

/**
 * @brief Relay pair for actuation channel 2.
 */
#define GPIO_ACT_PIN_CHANNEL_2_ON     GPIO_NUM_11
#define GPIO_ACT_PIN_CHANNEL_2_OFF    GPIO_NUM_10

/**
 * @brief Relay pair for actuation channel 3.
 */
#define GPIO_ACT_PIN_CHANNEL_3_ON     GPIO_NUM_9
#define GPIO_ACT_PIN_CHANNEL_3_OFF    GPIO_NUM_8

/**
 * @brief Relay pair for actuation channel 4.
 */
#define GPIO_ACT_PIN_CHANNEL_4_ON     GPIO_NUM_16
#define GPIO_ACT_PIN_CHANNEL_4_OFF    GPIO_NUM_35

/**
 * @brief Status inputs for the four actuation channels.
 */
#define GPIO_ACT_PIN_CHANNEL_1_STATUS GPIO_NUM_7
#define GPIO_ACT_PIN_CHANNEL_2_STATUS GPIO_NUM_6
#define GPIO_ACT_PIN_CHANNEL_3_STATUS GPIO_NUM_5
#define GPIO_ACT_PIN_CHANNEL_4_STATUS GPIO_NUM_4

/**
 * @brief GPIO bitmask for all relay outputs.
 */
#define GPIO_ACT_OUTPUT_PIN_GROUP ((1ULL << GPIO_ACT_PIN_CHANNEL_1_ON)  | \
                                   (1ULL << GPIO_ACT_PIN_CHANNEL_1_OFF) | \
                                   (1ULL << GPIO_ACT_PIN_CHANNEL_2_ON)  | \
                                   (1ULL << GPIO_ACT_PIN_CHANNEL_2_OFF) | \
                                   (1ULL << GPIO_ACT_PIN_CHANNEL_3_ON)  | \
                                   (1ULL << GPIO_ACT_PIN_CHANNEL_3_OFF) | \
                                   (1ULL << GPIO_ACT_PIN_CHANNEL_4_ON)  | \
                                   (1ULL << GPIO_ACT_PIN_CHANNEL_4_OFF))

/**
 * @brief GPIO bitmask for all status inputs.
 */
#define GPIO_ACT_INPUT_PIN_GROUP ((1ULL << GPIO_ACT_PIN_CHANNEL_1_STATUS) | \
                                  (1ULL << GPIO_ACT_PIN_CHANNEL_2_STATUS) | \
                                  (1ULL << GPIO_ACT_PIN_CHANNEL_3_STATUS) | \
                                  (1ULL << GPIO_ACT_PIN_CHANNEL_4_STATUS))

/**
 * @brief Initializes the GPIO actuator module.
 */
esp_err_t gpio_actuator_init(void);

/**
 * @brief Applies actuator configuration.
 */
esp_err_t gpio_actuator_config(actuation_config config);

/**
 * @brief Pulses the ON/OFF relays requested by the action payload.
 */
esp_err_t gpio_actuator_set(actuation_actions actions);

/**
 * @brief Returns the live ON/OFF status read from input GPIOs.
 */
actuation_status gpio_actuator_get(void);

/**
 * @brief Copies the live ON/OFF status to an output buffer.
 */
void gpio_actuator_get_status(actuation_status *status_out);

/**
 * @brief De-energizes every relay output without issuing ON/OFF commands.
 */
void gpio_actuator_shutdown(void);

#endif /* COMPONENTS_GPIO_ACTUATOR_INCLUDE_GPIO_ACTUATOR_H_ */
