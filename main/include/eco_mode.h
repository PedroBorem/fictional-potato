/**
 * @file eco_mode.h
 * @brief Header file for the Eco Mode module.
 * @author soil-dev
 * @date 15 de out. de 2023
 */

#ifndef MAIN_INCLUDE_ECO_MODE_H_
#define MAIN_INCLUDE_ECO_MODE_H_

#include "project_config.h"

/**
 * @brief Starts the Eco Mode with the provided configuration.
 * @param current_eco_mode Configuration for Eco Mode.
 * @note If the start or end time is not set, the Eco Mode will be stopped.
 */
void eco_mode_start(eco_mode_config current_eco_mode);

/**
 * @brief Stops the Eco Mode.
 */
void eco_mode_stop(void);

/**
 * @brief Stops the Eco Mode if it had turned off the pivot.
 */
void eco_mode_cmd_stop(void);

/**
 * @brief Registers a callback function for Eco Mode events.
 * @param callback Callback function to be registered.
 */
void eco_mode_register_callback(const app_callback callback);

#endif /* MAIN_INCLUDE_ECO_MODE_H_ */
