/*
 * data_app.h
 *
 *  Created on: 15 de jun. de 2022
 *      Author: brunolima
 */

#ifndef MAIN_INCLUDE_DATA_APP_H_
#define MAIN_INCLUDE_DATA_APP_H_


#include "project_config.h"

/*
 * @brief: function used with callback in load configuration.
 * */
typedef void (*data_app_callback)(app_call_states state);

bool data_app_init(data_app_callback app_callback);
bool data_app_save_config(pivot_config config_in, size_t config_length);
bool data_app_load_config(pivot_config* config_out, size_t* config_length);


#endif /* MAIN_INCLUDE_DATA_APP_H_ */
