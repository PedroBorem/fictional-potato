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

#define MAX_RAINFALL_ENTRIES 65

extern float rain_total; /**< Total accumulated rainfall (shared with other modules). */

extern char pluviometro[MAX_RAINFALL_ENTRIES][30];

/**
 * @brief Starts the system monitoring with the specified configuration.
 *
 * This function initiates the system monitoring module with the provided configuration.
 * It sets up a task to monitor the system based on the specified return configuration and monitoring time.
 *
 * @param[in] virtual_config The configuration for pivot return, including start and end angles.
 * @param[in] monitoring_time The time interval (in minutes) for monitoring the system.
 */
void system_monitoring_start(pivot_physical_config physical_config, pivot_virtual_config virtual_config, uint8_t monitoring_time);

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

/**
 * @brief Determines and triggers actuation based on the barrier status.
 *
 * This function evaluates the current angle of the pivot and determines the barrier status
 * based on the angle configuration. It then triggers actuation accordingly to handle the
 * pivot's movement in relation to the barrier.
 *
 * @param[in] current_pivot_actions The current actions and configuration of the pivot.
 */
void system_monitoring_barrier(pivot_actions current_current_pivot_actions, type_barrier barrier_type);

/**
 * @brief Task function for monitoring rainfall.
 *
 * This function continuously monitors rainfall and updates the total accumulated rainfall.
 *
 * @param arg Task argument (default NULL).
 */
void rainfall_task(void *arg);

/**
 * @brief Initializes the rain gauge vector with NVS data.
 */
void init_rainfall_data(void);

#endif /* MAIN_INCLUDE_SYSTEM_MONITORING_H_ */
