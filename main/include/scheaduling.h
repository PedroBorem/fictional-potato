/*
 * scheaduling.h
 *
 *  Created on: 31 de jul. de 2023
 *      Author: soil-dev
 */

#ifndef MAIN_INCLUDE_SCHEADULING_H_
#define MAIN_INCLUDE_SCHEADULING_H_

#include "project_config.h"

void scheduling_start(pivot_scheduling_date* in_scheduling_date, pivot_scheduling_angle* in_scheduling_angle);
void scheduling_stop(void);
void scheduling_register_callback(const app_callback callback);

#endif /* MAIN_INCLUDE_SCHEADULING_H_ */
