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

/**
 * name of configuration label created on nvs partition
 *
 */
#define NVS_LABEL_CONFIG 	"nvs_config"

/**
 * name of the configuration key created in the label NVS_LABEL_CONFIG
 *
 */
#define NVS_KEY_CONFIG 		"pivot_config"
//TODO: definitions for scheduling and history

/**
 * @brief	start NVS partition from memory
 * @return	ESP_OK: success
 * 			ESP_FAIL: fail to initialize
 */
esp_err_t nvs_data_init(void);

/**
 * @brief	save content in memory
 * @param	label_name - [in]: Namespace name. Maximum length is (NVS_KEY_NAME_MAX_SIZE-1) characters. Shouldn’t be empty
 * @param	key - [in]:  Key name. Maximum length is (NVS_KEY_NAME_MAX_SIZE-1) characters. Shouldn’t be empty.
 * @param 	value - [in]: The value to set.
 * @param	length – [in]: length of binary value to set, in bytes; Maximum length is 508000 bytes or (97.6% of the partition size - 4000)
 * 			bytes whichever is lower.
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail to initialize
 */
esp_err_t nvs_data_set(const char* label_name, const char* key, const uint8_t* value, size_t length);

/**
 * @brief	read content in memory
 * @param	label_name - [in]: Namespace name. Maximum length is (NVS_KEY_NAME_MAX_SIZE-1) characters. Shouldn’t be empty
 * @param	key - [in]:  Key name. Maximum length is (NVS_KEY_NAME_MAX_SIZE-1) characters. Shouldn’t be empty.
 * @param 	out_value - [out]: The value to set.
 * @param	length – [out]: length of binary value to set, in bytes; Maximum length is 508000 bytes or (97.6% of the partition size - 4000)
 * 			bytes whichever is lower.
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail to initialize
 */
esp_err_t nvs_data_get(const char* label_name, const char* key, uint8_t* out_value, size_t* length);

#endif /* COMPONENTS_NVS_INCLUDE_NVS_DATA_H_ */
