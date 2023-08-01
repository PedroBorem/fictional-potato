/**
 * @file rtc_app.c
 * @brief RTC application implementation for managing date and time functionality.
 *
 * This file contains the implementation of the RTC application for managing date and time functionality.
 */

#include "rtc_app.h"
#include "rtc_ds3231.h"
#include "esp_log.h"

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
 * @var dev
 * @brief The RTC I2C device structure.
 */
static rtc_i2c_dev_t dev = {};

/* Public methods ----------------------------------- */

esp_err_t rtc_app_init(void)
{
	// Initialize RTC
	if( ds3231_init_desc(&dev, I2C_NUM_0, RTC_SDA_PIN, RTC_SCL_PIN) == ESP_OK)
	{
		return ESP_OK;
	}

	return ESP_FAIL;
}

bool rtc_app_set_timestamp(time_t timestamp)
{
	bool ret = false;

	if(timestamp > 1670123456) // date : 04/12/2022
	{
		struct tm time = *localtime(&timestamp); //obs time + 1900
		time.tm_year += 1900;
		time.tm_mon += 1;

		ESP_LOGI(RTC_APP_TAG, "timeinfo.tm_sec=%d",time.tm_sec);
		ESP_LOGI(RTC_APP_TAG, "timeinfo.tm_min=%d",time.tm_min);
		ESP_LOGI(RTC_APP_TAG, "timeinfo.tm_hour=%d",time.tm_hour);
		ESP_LOGI(RTC_APP_TAG, "timeinfo.tm_wday=%d",time.tm_wday);
		ESP_LOGI(RTC_APP_TAG, "timeinfo.tm_mday=%d",time.tm_mday);
		ESP_LOGI(RTC_APP_TAG, "timeinfo.tm_mon=%d",time.tm_mon);
		ESP_LOGI(RTC_APP_TAG, "timeinfo.tm_year=%d",time.tm_year);

		if (ds3231_set_time(&dev, &time) != ESP_OK)
		{
			ESP_LOGE(RTC_APP_TAG, "Could not set time.");
		}
		else
		{
			ret = true;
			ESP_LOGI(RTC_APP_TAG, "Set initial date time done");
		}
	}

	return ret;
}

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

		if(rtc_show_dt == true)
		{
			rtc_show_date_time(timestamp_now, RTC_CONFIG_TIMEZONE);
		}
	}

	return timestamp_now;
}

void rtc_app_get_date_time(struct tm* rtcinfo)
{
	if(rtcinfo == NULL)
	{
		ESP_LOGE(RTC_APP_TAG, "pointers is null");
	}
	else if (ds3231_get_time(&dev, rtcinfo) != ESP_OK)
	{
		ESP_LOGE(RTC_APP_TAG, "Could not get time.");
	}
}

void rtc_show_date_time(time_t timestamp_now, uint8_t time_z)
{
	char time_str[80];
	struct tm tminfo;
	time_t rawtime = (timestamp_now + (time_z*3600));

	tminfo = *localtime ( &rawtime );

	strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S %Z", &tminfo);
	ESP_LOGI(RTC_APP_TAG, "timestamp %lld [%s]", timestamp_now, time_str);
}
