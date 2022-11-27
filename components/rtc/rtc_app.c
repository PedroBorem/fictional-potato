/*
 * rtc_app.c
 *
 *  Created on: 27 de nov de 2022
 *      Author: bruno
 */

#include "rtc_app.h"
#include "rtc_ds3231.h"

#include <time.h>

#define RTC_SDA_PIN	10	//change pins
#define RTC_SCL_PIN	12	//change pins

static time_t calendar_to_timestamp(DS3231_Calendar_t calendar);
static DS3231_Calendar_t timestamp_to_calendar(time_t timestamp);

bool rtc_app_init(void)
{
	bool ret = false;
	esp_err_t err;

	i2c_config_t i2c_config =
	{
		.mode = I2C_MODE_MASTER,
		.sda_io_num = RTC_SDA_PIN,
		.scl_io_num = RTC_SCL_PIN,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = 400000,
	};

	err = i2c_param_config(I2C_NUM_0, &i2c_config);
	if(err == ESP_OK )
	{
		err = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
		if(err == ESP_OK )
		{
			DS3231_Cfg_t ds3231_cfg = ds3231_create(I2C_NUM_0);
			if(ds3231_cfg != NULL)
			{
				ret = true;
			}

			ds3231_delete(ds3231_cfg);
		}
	}

	return ret;
}

bool rtc_app_set_timestamp(time_t timestamp)
{
	bool ret = false;

	DS3231_Calendar_t calendar = timestamp_to_calendar(timestamp);
	DS3231_Cfg_t ds3231_cfg = ds3231_create(I2C_NUM_0);
	if(ds3231_cfg != NULL)
	{
		esp_err_t res = ds3231_set_calendar(ds3231_cfg, &calendar, pdMS_TO_TICKS(10));
		if(res == ESP_OK)
		{
			ret = true;
		}
	}

	ds3231_delete(ds3231_cfg);
	return ret;
}

time_t rtc_app_get_timestamp(void)
{
	esp_err_t res = ESP_FAIL;
	time_t time_ret = 0;

	DS3231_Calendar_t calendar = {};
	DS3231_Cfg_t ds3231_cfg = ds3231_create(I2C_NUM_0);

	if(ds3231_cfg != NULL)
	{
		res = ds3231_get_calendar(ds3231_cfg, &calendar, pdMS_TO_TICKS(10));
		if (res == ESP_OK)
		{
			time_ret = calendar_to_timestamp(calendar);
		}
	}

	ds3231_delete(ds3231_cfg);
	return time_ret;
}

static time_t calendar_to_timestamp(DS3231_Calendar_t calendar)
{
	time_t timestamp = 0;

	struct tm tm_var = {
			.tm_sec = calendar.seconds,
			.tm_min = calendar.minutes,
			.tm_hour = calendar.hour,
			.tm_mday = calendar.day_of_month,
			.tm_mon = calendar.month,
			.tm_year = calendar.year,
			.tm_wday = calendar.day_of_week,
	};

	timestamp = mktime(&tm_var);

	return timestamp;
}

static DS3231_Calendar_t timestamp_to_calendar(time_t timestamp)
{
	struct tm tm_var = {};

	tm_var = *localtime(&timestamp);

	DS3231_Calendar_t calendar = {
				.seconds = tm_var.tm_sec,
				.minutes = tm_var.tm_min,
				.hour = tm_var.tm_hour,
				.day_of_month = tm_var.tm_mday,
				.month = tm_var.tm_mon,
				.year = tm_var.tm_year,
				.day_of_week = tm_var.tm_wday,
	};

	return calendar;
}
