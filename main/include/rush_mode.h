/**
 * @file rush_mode.h
 * @brief Rush Mode public API.
 * @author soil-dev
 * @date 15 de out. de 2023
 */

#ifndef MAIN_INCLUDE_RUSH_MODE_H_
#define MAIN_INCLUDE_RUSH_MODE_H_

#include "project_config.h"

/**
 * @brief Starts Rush Mode with the given configuration.
 * @param current_rush_mode Rush Mode configuration.
 */
void rush_mode_start(rush_mode_config current_rush_mode);

/**
 * @brief Stops Rush Mode.
 */
void rush_mode_stop(void);

/**
 * @brief Suspends Rush Mode for the current window and clears any persisted restore state.
 *
 * When called during an active configured window, the current window remains suspended
 * until its end even after a reboot. Outside the configured window, any stale runtime
 * state is simply cleared.
 *
 * @param notify_override True to emit the rush override notification.
 */
void rush_mode_suspend_current_window(bool notify_override);

/**
 * @brief Registers a callback for Rush Mode events.
 * @param callback Callback to be registered.
 */
void rush_mode_register_callback(const app_callback callback);

#endif /* MAIN_INCLUDE_RUSH_MODE_H_ */
