/*
 * comm_app.h
 *
 *  Created on: 23 de jun. de 2022
 *      Author: brunolima
 */

#ifndef COMPONENTS_APPLICATIONS_INCLUDE_COMM_APP_H_
#define COMPONENTS_APPLICATIONS_INCLUDE_COMM_APP_H_

/**
 * @file common_parser.h
 * @date June 23, 2022
 * @brief communication class control application
*/

#include "project_config.h"

#include "esp_err.h"

/**
 * @brief	starts all communication modules
 * @param 	app_callback[in]: function used with return to main application class
 * @return
 * 	- true: success
 * 	- false: fail
 */
esp_err_t comm_app_init(const app_callback callback);

void comm_app_send_idp_pack(const char* idp_pack, comm_type communication);

void comm_app_wifi_config(char* wifi_ssid, char* wifi_pass);

#endif /* COMPONENTS_APPLICATIONS_INCLUDE_COMM_APP_H_ */
