/**
 * @file actuation_app.h
 * @brief Application layer for the new four-channel actuation product.
 */

#ifndef COMPONENTS_APPLICATIONS_INCLUDE_ACTUATION_APP_H_
#define COMPONENTS_APPLICATIONS_INCLUDE_ACTUATION_APP_H_

#include "esp_err.h"
#include "project_config.h"

/**
 * @brief Empty command payload used when no relay pulse should be issued.
 */
extern const actuation_actions actuation_actions_none;

/**
 * @brief Initializes the actuation application and GPIO driver.
 */
esp_err_t actuation_app_init(const app_callback callback);

/**
 * @brief Applies actuation configuration.
 */
esp_err_t actuation_app_set_config(actuation_config config);

/**
 * @brief Pulses ON/OFF relay pairs according to the provided command payload.
 */
void actuation_app_set_actions(const actuation_actions actions, bool alert_change);

/**
 * @brief Returns the last command payload accepted by the application.
 */
void actuation_app_get_actions(actuation_actions *actions_out, size_t actions_size);

/**
 * @brief Reads the live ON/OFF status from the four status inputs.
 */
void actuation_app_get_status(actuation_status *status_out, size_t status_size);

/**
 * @brief Returns true when all live status inputs indicate OFF.
 */
bool actuation_app_is_all_off(void);

/**
 * @brief De-energizes every relay output without issuing ON/OFF commands.
 */
void actuation_app_shutdown(void);

#endif /* COMPONENTS_APPLICATIONS_INCLUDE_ACTUATION_APP_H_ */
