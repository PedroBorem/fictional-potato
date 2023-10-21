/*
 * scheaduling.h
 *
 *  Created on: 31 de jul. de 2023
 *      Author: soil-dev
 */

#ifndef MAIN_INCLUDE_SCHEDULING_H_
#define MAIN_INCLUDE_SCHEDULING_H_

#include "project_config.h"

void scheduling_start(idp_type scheduling_idp, void* scheduling_data);
void scheduling_stop(idp_type scheduling_idp);
void scheduling_register_callback(const app_callback callback);

#endif /* MAIN_INCLUDE_SCHEDULING_H_ */
