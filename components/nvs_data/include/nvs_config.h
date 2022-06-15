/*
 * nvs_config.h
 *
 *  Created on: 15 de jun. de 2022
 *      Author: brunolima
 */

#ifndef COMPONENTS_NVS_DATA_INCLUDE_NVS_CONFIG_H_
#define COMPONENTS_NVS_DATA_INCLUDE_NVS_CONFIG_H_

#include "project_config.h"


esp_err_t nvs_config_init(void);


esp_err_t nvs_config_set(pivot_config config, size_t config_length);


esp_err_t nvs_config_get(pivot_config* out_config, size_t* config_length);


#endif /* COMPONENTS_NVS_DATA_INCLUDE_NVS_CONFIG_H_ */
