/**
 * @file scheduling.h
 * @brief Header file for scheduling functionality.
 * @author soil-dev
 * @date 31 de jul. de 2023
 */

#ifndef MAIN_INCLUDE_SCHEDULING_H_
#define MAIN_INCLUDE_SCHEDULING_H_

#include "project_config.h"

/**
 * @brief Starts a scheduling task based on the provided IDP and data.
 *
 * This function initiates a scheduling task based on the given IDP (InterDevice Protocol) and the associated data.
 * The task is selected according to the IDP, and the data is used to configure and execute the task.
 *
 * @param scheduling_idp The InterDevice Protocol identifier for scheduling.
 * @param scheduling_data Pointer to the data structure associated with the scheduling task.
 *
 * @note The function checks for null values in the scheduling_data parameter and logs an error if found.
 */
void scheduling_start(idp_type scheduling_idp, void* scheduling_data);

/**
 * @brief Registers a callback function for scheduling events.
 *
 * This function allows the registration of a callback function that will be invoked for scheduling events.
 * The callback function is used to communicate with other modules or external systems.
 *
 * @param callback The callback function to be registered.
 *
 * @note The function checks for null values in the callback parameter.
 */
void scheduling_register_callback(const app_callback callback);

/**
 * @brief Registers a callback function for scheduling hang up events.
 *
 * This function registers a callback that will be invoked when scheduling hang up events occur.
 * The callback is used to notify other modules or external systems about these events.
 *
 * @param callback The hang up callback function to be registered.
 *
 * @note The function checks if the callback parameter is not NULL.
 */
void scheduling_hangs_up_callback(const hangs_up_callback callback);

#endif /* MAIN_INCLUDE_SCHEDULING_H_ */
