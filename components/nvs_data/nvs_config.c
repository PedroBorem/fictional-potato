/*
 * nvs_config.c
 *
 *  Created on: 15 de jun. de 2022
 *      Author: brunolima
 */

/**
 * @file nvs_config.c
 * @date June 15, 2022
 * @brief nvs pivot configuration
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

/* Private definitions ------------------------------------------- */
#define NVS_CONFIG_TAG				"nvs_config"
#define NVS_DEFAULT_PERCENTIMETER 	(0)

/* Private variables --------------------------------------------- */
static pivot_config pivot_current_config = {};
static size_t pivot_config_length = 0;

/* Private methods declarations ---------------------------------- */
static bool nvs_config_validate(pivot_config config);

/* Public methods ------------------------------------------------ */
esp_err_t nvs_config_init(void)
{
	esp_err_t err = ESP_FAIL;

	pivot_config pivot_default_config ={PIVOT_OFF,
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
		else
		{
			ESP_LOGE( NVS_CONFIG_TAG, "%s,failed to apply default settings", __func__);
			err = ESP_FAIL;
		}
	}
	else if(err != ESP_OK)
	{
		ESP_LOGE( NVS_CONFIG_TAG, "%s,failed to apply default settings, error getting data", __func__);
		err = ESP_FAIL;
	}

	return err;
}

esp_err_t nvs_config_set(pivot_config config, size_t config_length)
{
	esp_err_t err = ESP_FAIL ;

	if(nvs_config_validate(config) && config_length > 0 )
	{
		err = nvs_data_set(NVS_LABEL_CONFIG, NVS_KEY_CONFIG, (uint8_t*)&config, config_length);
		if(err == ESP_OK)
		{
			memcpy(&pivot_current_config, &config, sizeof(pivot_current_config));
			nvs_config_show_current();
		}
		else
		{
			ESP_LOGE( NVS_CONFIG_TAG, "%s, failed to write to memory", __func__);
		}
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
	LOG_DATA(NVS_CONFIG_TAG, "");
	LOG_DATA(NVS_CONFIG_TAG, " ------ NVS Current Config ------");
	LOG_DATA(NVS_CONFIG_TAG, " Power state: %d", pivot_current_config.power_state);
	LOG_DATA(NVS_CONFIG_TAG, " Advance mode: %d", pivot_current_config.advance_mode);
	LOG_DATA(NVS_CONFIG_TAG, " Watering state: %d", pivot_current_config.watering_state);
	LOG_DATA(NVS_CONFIG_TAG, " Percentimeter %.3d %%", pivot_current_config.percentimeter);
	LOG_DATA(NVS_CONFIG_TAG, " --------------------------------\n");
}

/* Private methods ----------------------------------------------- */
/**
 * @brief	configuration buffer validation
 * @param	config - [in]: pivot configuration structure
 * @return
 * 	- true: configuration is correct
 * 	- false: configuration is incorrect
 */
static bool nvs_config_validate(pivot_config config)
{
	bool ret = false;
	if((config.power_state == PIVOT_ON || config.power_state == PIVOT_OFF)
		&&(config.advance_mode == PIVOT_ADVANCE || config.advance_mode == PIVOT_REVERSE)
		&&(config.watering_state == PIVOT_DRY || config.watering_state == PIVOT_WET)
		&&(config.percentimeter <= 100))
	{
		ret = true;
	}
	else
	{
		ESP_LOGE( NVS_CONFIG_TAG, "%s, invalid configuration", __func__);
	}

	return ret;
}

/**@}*/ 	//nvs_data
/** @}*/	//components

