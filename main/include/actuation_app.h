/*
 * actuation_app.h
 *
 *  Created on: 22 de jul. de 2022
 *      Author: brunolima
 */

#ifndef MAIN_INCLUDE_ACTUATION_APP_H_
#define MAIN_INCLUDE_ACTUATION_APP_H_

/**
 * @file actuation_app.h
 * @date June 23, 2022
 * @brief actuation control class
*/

#include "project_config.h"

bool actuation_app_init(const app_callback callback);

void actuation_app_set_config(const pivot_config config_in);

void actuation_app_get_config(pivot_config* config_out, size_t config_size);

#endif /* MAIN_INCLUDE_ACTUATION_APP_H_ */
