/**
 * @file pluviometer_app.h
 * @brief Pluviometer application class for managing rainfall data.
 * 
 * This header file defines the Pluviometer application class for managing rainfall data.
 */
#ifndef PLUVIOMETER_APP_H
#define PLUVIOMETER_APP_H

#include "project_config.h"

#include "esp_err.h"


#define RAINFALL_SAVE_INTERVAL_MS (3600000) /**< Interval for saving rainfall data in milliseconds (1 hour) */

/**
 * @brief Returns the rain calibration value in millimeters per pulse.
 * @return Current calibration value (mm/pulse).
 */
float get_rain_per_pulse(void);

/**
 * @brief Sets the rain calibration value in millimeters per pulse.
 * @param mm_per_pulse New calibration value (mm/pulse).
 */
void set_rain_per_pulse(float mm_per_pulse);

/**
 * @brief Set the rain-per-pulse flag status.
 * @param flag true to enable rain-per-pulse mode, false to disable.
 */
void set_rain_per_pulse_flag(bool flag);

/**
 * @brief Initializes the rain gauge vector with NVS data.
 */
void pluviometer_app_init_rainfall_data(const app_callback callback);

/**
 * @brief Obtains a safe copy of the current day's data structure.
 * @param out_data Pointer to the structure where the data will be copied.
 * @return ESP_OK if successful.
 */
esp_err_t pluviometer_app_get_current_day(rain_per_day_data *out_data);

/**
 * @brief Obtains the index of the last closed hour (0-23).
 * @return uint8_t Hour index (0-23), or 0xFF if not yet initialized.
 */
uint8_t pluviometer_app_get_active_hour_idx(void);

/**
 * @brief Task to monitor and manage rainfall data.
 *
 * This function runs as a FreeRTOS task to monitor and manage rainfall data.
 *
 * @param arg Pointer to task arguments (not used).
 */
void pluviometer_app_rainfall_task(void *arg);

#endif // PLUVIOMETER_APP_H
