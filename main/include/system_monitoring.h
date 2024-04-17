/**
 * @file system_monitoring.h
 * @brief Header file for the system monitoring module.
 *
 * This module provides functions to start and stop system monitoring and register a callback function.
 *
 * @date 4 de ago. de 2023
 * @author soil-dev
 */

#ifndef MAIN_INCLUDE_SYSTEM_MONITORING_H_
#define MAIN_INCLUDE_SYSTEM_MONITORING_H_

#include "project_config.h"

/**
 * @brief Starts the system monitoring with the specified configuration.
 *
 * This function initiates the system monitoring module with the provided configuration.
 * It sets up a task to monitor the system based on the specified return configuration and monitoring time.
 *
 * @param[in] return_config The configuration for pivot return, including start and end angles.
 * @param[in] monitoring_time The time interval (in minutes) for monitoring the system.
 */
void system_monitoring_start(pivot_return_config return_config, uint8_t monitoring_time);

/**
 * @brief Stops the system monitoring.
 *
 * This function stops the system monitoring task and cleans up associated resources.
 */
void system_monitoring_stop(void);

/**
 * @brief Registers a callback function for system monitoring.
 *
 * This function allows the registration of a callback function to be used in the system monitoring module.
 * The registered callback will be invoked by the system monitoring module to perform specific actions.
 *
 * @param[in] callback A function pointer to the callback function.
 */
void system_monitoring_register_callback(const app_callback callback);


barrier_status get_barrier_status()

#endif /* MAIN_INCLUDE_SYSTEM_MONITORING_H_ */
