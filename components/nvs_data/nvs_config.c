/*
 * nvs_config.c
 *
 *  Created on: 15 de jun. de 2022
 *      Author: brunolima
 */

/* Self include */
#include "nvs_config.h"

/* NVS include */
#include "nvs_data.h"

/**\addtogroup components
 * @{
 *
 */

/**\addtogroup nvs_config
 * @{
 *
 */

esp_err_t nvs_config_init(void)
{
	esp_err_t err = ESP_FAIL;

	return err;
}

esp_err_t nvs_config_set(pivot_config config, size_t config_length)
{
	esp_err_t err = ESP_FAIL;

	err = nvs_data_set(NVS_LABEL_CONFIG, NVS_KEY_CONFIG, (uint8_t*)config, config_length);

	return err;
}

esp_err_t nvs_config_get(pivot_config* out_config, size_t* config_length)
{
	esp_err_t err = ESP_FAIL;

	err = nvs_data_get(NVS_LABEL_CONFIG, NVS_KEY_CONFIG, (uint8_t*)out_config, config_length);

	return err;
}


/**@}*/ 	//nvs_data
/** @}*/	//components

