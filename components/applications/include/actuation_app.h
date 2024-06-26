/**
 * @file actuation_app.h
 * @date June 23, 2022
 * @brief Actuation control class header file.
 */

#ifndef COMPONENTS_APPLICATIONS_INCLUDE_ACTUATION_APP_H_
#define COMPONENTS_APPLICATIONS_INCLUDE_ACTUATION_APP_H_

#include "project_config.h"
#include "esp_err.h"

/**
 * @brief Pivot actions structure.
 *
 * This structure defines the possible actions for actuation, including power state,
 * rotation direction, watering state, and percentimeter reading.
 */
extern const pivot_actions pivot_actions_off;

/**
 * @brief Initializes the actuation application.
 *
 * This function starts the tasks for reading and actuation of the I/O ports.
 *
 * @param app_callback - [in]: Function used for returning to the main application class.
 * @return
 * 	- ESP_OK: Success
 * 	- ESP_FAIL: Fail to initialize
 */
esp_err_t actuation_app_init(const app_callback callback);

/**
 * @brief Sets the configuration for the actuation application.
 *
 * This function applies the provided configuration to the drives.
 *
 * @param config - [in]: Pivot configuration structure.
 * @return
 *   - ESP_OK: Configuration applied successfully
 *   - ESP_FAIL: Configuration failed
 */
esp_err_t actuation_app_set_config(pivot_config config);

/**
 * @brief Applies the provided actions to the actuation application.
 *
 * This function applies the provided actions to the actuation application,
 * controlling power state, rotation direction, watering state, and percentimeter reading.
 *
 * @param config_in - [in]: Pivot actions structure.
 * @param alert_change - [in]: Flag indicating whether to alert on configuration change.
 */
void actuation_app_set_actions(const pivot_actions config_in, bool alert_change);

/**
 * @brief Reads the current status on IO ports.
 *
 * This function retrieves the current status of the actuation application,
 * including power state, rotation direction, watering state, and percentimeter reading.
 *
 * @param config_out - [out]: Pivot configuration structure.
 * @param config_size - [out]: Structure size.
 */
void actuation_app_get_actions(pivot_actions* config_out, size_t config_size);

/**
 * @brief Sets the pump state for the actuation application.
 *
 * This function turns on or off the pump of the actuation application.
 *
 * @param pump_state - [in]: Pump state (true for on, false for off).
 */
void actuation_app_set_pump(bool pump_state);

/**
 * @brief Shuts down the actuation application.
 *
 * This function gracefully shuts down the actuation application.
 */
void actuation_app_shutdown(void);

void actuation_app_leaving_barrier_time(pivot_return_config barrier_config);

#endif /* COMPONENTS_APPLICATIONS_INCLUDE_ACTUATION_APP_H_ */
