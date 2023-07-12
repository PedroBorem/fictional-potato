/**
 * @file nvs_data.c
 * @brief Class for direct access to flash memory (NVS).
 */

/* Self include */
#include "nvs_data.h"

/* NVS include */
#include "nvs_flash.h"
#include "nvs.h"

/* common lib */
#include "esp_log.h"
#include "string.h"

/* Private definitions ------------------------------------------- */
/**
 * @def NVS_DATA_TAG
 * @brief Tag used for logging related to NVS data.
 */
#define NVS_DATA_TAG "nvs_data"

/* Public method implementation ---------------------------------- */
esp_err_t nvs_data_init(void)
{
	esp_err_t err = ESP_FAIL;

	err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		// NVS partition was truncated and needs to be erased
		// Retry nvs_flash_init
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}

	ESP_ERROR_CHECK( err );
	return err;
}

esp_err_t nvs_data_set(const char* label_name, const char* key, const void* value, size_t length)
{
	esp_err_t err = ESP_FAIL;
	nvs_handle_t handle = (nvs_handle_t)NULL;

	err = nvs_open(label_name, NVS_READWRITE, &handle);
	if( (err != ESP_OK) || (handle == (nvs_handle_t)NULL) )
	{
		ESP_LOGE( NVS_DATA_TAG, "%s,failed to open label : %s", __func__, label_name);
		err = ESP_FAIL;
	}
	else if (err == ESP_OK)
	{
		err = nvs_set_blob(handle, key, value, length);
		if(err == ESP_OK)
		{
			 err = nvs_commit(handle);
			 ESP_LOGI(NVS_DATA_TAG, "%s, Saved successfully (%p)", __func__, value);
		}
		else
		{
			ESP_LOGE( NVS_DATA_TAG, "%s,failed to save (%p)", __func__, value);
		}
	}

	nvs_close(handle);

	return err;
}

size_t nvs_data_get_size(const char* label_name, const char* key)
{
	esp_err_t err = ESP_FAIL;
	nvs_handle_t handle = (nvs_handle_t)NULL;
	size_t required_size = 0;

	err = nvs_open(label_name, NVS_READWRITE, &handle);
	if (err == ESP_OK)
	{
		err = nvs_get_blob(handle, key, NULL, &required_size);
		if(err != ESP_OK)
		{
			required_size = 0;
		}
	}

	nvs_close(handle);

	return required_size;
}

esp_err_t nvs_data_get_blob(const char* label_name, const char* key, uint8_t* out_value)
{
	esp_err_t err = ESP_FAIL;
	nvs_handle_t handle = (nvs_handle_t)NULL;
	size_t required_size = 0;

	err = nvs_open(label_name, NVS_READWRITE, &handle);
	if (err == ESP_OK)
	{
		err = nvs_get_blob(handle, key, NULL, &required_size);
		if(err == ESP_OK)
		{
			uint32_t* run_time = malloc(required_size + sizeof(uint32_t));
			err = nvs_get_blob(handle, key, run_time, &required_size);
			if(err == ESP_OK)
			{
				memcpy(out_value, (uint8_t*)run_time, required_size);
			}
			else
			{
				ESP_LOGE( NVS_DATA_TAG, "%s,failed to get configuration", __func__);
			}
			free(run_time);
		}
		else
		{
			ESP_LOGE( NVS_DATA_TAG, "%s,failed to get configuration, invalid length", __func__);
		}
	}
	else
	{
		ESP_LOGE( NVS_DATA_TAG, "%s,failed to open label : %s", __func__, label_name);
	}

	nvs_close(handle);

	return err;
}

