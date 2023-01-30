/*
 * nvs_data.h
 *
 *  Created on: 18 de set de 2022
 *      Author: bruno
 */

#ifndef COMPONENTS_NVS_DATA_INCLUDE_NVS_DATA_H_
#define COMPONENTS_NVS_DATA_INCLUDE_NVS_DATA_H_

/**
 * @file nvs_data.h
 * @date Set 18, 2022
 * @brief class for direct access to flash memory (NVS)
*/

#include "esp_err.h"
#include <stdint.h>

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
esp_err_t nvs_data_set(const char* label_name, const char* key, const void* value, size_t length);

/**
 * @brief	returns the size of the content saved in the label.
 * @param	label_name - [in]: Namespace name. Maximum length is (NVS_KEY_NAME_MAX_SIZE-1) characters. Shouldn’t be empty
 * @param	key - [in]:  Key name. Maximum length is (NVS_KEY_NAME_MAX_SIZE-1) characters. Shouldn’t be empty.
 * @return
 * 	- returns the size of the label, in case of error returns 0.
 */
size_t nvs_data_get_size(const char* label_name, const char* key);

/**
 * @brief	read content in memory
 * @param	label_name - [in]: Namespace name. Maximum length is (NVS_KEY_NAME_MAX_SIZE-1) characters. Shouldn’t be empty
 * @param	key - [in]:  Key name. Maximum length is (NVS_KEY_NAME_MAX_SIZE-1) characters. Shouldn’t be empty.
 * @param 	out_value - [out]: The value to set.
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail to initialize
 */
esp_err_t nvs_data_get_blob(const char* label_name, const char* key, uint8_t* out_value);

#endif /* COMPONENTS_NVS_DATA_INCLUDE_NVS_DATA_H_ */
