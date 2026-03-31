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
 * @brief Checks if the current angle is within the specified range barrier.
 *
 * This function evaluates the pivot's current angle and determines if it falls
 * within the defined range around the physical barrier's start and end angles.
 * It returns true if the angle is within the range, allowing for boundary
 * transitions, and false otherwise.
 *
 * @param[in] range_barrier The tolerance range (in degrees) around the barrier's start and end angles.
 * 
 * @return bool True if the current angle is within the specified range, false otherwise.
 */
bool system_monitoring_range_barrier(uint8_t range_barrier);

/**
 * @brief Registers a pivot shutdown event for system monitoring.
 *
 * Builds and stores a record describing why the pivot was shut down and
 * schedules an IDP #28 notification to be sent.
 *
 * @param shutdown_reason Enumerated cause of the pivot shutdown.
 * @param idp             IDP value associated with this shutdown event.
 * @param scheduling_id   Optional scheduling identifier string (may be NULL).
 * @param origin          Optional string that identifies who requested
 *                        the shutdown (module, app, etc.) (may be NULL).
 */
void system_monitoring_pivot_shutdown(hangs_up_status shutdown_reason, idp_type idp, char *scheduling_id, char *origin);

/**
 * @brief Starts the dedicated heartbeat monitoring task for the modem link.
 *
 * This monitor is independent from the barrier analysis task and is responsible
 * only for tracking the liveness packets exchanged with the modem. When the
 * heartbeat times out, the monitor first requests a local modem reset through
 * `IDP 92` and only later may request a local `IDP 91` reset if the pivot is
 * currently off. The task is expected to start only after the first valid
 * `PING` received from the modem.
 */
void system_monitoring_modem_heartbeat_start(void);

/**
 * @brief Stops the dedicated heartbeat monitoring task for the modem link.
 */
void system_monitoring_modem_heartbeat_stop(void);

/**
 * @brief Feeds the heartbeat monitor with a new valid frame from the modem.
 *
 * A valid heartbeat frame clears any pending timeout-driven recovery stage.
 *
 * @param heartbeat_state Textual state received in the heartbeat payload.
 */
void system_monitoring_modem_heartbeat_feed(const char *heartbeat_state);

#endif /* MAIN_INCLUDE_SYSTEM_MONITORING_H_ */
