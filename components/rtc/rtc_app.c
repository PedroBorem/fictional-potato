/*
 * rtc_app.c
 *
 *  Created on: 27 de nov de 2022
 *      Author: bruno
 */

#include "rtc_app.h"
#include "rtc_ds3231.h"

#include <time.h>

#define RTC_APP_TAG "rtc_app"

#define RTC_SDA_PIN	36
#define RTC_SCL_PIN	37

#define RTC_CONFIG_TIMEZONE

static i2c_dev_t dev;

bool rtc_app_init(void)
{
	// Initialize RTC
	if( ds3231_init_desc(&dev, I2C_NUM_1, RTC_SDA_PIN, RTC_SCL_PIN) == ESP_OK)
	{
		return true;
	}

	return false;
}

bool rtc_app_set_timestamp(time_t timestamp)
{
	bool ret = false;

	struct tm time = *localtime(&timestamp); //obs time + 1900
	time.tm_year += 1900;

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

	return ret;
}

time_t rtc_app_get_timestamp(void)
{
	struct tm rtcinfo = {};
	time_t timestamp_now = 0;

	if (ds3231_get_time(&dev, &rtcinfo) != ESP_OK)
	{
		ESP_LOGE(RTC_APP_TAG, "Could not get time.");

	}
	else
	{
		ESP_LOGI(RTC_APP_TAG, "%04d-%02d-%02d %02d:%02d:%02d",
			rtcinfo.tm_year, rtcinfo.tm_mon + 1,
			rtcinfo.tm_mday, rtcinfo.tm_hour, rtcinfo.tm_min, rtcinfo.tm_sec);

		timestamp_now = mktime(&rtcinfo);
	}

	return timestamp_now;
}

