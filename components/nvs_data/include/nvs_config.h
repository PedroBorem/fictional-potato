/*
 * nvs_config.h
 *
 *  Created on: 15 de jun. de 2022
 *      Author: brunolima
 */

#ifndef COMPONENTS_NVS_DATA_INCLUDE_NVS_CONFIG_H_
#define COMPONENTS_NVS_DATA_INCLUDE_NVS_CONFIG_H_

/**
 * @file nvs_config.h
 * @date June 15, 2022
 * @brief nvs pivot configuration
*/

#include "project_config.h"

/**
 * @brief	validates the content of the pivo configuration and if there is no configuration, applies a default.
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail to initialize
 */
esp_err_t nvs_config_init(void);

/**
 * @brief	save content in memory
 * @param	config - [in]: pivot configuration structure
 * @param	config_length - [in]: structure size
 * @return
 * 	- ESP_OK: success saving
 * 	- ESP_FAIL: fail to save
 */
esp_err_t nvs_config_set(pivot_config config, size_t config_length);

/**
 * @brief	load content in memory
 * @param	config - [out]: pivot configuration structure
 * @param	config_length - [out]: structure size
 * @return
 * 	- ESP_OK: success loading
 * 	- ESP_FAIL: fail to load
 * 	- ESP_ERR_NVS_NOT_FOUND: empty memory
 */
esp_err_t nvs_config_get(pivot_config* out_config, size_t* config_length);

/**
 * @brief	show configuration content in memory
 *
 */
void nvs_config_show_current(void);

#endif /* COMPONENTS_NVS_DATA_INCLUDE_NVS_CONFIG_H_ */
