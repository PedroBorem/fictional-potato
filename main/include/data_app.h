/*
 * data_app.h
 *
 *  Created on: 15 de jun. de 2022
 *      Author: brunolima
 */

#ifndef MAIN_INCLUDE_DATA_APP_H_
#define MAIN_INCLUDE_DATA_APP_H_

/**
 * @file data_app.h
 * @date June 15, 2022
 * @brief memory control application
*/

#include "project_config.h"

/**
 * @brief	start all memory modules
 * @param	app_callback - [in]: function used with return to main application class
 * @return
 * 	- true: success
 * 	- false: fail to initialize
 */
bool data_app_init(app_callback callback);

/**
 * @brief	sends the queue a request to save a new configuration
 * @param	config_in - [in]: pivot configuration structure
 * @param	config_length - [in]: structure size
 * @return
 * 	- true: success
 * 	- false: fail to save
 */
bool data_app_save_config(pivot_config config_in, size_t config_length);

/**
 * @brief	sends the queue a request to load a configuration
 * @param	config_out - [out]: pivot configuration structure
 * @param	config_length - [out]: structure size
 * @return
 * 	- true: success
 * 	- false: fail to save
 */
bool data_app_load_config(pivot_config* config_out, size_t* config_length);

/**
 * @brief	show configuration content in memory
 *
 * propagation of the nvs_config_show_current method
 *
 */
void data_app_show_config(void);

#endif /* MAIN_INCLUDE_DATA_APP_H_ */
