/**
 * @file nvs_data.h
 * @brief Class for direct access to flash memory (NVS).
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
 * @brief Initializes the NVS data class.
 *
 * This function initializes the NVS data class by initializing the NVS flash memory.
 *
 * @return esp_err_t An error code indicating the success or failure of the initialization.
 */
esp_err_t nvs_data_init(void);

/**
 * @brief Sets the value in the NVS data.
 *
 * This function sets the value in the NVS data under the specified label and key.
 *
 * @param label_name The label name for the NVS data.
 * @param value The value to be stored.
 * @param length The length of the value.
 * @return esp_err_t An error code indicating the success or failure of the operation.
 */
esp_err_t nvs_data_set(const char* label_name, const void* value, size_t length);

/**
 * @brief Gets the size of the NVS data.
 *
 * This function retrieves the size of the NVS data under the specified label and key.
 *
 * @param label_name The label name for the NVS data.
 * @return size_t The size of the NVS data.
 */
size_t nvs_data_get_size(const char* label_name);

/**
 * @brief Gets the NVS data as a blob.
 *
 * This function retrieves the NVS data as a blob under the specified label and key.
 *
 * @param label_name The label name for the NVS data.
 * @param out_value The buffer to store the retrieved data.
 * @return esp_err_t An error code indicating the success or failure of the operation.
 */
esp_err_t nvs_data_get_blob(const char* label_name, uint8_t* out_value);

#endif /* COMPONENTS_NVS_DATA_INCLUDE_NVS_DATA_H_ */
