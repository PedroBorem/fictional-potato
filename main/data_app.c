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
#include "project_config.h"

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
 * NVS access space
 *
 */
#define DATA_APP_LABEL_ACTION	"space_action"
#define DATA_APP_LABEL_CONFIG	"space_config"

/**
 * NVS access label
 *
 */
#define DATA_KEY_ACTIONS	"label_actions"
#define DATA_KEY_CONFIG		"label_config"

/* Public methods ------------------------------------------------ */
esp_err_t data_app_init(void)
{
	esp_err_t err = ESP_FAIL;

	const pivot_actions default_action = {
			.power_state = PIVOT_OFF,
			.rotation = PIVOT_CCW,
			.watering_state = PIVOT_DRY,
			.percentimeter = 0,
	};

	const pivot_config default_config = {
			.pivot_id = "soil",
			.gprs_id = "soil",
			.contactor = CONTACTOR_NA,
			.pressure_switch = PRESSURE_SWITCH_NA,
			.pressurization_time = 30,
			.on_off_time = 01,
			.eco_mode = false,
			.sector_enabled = false,
	};

	err = nvs_data_init();
	if(err == ESP_OK)
	{
		if(nvs_data_get_size(DATA_APP_LABEL_ACTION, DATA_KEY_ACTIONS) == 0)
		{
			data_app_save_actions(&default_action, sizeof(default_action));
		}

		if(nvs_data_get_size(DATA_APP_LABEL_CONFIG, DATA_KEY_CONFIG) == 0)
		{
			data_app_save_config(&default_config, sizeof(default_config));
		}

		ESP_LOGI( DATA_APP_TAG, "%s, data application started successfully", __func__);
	}
	else
	{
		ESP_LOGE( DATA_APP_TAG, "%s, failed to start data application", __func__);
	}

	return err;
}

esp_err_t data_app_save_actions(const void* value, size_t size)
{
	esp_err_t ret = ESP_FAIL;

	if(value != NULL)
	{
		ret = nvs_data_set(DATA_APP_LABEL_ACTION, DATA_KEY_ACTIONS, value, size);
	}

	return ret;
}

esp_err_t data_app_load_actions(void* out_value, size_t size)
{
	esp_err_t ret = ESP_FAIL;

	size_t required_size = nvs_data_get_size(DATA_APP_LABEL_ACTION, DATA_KEY_ACTIONS);
	if(size < required_size)
	{
		ESP_LOGE( DATA_APP_TAG, "%s, buffer size smaller than label size", __func__);
	}
	else
	{
		ret = nvs_data_get_blob(DATA_APP_LABEL_ACTION, DATA_KEY_ACTIONS, out_value);
	}

	return ret;
}

esp_err_t data_app_save_config(const void* value, size_t size)
{
	esp_err_t ret = ESP_FAIL;

	if(value != NULL)
	{
		ret = nvs_data_set(DATA_APP_LABEL_CONFIG, DATA_KEY_CONFIG, value, size);
	}

	return ret;
}

esp_err_t data_app_load_config(void* out_value, size_t size)
{
	esp_err_t ret = ESP_FAIL;

	size_t required_size = nvs_data_get_size(DATA_APP_LABEL_CONFIG, DATA_KEY_CONFIG);
	if(size < required_size)
	{
		ESP_LOGE( DATA_APP_TAG, "%s, buffer size smaller than label size", __func__);
	}
	else
	{
		ret = nvs_data_get_blob(DATA_APP_LABEL_CONFIG, DATA_KEY_CONFIG, out_value);
	}

	return ret;
}

size_t data_app_get_data_size(const char* label_name, const char* key)
{
    return nvs_data_get_size(label_name, key);
}

/**@}*/ 	//data_app
/** @}*/	//main
