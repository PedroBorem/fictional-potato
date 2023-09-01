/**
 * @file data_app.c
 * @date Set 18, 2022
 * @brief memory control application
*/

/* Self include */
#include "data_app.h"
#include "nvs_data.h"

/* Project include */
#include "esp_log.h"
#include "esp_random.h"
#include "project_config.h"

/* C base */
#include <string.h>

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
#define DATA_ACTION					"action"
#define DATA_PIVOT_CONFIG			"pivot_config"
#define DATA_NETWORK_CONFIG			"net_config"
#define DATA_ECO_MODE_CONFIG		"eco_config"
#define DATA_SECTOR_CONFIG			"sector_config"
#define DATA_SCHEDULING_DATE		"s_date"
#define DATA_SCHEDULING_ANGLE		"s_angle"
#define DATA_HISTORY				"history"
#define DATA_TIMESTAMP				"timestamp"


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
			.contactor = "NA",
			.pressure = "NA",
			.pressurization_time = 30,
			.on_off_time = 10,
	};

	const network_config default_network = {
			.gprs_id = "agrishow_1",
			.modem_apn = "apn-vazia",
			.wifi_ssid = "soil2023",
			.wifi_pass = "soiltech",
	};

	const eco_mode_config default_eco =	{};
	const sector_config default_sector = {};
	const pivot_scheduling_date default_scheduling_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
	const pivot_scheduling_angle default_scheduling_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};
	const pivot_history default_history[CONFIG_HISTORY_MAX_VALUE] = {};
	const time_t default_timestamp = 0;

	err = nvs_data_init();
	if(err == ESP_OK)
	{
		if(nvs_data_get_size(DATA_ACTION) == 0)
		{
			data_app_save(DATA_TYPE_ACTIONS, &default_action, sizeof(default_action));
		}

		if(nvs_data_get_size(DATA_PIVOT_CONFIG) == 0)
		{
			data_app_save(DATA_TYPE_PIVOT_CONFIG, &default_config, sizeof(default_config));
		}

		if(nvs_data_get_size(DATA_NETWORK_CONFIG) == 0)
		{
			data_app_save(DATA_TYPE_NETWORK_CONFIG, &default_network, sizeof(default_network));
		}

		if(nvs_data_get_size(DATA_ECO_MODE_CONFIG) == 0)
		{
			data_app_save(DATA_TYPE_ECO_MODE_CONFIG, &default_eco, sizeof(default_eco));
		}

		if(nvs_data_get_size(DATA_SECTOR_CONFIG) == 0)
		{
			data_app_save(DATA_TYPE_SECTOR_CONFIG, &default_sector, sizeof(default_sector));
		}

		if(nvs_data_get_size(DATA_SCHEDULING_DATE) == 0)
		{
			data_app_save(DATA_TYPE_SCHEADULING_DATE, &default_scheduling_date, sizeof(default_scheduling_date));
		}

		if(nvs_data_get_size(DATA_SCHEDULING_ANGLE) == 0)
		{
			data_app_save(DATA_TYPE_SCHEADULING_ANGLE, &default_scheduling_angle, sizeof(default_scheduling_angle));
		}

		if(nvs_data_get_size(DATA_HISTORY) == 0)
		{
			data_app_save(DATA_TYPE_HISTORY, &default_history, sizeof(default_history));
		}

		if(nvs_data_get_size(DATA_TIMESTAMP) == 0)
		{
			data_app_save(DATA_TYPE_TIMESTAMP, &default_timestamp, sizeof(default_timestamp));
		}

		ESP_LOGI( DATA_APP_TAG, "%s, data application started successfully", __func__);
	}
	else
	{
		ESP_LOGE( DATA_APP_TAG, "%s, failed to start data application", __func__);
	}

	return err;
}

esp_err_t data_app_save(data_type_t data_type, const void* data, size_t data_size)
{
	esp_err_t ret = ESP_FAIL;

	switch(data_type)
	{
		case DATA_TYPE_ACTIONS:
		{
			ret = nvs_data_set(DATA_ACTION, data, data_size);
			break;
		}
		case DATA_TYPE_PIVOT_CONFIG:
		{
			ret = nvs_data_set(DATA_PIVOT_CONFIG, data, data_size);
			break;
		}
		case DATA_TYPE_NETWORK_CONFIG:
		{
			ret = nvs_data_set(DATA_NETWORK_CONFIG, data, data_size);
			break;
		}
		case DATA_TYPE_ECO_MODE_CONFIG:
		{
			ret = nvs_data_set(DATA_ECO_MODE_CONFIG, data, data_size);
			break;
		}
		case DATA_TYPE_SECTOR_CONFIG:
		{
			ret = nvs_data_set(DATA_SECTOR_CONFIG, data, data_size);
			break;
		}
		case DATA_TYPE_SCHEADULING_DATE:
		{
			ret = nvs_data_set(DATA_SCHEDULING_DATE, data, data_size);
			break;
		}
		case DATA_TYPE_SCHEADULING_ANGLE:
		{
			ret = nvs_data_set(DATA_SCHEDULING_ANGLE, data, data_size);
			break;
		}
		case DATA_TYPE_HISTORY:
		{
			pivot_history history[CONFIG_HISTORY_MAX_VALUE] = {};
			pivot_history history_tmp = {};
			int i,j;

			data_app_load(DATA_TYPE_HISTORY, history);
			mempcpy(&history[0], data, sizeof(pivot_history));

			for( i = 1; i < CONFIG_HISTORY_MAX_VALUE; i++)
			{
				memcpy(&history_tmp, &history[i], sizeof(pivot_history));

				for(j = i-1; j >= 0 && history_tmp.start_date < history[j].start_date; j--)
				{
					memcpy(&history[j+1], &history[j], sizeof(pivot_history));
				}

				memcpy(&history[j+1], &history_tmp, sizeof(pivot_history));
			}

			ret = nvs_data_set(DATA_HISTORY, history, data_size);
			break;
		}
		case DATA_TYPE_OLD_HISTORY:
		{
			pivot_history history[CONFIG_HISTORY_MAX_VALUE] = {};
			pivot_history history_tmp = {};

			data_app_load(DATA_TYPE_HISTORY, history);

			mempcpy( &history_tmp, data, sizeof(pivot_history));

			history[CONFIG_HISTORY_MAX_VALUE - 1].end_date = history_tmp.end_date;
			history[CONFIG_HISTORY_MAX_VALUE - 1].end_angle = history_tmp.end_angle;

			ret = nvs_data_set(DATA_HISTORY, history, data_size);
			break;
		}
		case DATA_TYPE_TIMESTAMP:
		{
			ret = nvs_data_set(DATA_TIMESTAMP, data, data_size);
			break;
		}
		default:
		{
			break;
		}
	}

	return ret;
}

