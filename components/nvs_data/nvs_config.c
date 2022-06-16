/*
 * nvs_config.c
 *
 *  Created on: 15 de jun. de 2022
 *      Author: brunolima
 */

/* Self include */
#include "nvs_config.h"

/* NVS include */
#include "nvs.h"
#include "nvs_data.h"

/**\addtogroup components
 * @{
 *
 */

/**\addtogroup nvs_config
 * @{
 *
 */

#define NVS_CONFIG_TAG				"nvs_config"
#define NVS_DEFAULT_PERCENTIMETER 	(0)

static pivot_config pivot_current_config = {};
static size_t pivot_config_length = 0;

esp_err_t nvs_config_init(void)
{
	esp_err_t err = ESP_FAIL;

	pivot_config pivot_default_config = {PIVOT_OFF,
										PIVOT_ADVANCE,
										PIVOT_DRY,
										NVS_DEFAULT_PERCENTIMETER};

	err = nvs_config_get(&pivot_current_config, &pivot_config_length);

	if(err == ESP_ERR_NVS_NOT_FOUND || pivot_config_length < sizeof(pivot_config))
	{
		err = nvs_config_set(pivot_default_config, sizeof(pivot_default_config));
		if(err == ESP_OK)
		{
			err = nvs_config_get(&pivot_current_config, &pivot_config_length);
		}
	}

	return err;
}

esp_err_t nvs_config_set(pivot_config config, size_t config_length)
{
	esp_err_t err = ESP_FAIL;

	err = nvs_data_set(NVS_LABEL_CONFIG, NVS_KEY_CONFIG, (uint8_t*)&config, config_length);

	if(err == ESP_OK)
	{
		memcpy(&pivot_current_config, &config, sizeof(pivot_current_config));
		nvs_config_show_current();
	}

	return err;
}

esp_err_t nvs_config_get(pivot_config* out_config, size_t* config_length)
{
	esp_err_t err = ESP_FAIL;

	err = nvs_data_get(NVS_LABEL_CONFIG, NVS_KEY_CONFIG, (uint8_t*)out_config, config_length);

	return err;
}

void nvs_config_show_current(void)
{
	LOG_DATA(NVS_CONFIG_TAG, "\n ------ NVS Current Config ------");
	LOG_DATA(NVS_CONFIG_TAG, " Power state: %d", pivot_current_config.power_state);
	LOG_DATA(NVS_CONFIG_TAG, " Advance mode: %d", pivot_current_config.advance_mode);
	LOG_DATA(NVS_CONFIG_TAG, " Watering state: %d", pivot_current_config.watering_state);
	LOG_DATA(NVS_CONFIG_TAG, " Percentimeter %.3d %%", pivot_current_config.percentimeter);
	LOG_DATA(NVS_CONFIG_TAG, " --------------------------------\n");
}

/**@}*/ 	//nvs_data
/** @}*/	//components

