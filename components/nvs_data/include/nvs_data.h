/*
 * nvs.h
 *
 *  Created on: 15 de jun. de 2022
 *      Author: brunolima
 */

#ifndef COMPONENTS_NVS_INCLUDE_NVS_DATA_H_
#define COMPONENTS_NVS_INCLUDE_NVS_DATA_H_

/**
 * @file nvs_data.h
 * @date June 15, 2022
 * @brief nvs data
*/

#include "project_config.h"

/* Public definitions */
#define NVS_LABEL_CONFIG 	"nvs_config"
#define NVS_KEY_CONFIG 		"pivot_config"
//TODO: definitions for scheduling and history

esp_err_t nvs_data_init(void);

esp_err_t nvs_data_set(const char* label_name, const char* key, const uint8_t* value, size_t length);

esp_err_t nvs_data_get(const char* label_name, const char* key, uint8_t* out_value, size_t* length);

#endif /* COMPONENTS_NVS_INCLUDE_NVS_DATA_H_ */
