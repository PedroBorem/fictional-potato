/**
 * @file rtc_app.h
 * @brief RTC application class for managing date and time functionality.
 *
 * This header file defines the RTC application class for managing date and time functionality.
 */

#ifndef COMPONENTS_RTC_INCLUDE_RTC_APP_H_
#define COMPONENTS_RTC_INCLUDE_RTC_APP_H_

#include <time.h>
#include <stdbool.h>

#include "esp_err.h"

/**
 * @def RTC_CONFIG_TIMEZONE
 * @brief The default time zone for the RTC configuration.
 */
#define RTC_CONFIG_TIMEZONE -3


/**
 * @brief Initializes the RTC application.
 *
 * This function initializes the RTC application.
 *
 * @return bool True if initialization is successful, false otherwise.
 */
esp_err_t rtc_app_init(void);

/**
 * @brief Sets the timestamp in the RTC.
 *
 * This function sets the timestamp in the RTC with the specified timestamp value.
 *
 * @param timestamp The timestamp value to set.
 * @return bool True if setting the timestamp is successful, false otherwise.
 */
bool rtc_app_set_timestamp(time_t timestamp);

/**
 * @brief Gets the timestamp from the RTC.
 *
 * This function retrieves the timestamp from the RTC.
 *
 * @param rtc_show_dt Flag indicating whether to display the date and time in local time.
 * @return time_t The timestamp value.
 */
time_t rtc_app_get_timestamp(bool rtc_show_dt);

/**
 * @brief Gets the date and time from the RTC.
 *
 * This function retrieves the date and time from the RTC and stores it in the specified time structure.
 *
 * @param rtcinfo The pointer to the time structure to store the date and time.
 */
void rtc_app_get_date_time(struct tm* rtcinfo);

/**
 * @brief Displays the date and time.
 *
 * This function displays the date and time based on the specified timestamp and time zone.
 *
 * @param timestamp_now The timestamp value.
 * @param time_z The time zone.
 */
void rtc_show_date_time(time_t timestamp_now, uint8_t time_z);

#endif /* COMPONENTS_RTC_INCLUDE_RTC_APP_H_ */
