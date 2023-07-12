/**
 * @file gprs_module.h
 * @brief GPRS Module API
 *
 * This file contains the declarations of functions and structures for the GPRS module.
 */

#ifndef COMPONENTS_GPRS_INCLUDE_GPRS_MODULE_H_
#define COMPONENTS_GPRS_INCLUDE_GPRS_MODULE_H_

#include "project_config.h"
#include "esp_err.h"

/**
 * @brief Initializes the GPRS module.
 *
 * This function initializes the GPRS module and prepares it for communication.
 *
 * @return
 *         - ESP_OK: Initialization successful.
 *         - ESP_FAIL: Initialization failed.
 */
esp_err_t gprs_module_init(void);

/**
 * @brief Sends an event through the GPRS module.
 *
 * This function converts the provided configuration structure to a string and sends it through the UART.
 *
 * @param config_in Configuration structure to send.
 * @param degree Degree value.
 * @param gprs_id GPRS ID.
 *
 * @return
 *         - ESP_OK: Event sent successfully.
 *         - ESP_FAIL: Failed to send event.
 */
esp_err_t gprs_module_send_event(pivot_actions config_in, uint16_t degree, const char *gprs_id);

/**
 * @brief Sets the GPRS ID.
 *
 * This function sets the GPRS ID used for communication.
 *
 * @param gprs_id GPRS ID to set.
 *
 * @return
 *         - ESP_OK: GPRS ID set successfully.
 *         - ESP_FAIL: Failed to set GPRS ID.
 */
esp_err_t gprs_module_set_id(const char *gprs_id);

/**
 * @brief Sends the GPRS IDP.
 *
 * This function sends the GPRS IDP.
 *
 * @param gprs_id GPRS ID to send.
 *
 * @return
 *         - ESP_OK: GPRS IDP sent successfully.
 *         - ESP_FAIL: Failed to send GPRS IDP.
 */
esp_err_t gprs_module_send_idp(const char *gprs_id);

/**
 * @brief Gets the timestamp in seconds.
 *
 * This function retrieves the timestamp obtained via GPRS.
 *
 * @return Timestamp in seconds.
 */
time_t gprs_module_get_timestamp(void);

/* Public Callback  ---------------------------------------------- */

/**
 * @brief Notifies the application about status messages.
 *
 * This function triggers a callback for each status message received (write and read).
 *
 * @param notify_buffer Pointer to the notification buffer.
 */
void GPRS_MODULE_NOTIFY_APP(void *notify_buffer);

#endif /* COMPONENTS_GPRS_INCLUDE_GPRS_MODULE_H_ */
