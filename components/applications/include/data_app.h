/*
 * data_app.h
 *
 *  Created on: 18 de set de 2022
 *      Author: bruno
 */

#ifndef COMPONENTS_APPLICATIONS_INCLUDE_DATA_APP_H_
#define COMPONENTS_APPLICATIONS_INCLUDE_DATA_APP_H_

/**
 * @file data_app.h
 * @date June 15, 2022
 * @brief memory control application
*/

#include "esp_err.h"

typedef enum
{
	DATA_TYPE_ACTIONS = 0,
	DATA_TYPE_PIVOT_CONFIG,
	DATA_TYPE_NETWORK_CONFIG,
	DATA_TYPE_ECO_MODE_CONFIG,
	DATA_TYPE_SECTOR_CONFIG,
	DATA_TYPE_SCHEADULING_DATE,
	DATA_TYPE_SCHEADULING_ANGLE,
	DATA_TYPE_HISTORY,
	DATA_TYPE_OLD_HISTORY,
	DATA_TYPE_TIMESTAMP,
}data_type_t;

esp_err_t data_app_init(void);
esp_err_t data_app_save(data_type_t data_type, const void* data, size_t data_size);
esp_err_t data_app_load(data_type_t data_type, void* data);
esp_err_t data_app_delete(void* data_id);
void data_app_gen_scheduling_key(char* scheduling_id);
size_t data_app_get_data_size(const char* label_name);

#endif /* COMPONENTS_APPLICATIONS_INCLUDE_DATA_APP_H_ */
