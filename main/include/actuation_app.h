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

/**
 * @brief	starts the tasks of reading and actuation of the I/O ports
 * @param	app_callback - [in]: function used with return to main application class
 * @return
 * 	- true: success
 * 	- false: fail to initialize
 */
bool actuation_app_init(const app_callback callback);

/**
 * @brief	apply the configuration to the drives
 * @param	config_in - [in]: function used with return to main application class
 */
void actuation_app_set_config(const pivot_config config_in, bool alert_change);

/**
 * @brief	read current status on IO ports
 * @param	config_out - [out]: pivot configuration structure
 * @param	config_length - [out]: structure size
 */
void actuation_app_get_config(pivot_config* config_out, size_t config_size);

void actuation_app_set_pump(bool pump_state);

void actuation_app_shutdown(void);

#endif /* MAIN_INCLUDE_ACTUATION_APP_H_ */
