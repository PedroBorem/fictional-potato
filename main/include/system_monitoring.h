/*
 * system_monitoring.h
 *
 *  Created on: 4 de ago. de 2023
 *      Author: soil-dev
 */

#ifndef MAIN_INCLUDE_SYSTEM_MONITORING_H_
#define MAIN_INCLUDE_SYSTEM_MONITORING_H_

#include "project_config.h"

void system_monitoring_start(void);
void system_monitoring_stop(void);
void system_monitoring_register_callback(const app_callback callback);

#endif /* MAIN_INCLUDE_SYSTEM_MONITORING_H_ */
