/**
 * @file eco_mode.h
 * @brief Eco Mode public API.
 * @author soil-dev
 * @date 15 de out. de 2023
 */

#ifndef MAIN_INCLUDE_ECO_MODE_H_
#define MAIN_INCLUDE_ECO_MODE_H_

#include "project_config.h"

/**
 * @brief Starts Eco Mode with the given configuration.
 * @param current_eco_mode Eco Mode configuration.
 */
void eco_mode_start(eco_mode_config current_eco_mode);

/**
 * @brief Stops Eco Mode.
 */
void eco_mode_stop(void);

/**
 * @brief Suspends Eco Mode actions if they are active.
 */
void eco_mode_cmd_stop(void);

/**
 * @brief Checks if the Eco Mode window is active now.
 * @return true if the current time is inside the Eco Mode window.
 */
bool eco_mode_is_in_window_now(void);

/**
 * @brief Registers a callback for Eco Mode events.
 * @param callback Callback to be registered.
 */
void eco_mode_register_callback(const app_callback callback);

#endif /* MAIN_INCLUDE_ECO_MODE_H_ */
