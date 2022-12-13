/*
 * rtc_app.h
 *
 *  Created on: 27 de nov de 2022
 *      Author: bruno
 */

#ifndef COMPONENTS_RTC_INCLUDE_RTC_APP_H_
#define COMPONENTS_RTC_INCLUDE_RTC_APP_H_

#include "project_config.h"

bool rtc_app_init(void);

bool rtc_app_set_timestamp(time_t timestamp);

time_t rtc_app_get_timestamp(void);

#endif /* COMPONENTS_RTC_INCLUDE_RTC_APP_H_ */
