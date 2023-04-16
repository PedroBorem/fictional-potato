/*
 * data_app.h
 *
 *  Created on: 18 de set de 2022
 *      Author: bruno
 */

#ifndef MAIN_INCLUDE_DATA_APP_H_
#define MAIN_INCLUDE_DATA_APP_H_

/**
 * @file data_app.h
 * @date June 15, 2022
 * @brief memory control application
*/

#include "esp_err.h"

typedef enum
{
	data_scheduling_date = 0,
	data_scheduling_angle,
}data_scheduling_type;

/**
 * @brief	start all memory modules
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail to initialize
 */
esp_err_t data_app_init(void);

/**
 * @brief	sends the queue a request to save a new action
 * @param 	value - [in]: content you want to save in memory.
 * @param 	size - [in]: size of content
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail to save
 */
esp_err_t data_app_save_actions(const void* value, size_t size);

/**
 * @brief	sends the queue a request to load a current action.
 * @param 	out_value - [in]: return pointer with the value read from the label.
 * @param 	size - [in]: size allocated for pointer(out_value).
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail to save
 */
esp_err_t data_app_load_actions(void* out_value, size_t size);

/**
 * @brief	sends the queue a request to save a new configuration
 * @param 	value - [in]: content you want to save in memory.
 * @param 	size - [in]: size of content
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail to save
 */
esp_err_t data_app_save_config(const void* value, size_t size);

/**
 * @brief	sends the queue a request to load a configuration.
 * @param 	out_value - [in]: return pointer with the value read from the label.
 * @param 	size - [in]: size allocated for pointer(out_value).
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail to save
 */
esp_err_t data_app_load_config(void* out_value, size_t size);

esp_err_t data_app_save_scheduling(data_scheduling_type scheduling_type, const void* value, size_t size);

esp_err_t data_app_load_scheduling(data_scheduling_type scheduling_type, void* out_value, size_t size);

esp_err_t data_app_delete_scheduling(data_scheduling_type scheduling_type, char* scheduling_id);

/**
 * @brief Gets the data size for the informed key.
 * @param 	key - access key defined in project_config.h
 * @return
 *  - 0: in case of errors
 *  - Data size.
 */
size_t data_app_get_data_size(const char* label_name, const char* key);

#endif /* MAIN_INCLUDE_DATA_APP_H_ */
