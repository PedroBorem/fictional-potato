/*
 * eco_mode.h
 *
 *  Created on: 15 de out. de 2023
 *      Author: soil-dev
 */

#ifndef MAIN_INCLUDE_ECO_MODE_H_
#define MAIN_INCLUDE_ECO_MODE_H_

#include "project_config.h"

void eco_mode_start(eco_mode_config current_eco_mode);
void eco_mode_stop(void);
void eco_mode_register_callback(const app_callback callback);

#endif /* MAIN_INCLUDE_ECO_MODE_H_ */