esp_err_t data_app_load(data_type_t data_type, void* data)
{
	esp_err_t ret = ESP_FAIL;

	switch(data_type)
	{
		case DATA_TYPE_ACTIONS:
		{
			ret = nvs_data_get_blob(DATA_ACTION, data);
			break;
		}
		case DATA_TYPE_PIVOT_CONFIG:
		{
			ret = nvs_data_get_blob(DATA_PIVOT_CONFIG, data);
			break;
		}
		case DATA_TYPE_NETWORK_CONFIG:
		{
			ret = nvs_data_get_blob(DATA_NETWORK_CONFIG, data);
			break;
		}
		case DATA_TYPE_ECO_MODE_CONFIG:
		{
			ret = nvs_data_get_blob(DATA_ECO_MODE_CONFIG, data);
			break;
		}
		case DATA_TYPE_SECTOR_CONFIG:
		{
			ret = nvs_data_get_blob(DATA_SECTOR_CONFIG, data);
			break;
		}
		case DATA_TYPE_SCHEADULING_DATE:
		{
			ret = nvs_data_get_blob(DATA_SCHEDULING_DATE, data);
			break;
		}
		case DATA_TYPE_SCHEADULING_ANGLE:
		{
			ret = nvs_data_get_blob(DATA_SCHEDULING_ANGLE, data);
			break;
		}
		case DATA_TYPE_HISTORY:
		{
			ret = nvs_data_get_blob(DATA_HISTORY, data);
			break;
		}
		case DATA_TYPE_TIMESTAMP:
		{
			ret = nvs_data_get_blob(DATA_TIMESTAMP, data);
			break;
		}
		default:
		{
			break;
		}
	}

	return ret;
}

esp_err_t data_app_delete(void* data_id)
{
	esp_err_t ret = ESP_FAIL;

	pivot_scheduling_date scheduling_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
	ret = data_app_load(DATA_TYPE_SCHEADULING_DATE, scheduling_date);
	if(ret == ESP_OK)
	{
		for(uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
		{
			if(strcmp(scheduling_date[position].scheduling_id, data_id) == 0)
			{
				ESP_LOGW(DATA_APP_TAG, "deleting schedule date id : %s", scheduling_date[position].scheduling_id);
				memset(&scheduling_date[position], 0x00, sizeof(pivot_scheduling_date));
				data_app_save(DATA_TYPE_SCHEADULING_DATE, scheduling_date, sizeof(scheduling_date));
				return ret;
			}
		}
	}

	pivot_scheduling_angle scheduling_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};
	ret = data_app_load(DATA_TYPE_SCHEADULING_ANGLE, scheduling_angle);
	if(ret == ESP_OK)
	{
		for(uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
		{
			if(strcmp(scheduling_angle[position].scheduling_id, data_id) == 0)
			{
				ESP_LOGW(DATA_APP_TAG, "deleting schedule angle id : %s", scheduling_angle[position].scheduling_id);
				memset(&scheduling_angle[position], 0x00, sizeof(pivot_scheduling_angle));
				data_app_save(DATA_TYPE_SCHEADULING_ANGLE, scheduling_angle, sizeof(scheduling_angle));
				return ret;
			}
		}
	}

	return ret;
}

void data_app_gen_scheduling_key(char* scheduling_id)
{
	char output_key[8] = "";
	uint8_t random_number;
	uint8_t char_position = 0;

	esp_fill_random(&random_number, sizeof(random_number));
	while(char_position < 6)
	{
		if((random_number >= 48 && random_number <= 57)
		|| (random_number >= 65 && random_number <= 90)
		|| (random_number >= 97 && random_number <= 122))
		{
			output_key[char_position] = random_number;
			esp_fill_random(&random_number, sizeof(random_number));
			char_position++;
		}
		else
		{
			esp_fill_random(&random_number, sizeof(random_number));
		}
	}

	memcpy(scheduling_id, &output_key, strlen(output_key));
}

size_t data_app_get_data_size(const char* label_name)
{
    return nvs_data_get_size(label_name);
}
