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
 * NVS access space
 *
 */
#define DATA_APP_LABEL_ACTION	"space_action"
#define DATA_APP_LABEL_CONFIG	"space_config"
#define DATA_APP_LABEL_SCHEDULING_DATE	"space_s_date"
#define DATA_APP_LABEL_SCHEDULING_ANGLE	"space_s_angle"
#define DATA_APP_LABEL_HISTORY	"space_history"

/**
 * NVS access label
 *
 */
#define DATA_KEY_ACTIONS	"label_actions"
#define DATA_KEY_CONFIG		"label_config"
#define DATA_KEY_SCHEDULING_DATE	"label_s_date"
#define DATA_KEY_SCHEDULING_ANGLE	"label_s_angle"
#define DATA_KEY_HISTORY	"label_history"

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
			.gprs_id = "TesteInatel_5",
			.contactor = CONTACTOR_NA,
			.pressure_switch = PRESSURE_SWITCH_NA,
			.pressurization_time = 30,
			.on_off_time = 01,
			.eco_mode = false,
			.sector_enabled = false,
	};

	const pivot_scheduling_date default_scheduling_date[SCHEDULING_MAX_VALUE] = {};
	const pivot_scheduling_angle default_scheduling_angle[SCHEDULING_MAX_VALUE] = {};
	const pivot_history default_history[HISTORY_MAX_VALUE] = {};

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

		if(nvs_data_get_size(DATA_APP_LABEL_SCHEDULING_DATE, DATA_KEY_SCHEDULING_DATE) == 0)
		{
			data_app_save_scheduling(data_scheduling_date, &default_scheduling_date, sizeof(default_scheduling_date));
		}

		if(nvs_data_get_size(DATA_APP_LABEL_SCHEDULING_ANGLE, DATA_KEY_SCHEDULING_ANGLE) == 0)
		{
			data_app_save_scheduling(data_scheduling_angle, &default_scheduling_angle, sizeof(default_scheduling_angle));
		}

		if(nvs_data_get_size(DATA_APP_LABEL_HISTORY, DATA_KEY_HISTORY) == 0)
		{
			nvs_data_set(DATA_APP_LABEL_HISTORY, DATA_KEY_HISTORY, default_history, sizeof(default_history));
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
	return nvs_data_get_blob(DATA_APP_LABEL_ACTION, DATA_KEY_ACTIONS, out_value);
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
	return nvs_data_get_blob(DATA_APP_LABEL_CONFIG, DATA_KEY_CONFIG, out_value);
}

esp_err_t data_app_save_scheduling(data_scheduling_type scheduling_type, const void* value, size_t size)
{
	esp_err_t ret = ESP_FAIL;

	if(value != NULL)
	{
		if(scheduling_type == data_scheduling_date)
		{
			ret = nvs_data_set(DATA_APP_LABEL_SCHEDULING_DATE, DATA_KEY_SCHEDULING_DATE, value, size);
		}
		else if(scheduling_type == data_scheduling_angle)
		{
			ret = nvs_data_set(DATA_APP_LABEL_SCHEDULING_ANGLE, DATA_KEY_SCHEDULING_ANGLE, value, size);
		}
	}

	return ret;
}

esp_err_t data_app_load_scheduling(data_scheduling_type scheduling_type, void* out_value, size_t size)
{
	esp_err_t ret = ESP_FAIL;

	size_t required_size = nvs_data_get_size(DATA_APP_LABEL_CONFIG, DATA_KEY_CONFIG);
	if(size < required_size)
	{
		ESP_LOGE( DATA_APP_TAG, "%s, buffer size smaller than label size %d < %d", __func__, size, required_size);
	}
	else
	{
		if(scheduling_type == data_scheduling_date)
		{
			ret = nvs_data_get_blob(DATA_APP_LABEL_SCHEDULING_DATE, DATA_KEY_SCHEDULING_DATE, out_value);
		}
		else if(scheduling_type == data_scheduling_angle)
		{
			ret = nvs_data_get_blob(DATA_APP_LABEL_SCHEDULING_ANGLE, DATA_KEY_SCHEDULING_ANGLE, out_value);
		}
	}

	return ret;
}

