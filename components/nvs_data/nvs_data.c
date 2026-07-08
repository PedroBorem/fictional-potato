/**
 * @file nvs_data.c
 * @brief Class for direct access to flash memory (NVS).
 *
 * This file contains the implementation of the NVS Data class, which provides
 * functions for direct access to flash memory using the Non-Volatile Storage (NVS) API.
 */

/* Self include */
#include "nvs_data.h"

/* NVS include */
#include "nvs_flash.h"
#include "nvs.h"
#include "log.h"

/* common lib */
#include "string.h"

/* Private definitions ------------------------------------------- */
/**
 * @def NVS_DATA_TAG
 * @brief Tag used for logging related to NVS data.
 */
#define NVS_DATA_TAG "nvs_data"

/* Public method implementation ---------------------------------- */
/**
 * @brief Initializes the NVS data class.
 *
 * This function initializes the NVS data class by initializing the NVS flash memory.
 *
 * @return esp_err_t An error code indicating the success or failure of the initialization.
 */
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

	ESP_ERROR_CHECK(err);
	return err;
}

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
esp_err_t nvs_data_set(const char* label_name, const void* value, size_t length)
{
	esp_err_t err = ESP_FAIL;
	nvs_handle_t handle = (nvs_handle_t)NULL;

	err = nvs_open(label_name, NVS_READWRITE, &handle);
	if ((err != ESP_OK) || (handle == (nvs_handle_t)NULL))
	{
		LOG_ERROR(NVS_DATA_TAG, "NVS", "%s, failed to open label: %s", __func__, label_name);
		err = ESP_FAIL;
	}
	else if (err == ESP_OK)
	{
		err = nvs_set_blob(handle, label_name, value, length);
		if (err == ESP_OK)
		{
			err = nvs_commit(handle);
			LOG_NVS(NVS_DATA_TAG, "%s, saved successfully (%p)", __func__, value);
		}
		else
		{
			LOG_ERROR(NVS_DATA_TAG, "NVS", "%s, failed to save (%p)", __func__, value);
		}
	}

	nvs_close(handle);

	return err;
}

esp_err_t nvs_data_erase(const char *label_name)
{
	esp_err_t err;
	nvs_handle_t handle = (nvs_handle_t)NULL;

	if (label_name == NULL)
	{
		return ESP_ERR_INVALID_ARG;
	}

	err = nvs_open(label_name, NVS_READWRITE, &handle);
	if (err == ESP_ERR_NVS_NOT_FOUND)
	{
		return ESP_OK;
	}
	if (err != ESP_OK)
	{
		return err;
	}

	err = nvs_erase_key(handle, label_name);
	if (err == ESP_OK)
	{
		err = nvs_commit(handle);
	}
	else if (err == ESP_ERR_NVS_NOT_FOUND)
	{
		err = ESP_OK;
	}

	nvs_close(handle);
	return err;
}

/**
 * @brief Gets the size of the NVS data.
 *
 * This function retrieves the size of the NVS data under the specified label and key.
 *
 * @param label_name The label name for the NVS data.
 * @return size_t The size of the NVS data.
 */
size_t nvs_data_get_size(const char* label_name)
{
	esp_err_t err = ESP_FAIL;
	nvs_handle_t handle = (nvs_handle_t)NULL;
	size_t required_size = 0;

	err = nvs_open(label_name, NVS_READWRITE, &handle);
	if (err == ESP_OK)
	{
		err = nvs_get_blob(handle, label_name, NULL, &required_size);
		if (err != ESP_OK)
		{
			required_size = 0;
		}
	}

	nvs_close(handle);

	return required_size;
}

/**
 * @brief Gets the NVS data as a blob.
 *
 * This function retrieves the NVS data as a blob under the specified label and key.
 *
 * @param label_name The label name for the NVS data.
 * @param out_value The buffer to store the retrieved data.
 * @return esp_err_t An error code indicating the success or failure of the operation.
 */
esp_err_t nvs_data_get_blob(const char* label_name, uint8_t* out_value)
{
	esp_err_t err = ESP_FAIL;
	nvs_handle_t handle = (nvs_handle_t)NULL;
	size_t required_size = 0;

	err = nvs_open(label_name, NVS_READWRITE, &handle);
	if (err == ESP_OK)
	{
		err = nvs_get_blob(handle, label_name, NULL, &required_size);
		if (err == ESP_OK)
		{
			uint32_t* run_time = malloc(required_size + sizeof(uint32_t));
			err = nvs_get_blob(handle, label_name, run_time, &required_size);
			if (err == ESP_OK)
			{
				memcpy(out_value, (uint8_t*)run_time, required_size);
			}
			else
			{
					LOG_ERROR(NVS_DATA_TAG, "NVS", "%s, failed to get configuration", __func__);
			}
			free(run_time);
		}
		else
		{
			LOG_ERROR(NVS_DATA_TAG, "NVS", "%s, failed to get configuration, invalid length (%s)", __func__, label_name);
		}
	}
	else
	{
		LOG_ERROR(NVS_DATA_TAG, "NVS", "%s, failed to open label: %s", __func__, label_name);
	}

	nvs_close(handle);

	return err;
}
