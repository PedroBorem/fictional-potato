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
 * @brief Suspends Eco Mode for the current window and emits the rush override notification.
 */
void eco_mode_cmd_stop(void);

/**
 * @brief Suspends Eco Mode for the current window and clears any persisted restore state.
 *
 * When called during an active configured window, the current window remains suspended
 * until its end even after a reboot. Outside the configured window, any stale runtime
 * state is simply cleared.
 *
 * @param notify_override True to emit the rush override notification.
 */
void eco_mode_suspend_current_window(bool notify_override);

/**
 * @brief Registers a callback for Eco Mode events.
 * @param callback Callback to be registered.
 */
void eco_mode_register_callback(const app_callback callback);

#endif /* MAIN_INCLUDE_ECO_MODE_H_ */
