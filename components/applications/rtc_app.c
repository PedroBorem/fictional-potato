/**
 * @file rtc_app.c
 * @brief RTC application implementation for managing date and time functionality.
 *
 * This file contains the implementation of the RTC application for managing date and time functionality.
 */

#include "rtc_app.h"
#include "rtc_ds3231.h"
#include "esp_log.h"
#include "log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>
#include <stdlib.h>

/**
 * @def RTC_APP_TAG
 * @brief Log tag for RTC application.
 */
#define RTC_APP_TAG "rtc_app"

/**
 * @def RTC_SDA_PIN
 * @brief The GPIO pin number for the RTC I2C SDA signal.
 */
#define RTC_SDA_PIN 36

/**
 * @def RTC_SCL_PIN
 * @brief The GPIO pin number for the RTC I2C SCL signal.
 */
#define RTC_SCL_PIN 37

/**
 * @def TIMESTAMP_09_05_2024
 * @brief Timestamp representing May 9th, 2024.
 */
#define TIMESTAMP_09_05_2024 1715280638

/**
 * @def TIMESTAMP_09_05_2040
 * @brief Timestamp representing May 9th, 2040.
 */
#define TIMESTAMP_09_05_2040 2220202096


/**
 * @var dev
 * @brief The RTC I2C device structure.
 */
static rtc_i2c_dev_t dev = {};

static time_t rtc_app_timestamp_ctrl = 0;

/* Public methods ----------------------------------- */

/**
 * @brief Initializes the RTC application.
 *
 * This function initializes the RTC application by configuring the RTC I2C device.
 *
 * @return esp_err_t Error code indicating the success of the operation.
 */
esp_err_t rtc_app_init(void)
{
	// Initialize RTC
	if( ds3231_init_desc(&dev, I2C_NUM_0, RTC_SDA_PIN, RTC_SCL_PIN) == ESP_OK)
	{
		return ESP_OK;
	}

	return ESP_FAIL;
}

/**
 * @brief Sets the timestamp in the RTC.
 *
 * This function sets the timestamp in the RTC with the specified timestamp value.
 *
 * @param timestamp The timestamp value to set.
 * @return bool True if setting the timestamp is successful, false otherwise.
 */
bool rtc_app_set_timestamp(time_t timestamp)
{
	bool ret = false;

	if(timestamp > TIMESTAMP_09_05_2024 && timestamp < TIMESTAMP_09_05_2040) // date: 09/05/2024 09/05/2040
	{
		struct tm time = *localtime(&timestamp); //obs time + 1900
		time.tm_year += 1900;
		time.tm_mon += 1;

		if (ds3231_set_time(&dev, &time) != ESP_OK)
		{
			ESP_LOGE(RTC_APP_TAG, "Could not set time.");
			LOG_DBG_ERROR(RTC_APP_TAG, "set_hw_rtc_error");
		}
		else
		{
			rtc_app_timestamp_ctrl = timestamp;
			ret = true;
			ESP_LOGI(RTC_APP_TAG, "Set initial date time done (%lld)", timestamp);
		}
	}
	else
	{
		ESP_LOGE(RTC_APP_TAG, "(%s), Set invalid timestamp (%lld)",__func__, timestamp);
		LOG_DBG_ERROR(RTC_APP_TAG, "set_invalid_timestamp");
		vTaskDelay(pdMS_TO_TICKS(2000));
		char buffer[20];
    	sprintf(buffer, "%lld", timestamp);
		LOG_DBG_ERROR(RTC_APP_TAG, buffer);
	}

	return ret;
}

/**
 * @brief Gets the timestamp from the RTC.
 *
 * This function retrieves the timestamp from the RTC.
 *
 * @param rtc_show_dt Flag indicating whether to display the date and time in local time.
 * @return time_t The timestamp value.
 */
time_t rtc_app_get_timestamp(bool rtc_show_dt)
{
	struct tm rtcinfo = {0};
	time_t timestamp_now = 0;

	if (ds3231_get_time(&dev, &rtcinfo) != ESP_OK)
	{
		ESP_LOGE(RTC_APP_TAG, "Could not get time.");
	}
	else
	{
		rtcinfo.tm_year = rtcinfo.tm_year - 1900;
		rtcinfo.tm_mon = rtcinfo.tm_mon - 1;

		timestamp_now = mktime(&rtcinfo);

		if(llabs(timestamp_now - rtc_app_timestamp_ctrl) >= 2592000LL
				&& rtc_app_timestamp_ctrl != 0) // 1 month
		{
			rtc_app_set_timestamp(rtc_app_timestamp_ctrl);
			timestamp_now = rtc_app_timestamp_ctrl;
		}

		if(rtc_show_dt == true)
		{
			rtc_show_date_time(timestamp_now, RTC_CONFIG_TIMEZONE);
		}
	}

	return timestamp_now;
}

/**
 * @brief Gets the date and time from the RTC.
 *
 * This function retrieves the date and time from the RTC and stores it in the specified time structure.
 *
 * @param rtcinfo The pointer to the time structure to store the date and time.
 */
void rtc_app_get_date_time(struct tm* rtcinfo)
{
	if(rtcinfo == NULL)
	{
		ESP_LOGE(RTC_APP_TAG, "pointers is null");
	}
	else if (ds3231_get_time(&dev, rtcinfo) != ESP_OK)
	{
		ESP_LOGE(RTC_APP_TAG, "Could not get time.");
		LOG_DBG_ERROR(RTC_APP_TAG, "get_hw_rtc_error");
	}
}

/**
 * @brief Displays the date and time.
 *
 * This function displays the date and time based on the specified timestamp and time zone.
 *
 * @param timestamp_now The timestamp value.
 * @param time_z The time zone.
 */
void rtc_show_date_time(time_t timestamp_now, uint8_t time_z)
{
	char time_str[80];
	struct tm tminfo;
	time_t rawtime = (timestamp_now + (time_z * 3600));

	tminfo = *localtime ( &rawtime );

	strftime(time_str, sizeof(time_str), "%d/%m/%Y %H:%M:%S", &tminfo);
	ESP_LOGI(RTC_APP_TAG, "Timestamp %lld", timestamp_now);
	ESP_LOGI(RTC_APP_TAG, "Date time [%s] GMT (%d)", time_str, time_z);
}

/**
 * @brief Gets the date and time in string format.
 *
 * This function retrieves the date and time from the RTC and formats it as a string.
 *
 * @param timestamp_now The timestamp value.
 * @param str_out Pointer to the output string buffer.
 */
void rtc_app_get_str_date_time(time_t timestamp_now, char* str_out)
{
	char buff[50] = {};
	time_t now = (timestamp_now + (RTC_CONFIG_TIMEZONE * 3600));

	strftime(buff, sizeof(buff), "%d/%m/%Y_%H:%M:%S", localtime(&now));
	strcpy(str_out, buff);
}
