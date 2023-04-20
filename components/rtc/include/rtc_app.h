/*
 * rtc_app.h
 *
 *  Created on: 27 de nov de 2022
 *      Author: bruno
 */

#ifndef COMPONENTS_RTC_INCLUDE_RTC_APP_H_
#define COMPONENTS_RTC_INCLUDE_RTC_APP_H_

#include "project_config.h"

#include <time.h>

bool rtc_app_init(void);

bool rtc_app_set_timestamp(time_t timestamp);

time_t rtc_app_get_timestamp(bool rtc_show_dt);

void rtc_app_get_date_time(struct tm* rtcinfo);

void rtc_show_date_time(time_t timestamp_now, uint8_t time_z);

#endif /* COMPONENTS_RTC_INCLUDE_RTC_APP_H_ */
