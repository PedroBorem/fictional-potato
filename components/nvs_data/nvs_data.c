/*
 * nvs.c
 *
 *  Created on: 15 de jun. de 2022
 *      Author: brunolima
 */

/**
 * @file nvs_data.c
 * @brief nvs data
*/

/* Self include */
#include "nvs_data.h"

/* NVS include */
#include "nvs_flash.h"
#include "nvs.h"

/**\addtogroup components
 * @{
 *
 */

/**\addtogroup nvs_data
 * @{
 *
 */

#define NVS_DATA_TAG	"nvs_data"

/* Private definitions */


esp_err_t nvs_data_init(void)
{
	esp_err_t err = ESP_FAIL;
	nvs_handle_t handle = (nvs_handle_t)NULL;

	err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		// NVS partition was truncated and needs to be erased
		// Retry nvs_flash_init
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}

	if (err == ESP_OK)
	{
		err = nvs_open( NVS_LABEL_CONFIG, NVS_READWRITE, &handle );
		if( (err != ESP_OK) || (handle == (nvs_handle_t)NULL) )
		{
			ESP_LOGE( NVS_DATA_TAG, "%s,failed to open label : %s", __func__, NVS_LABEL_CONFIG);
			err = ESP_FAIL;
		}

		nvs_close( handle );
		handle = (nvs_handle_t)NULL;
		//TODO: initialize for scheduling and history
	}
	else
	{
		ESP_LOGE( NVS_DATA_TAG, "%s, failed to initialize flash", __func__);
	}

	ESP_ERROR_CHECK( err );
	return err;
}

esp_err_t nvs_data_set(const char* label_name, const char* key, const uint8_t* value, size_t length)
{
	esp_err_t err = ESP_FAIL;
	nvs_handle_t handle = (nvs_handle_t)NULL;

	err = nvs_open(label_name, NVS_READWRITE, &handle);
	if (err == ESP_OK)
	{
		err = nvs_set_blob(handle, key, value, length);
		if(err == ESP_OK)
		{
			 err = nvs_commit(handle);
		}
	}

	nvs_close(handle);
	handle = (nvs_handle_t)NULL;

	return err;
}

esp_err_t nvs_data_get(const char* label_name, const char* key, uint8_t* out_value, size_t *length)
{
	esp_err_t err = ESP_FAIL;
	nvs_handle_t handle = (nvs_handle_t)NULL;

	err = nvs_open(label_name, NVS_READWRITE, &handle);
	if (err == ESP_OK)
	{
		err = nvs_get_blob(handle, key, out_value, length);
		if(err == ESP_OK)
		{
			err = nvs_commit(handle);
		}
	}

	nvs_close(handle);
	handle = (nvs_handle_t)NULL;

	return err;
}


/**@}*/ 	//nvs_data
/** @}*/	//components