esp_err_t data_app_delete_scheduling(data_scheduling_type scheduling_type, char* scheduling_id)
{
	esp_err_t ret = ESP_FAIL;

	if(scheduling_type == data_scheduling_date)
	{
		pivot_scheduling_date scheduling_date[SCHEDULING_MAX_VALUE] = {};
		ret = data_app_load_scheduling(data_scheduling_date, scheduling_date, sizeof(scheduling_date));
		if(ret == ESP_OK)
		{
			for(uint8_t position = 0; position < SCHEDULING_MAX_VALUE; position++)
			{
				if(strcmp(scheduling_date[position].scheduling_id,scheduling_id) == 0)
				{
					ESP_LOGW(DATA_APP_TAG, "deleting schedule date id : %s", scheduling_date[position].scheduling_id);
					memset(&scheduling_date[position], 0x00, sizeof(pivot_scheduling_date));
					data_app_save_scheduling(data_scheduling_date, scheduling_date, sizeof(scheduling_date));
					break;
				}
			}
		}
	}
	else if(scheduling_type == data_scheduling_angle)
	{
		pivot_scheduling_angle scheduling_angle[SCHEDULING_MAX_VALUE] = {};
		ret = data_app_load_scheduling(data_scheduling_angle, scheduling_angle, sizeof(scheduling_angle));
		if(ret == ESP_OK)
		{
			for(uint8_t position = 0; position < SCHEDULING_MAX_VALUE; position++)
			{
				if(strcmp(scheduling_angle[position].scheduling_id,scheduling_id) == 0)
				{
					ESP_LOGW(DATA_APP_TAG, "deleting schedule angle id : %s", scheduling_angle[position].scheduling_id);
					memset(&scheduling_angle[position], 0x00, sizeof(pivot_scheduling_angle));
					data_app_save_scheduling(data_scheduling_angle, scheduling_angle, sizeof(scheduling_angle));
					break;
				}
			}
		}
	}

	return ret;
}

esp_err_t data_app_save_new_history(pivot_history new_history)
{
	esp_err_t ret = ESP_FAIL;
	pivot_history history[HISTORY_MAX_VALUE] = {};
	pivot_history history_tmp = {};
	int i,j;

	data_app_load_history(history, sizeof(history));
	mempcpy(&history[0], &new_history, sizeof(pivot_history));

    for( i = 1; i < HISTORY_MAX_VALUE; i++)
    {
    	//printf("data_app_save_new_history %lld\n",history[i].start_date);
        memcpy(&history_tmp, &history[i], sizeof(pivot_history));

        for(j = i-1; j >= 0 && history_tmp.start_date < history[j].start_date; j--)
        {
            memcpy(&history[j+1], &history[j], sizeof(pivot_history));
        }

        memcpy(&history[j+1], &history_tmp, sizeof(pivot_history));
    }

	nvs_data_set(DATA_APP_LABEL_HISTORY, DATA_KEY_HISTORY, history, sizeof(history));

	return ret;
}

esp_err_t data_app_save_old_history(time_t end_date, uint16_t end_angle)
{
	esp_err_t ret = ESP_FAIL;
	pivot_history history[HISTORY_MAX_VALUE] = {};

	data_app_load_history(history, sizeof(history));

	history[(HISTORY_MAX_VALUE - 1)].is_running = false;
	history[(HISTORY_MAX_VALUE - 1)].end_date = end_date;
	history[(HISTORY_MAX_VALUE - 1)].end_angle = end_angle;

	nvs_data_set(DATA_APP_LABEL_HISTORY, DATA_KEY_HISTORY, history, sizeof(history));

	return ret;
}

esp_err_t data_app_load_history(void* out_value, size_t size)
{
	return nvs_data_get_blob(DATA_APP_LABEL_HISTORY, DATA_KEY_HISTORY, out_value);
}

size_t data_app_get_data_size(const char* label_name, const char* key)
{
    return nvs_data_get_size(label_name, key);
}

/**@}*/ 	//data_app
/** @}*/	//main
