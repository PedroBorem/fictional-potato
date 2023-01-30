/*
 * data_app.c
 *
 *  Created on: 18 de set de 2022
 *      Author: bruno
 */

/**
 * @file data_app.c
 * @date Set 18, 2022
 * @brief memory control application
*/

/* Self include */
#include "data_app.h"

/* NVS include */
#include "nvs_data.h"

/* Project include */
#include "esp_log.h"

/**\addtogroup main
 * @{
 *
 */

/**\addtogroup data_app
 * @{
 *
 */

/* Private definitions ------------------------------------------- */
/**
 * class log tag
 *
 */
#define DATA_APP_TAG			"data_app"

/**
 * NVS access key
 *
 */
#define DATA_APP_NAMESPACE		"msf_app_data"

/* Public methods ------------------------------------------------ */
esp_err_t data_app_init(void)
{
	esp_err_t err = ESP_FAIL;

	err = nvs_data_init();
	if(err == ESP_OK)
	{
		ESP_LOGI( DATA_APP_TAG, "%s, data application started successfully", __func__);
	}
	else
	{
		ESP_LOGE( DATA_APP_TAG, "%s, failed to start data application", __func__);
	}

	return err;
}

esp_err_t data_app_save_config(const char* key, const void* value, size_t size)
{
	esp_err_t ret = ESP_FAIL;

	if(key != NULL && value != NULL)
	{
		ret = nvs_data_set(DATA_APP_NAMESPACE, key, value, size);
	}

	return ret;
}

esp_err_t data_app_load_config(const char* key, void* out_value, size_t size)
{
	esp_err_t ret = ESP_FAIL;

	size_t required_size = nvs_data_get_size(DATA_APP_NAMESPACE, key);
	if(size < required_size)
	{
		ESP_LOGE( DATA_APP_TAG, "%s, buffer size smaller than label size", __func__);
	}
	else
	{
		ret = nvs_data_get_blob(DATA_APP_NAMESPACE, key, out_value);
	}

	return ret;
}

size_t data_app_get_data_size(const char* key){
    return nvs_data_get_size(DATA_APP_NAMESPACE, key);
}

/**@}*/ 	//data_app
/** @}*/	//main
