/*
 * comm_app.h
 *
 *  Created on: 23 de jun. de 2022
 *      Author: brunolima
 */

#ifndef MAIN_INCLUDE_COMM_APP_H_
#define MAIN_INCLUDE_COMM_APP_H_

/**
 * @file common_parser.h
 * @date June 23, 2022
 * @brief communication class control application
*/

#include "project_config.h"

/**
 * @brief	starts all communication modules
 * @param 	app_callback[in]: function used with return to main application class
 * @return
 * 	- true: success
 * 	- false: fail
 */
bool comm_app_init(const app_callback callback);

/**
 * @brief	starts all communication modules
 * @return 	angle: returns the current angle (degrees) received
 */
uint16_t comm_app_get_degree(void);

/**
 * @brief	send events over communication interfaces
 * @param 	pivot_status[in]:  configuration structure to send
 */
void comm_app_send_event(pivot_actions pivot_status);

void comm_app_set_config(const pivot_config config);

void comm_app_set_actions(const pivot_actions action, const pivot_config config, uint16_t start_angle, uint16_t end_angle);

#endif /* MAIN_INCLUDE_COMM_APP_H_ */
