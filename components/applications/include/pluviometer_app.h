/**
 * @file pluviometer_app.h
 * @brief Pluviometer application class for managing rainfall data.
 * 
 * This header file defines the Pluviometer application class for managing rainfall data.
 */
#ifndef PLUVIOMETER_APP_H
#define PLUVIOMETER_APP_H

#include "project_config.h"

/**
 * @brief Daily rainfall with 24 hourly bins.
 * @note Values are in tenths of a millimeter (0.1 mm).
 */
typedef struct __attribute__((__packed__))
{
    char date_day[50];   /**< Local date in "YYYYMMDD". */
    float rain_per_hour[24];  /**< Hourly rainfall (index 0..23 = 00–01 ... 23–00), 0.1 mm. */
    float daily_total;      /**< Daily total (sum of 24 hours), 0.1 mm. */
} rain_per_day_data;

#define MAX_RAINFALL_ENTRIES 24 /**< Maximum number of rainfall entries */

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
void system_monitoring_init_rainfall_data(void);

/**
 * @brief Task to monitor and manage rainfall data.
 *
 * This function runs as a FreeRTOS task to monitor and manage rainfall data.
 *
 * @param arg Pointer to task arguments (not used).
 */
void system_monitoring_rainfall_task(void *arg);

#endif // PLUVIOMETER_APP_H
