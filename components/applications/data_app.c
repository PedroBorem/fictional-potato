/**
 * @file data_app.c
 * @date Set 18, 2022
 * @brief Memory control application.
*/

/* Self include */
#include "data_app.h"
#include "nvs_data.h"

/* Project include */
#include "log.h"
#include "esp_log.h"
#include "esp_random.h"
#include "project_config.h"

/* C base */
#include <string.h>

/**
 * @def DATA_APP_TAG
 * @brief Class log tag.
 */
#define DATA_APP_TAG "data_app"

/**
 * @def DATA_ACTION
 * @brief NVS access space for action data.
 */
#define DATA_ACTION "action"

/**
 * @def DATA_PIVOT_CONFIG
 * @brief NVS access space for pivot configuration data.
 */
#define DATA_PIVOT_CONFIG "pivot_config"

/**
 * @def DATA_NETWORK_CONFIG
 * @brief NVS access space for network configuration data.
 */
#define DATA_NETWORK_CONFIG "net_config"

/**
 * @def DATA_ECO_MODE_CONFIG
 * @brief NVS access space for eco mode configuration data.
 */
#define DATA_ECO_MODE_CONFIG "eco_config"

/**
 * @def DATA_SECTOR_CONFIG
 * @brief NVS access space for sector configuration data.
 */
#define DATA_SECTOR_CONFIG "sector_config"

/**
 * @def DATA_GPS_CONFIG
 * @brief NVS access space for gps configuration data.
 */
#define DATA_GPS_CONFIG "gps_config"

/**
 * @def DATA_RETURN_CONFIG
 * @brief NVS access space for return configuration data.
 */
#define DATA_RETURN_CONFIG "return_config"

/**
 * @def DATA_REBOOT_CONFIG
 * @brief NVS access space for reboot configuration data.
 */
#define DATA_REBOOT_CONFIG "reboot_config"

/**
 * @def DATA_SCHEDULING_DATE
 * @brief NVS access space for scheduling date data.
 */
#define DATA_SCHEDULING_DATE "s_date"

/**
 * @def DATA_SCHEDULING_OFF_DATE
 * @brief NVS access space for scheduling off date data.
 */
#define DATA_SCHEDULING_OFF_DATE "s_off_date"

/**
 * @def DATA_SCHEDULING_ANGLE
 * @brief NVS access space for scheduling angle data.
 */
#define DATA_SCHEDULING_ANGLE "s_angle"

/**
 * @def DATA_SCHEDULING_OFF_ANGLE
 * @brief NVS access space for scheduling off angle data.
 */
#define DATA_SCHEDULING_OFF_ANGLE "s_off_angle"

/**
 * @def DATA_HISTORY
 * @brief NVS access space for history data.
 */
#define DATA_HISTORY "history"

/**
 * @def DATA_TIMESTAMP
 * @brief NVS access space for timestamp data.
 */
#define DATA_TIMESTAMP "timestamp"

/**
 * @def DATA_BARRIER
 * @brief NVS access space for barrier status data.
 */
#define DATA_BARRIER "barrier"

/**
 * @brief Initializes the data application.
 * @return esp_err_t Error code indicating the success of the operation.
 */
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
			.on_time = 1,
			.off_time = 1,
			.read_time = 10
	};

	const network_config default_network = {
			.gprs_id = "soilteste_1",
			.modem_apn = "virtueyes.com.br",
			.wifi_ssid = "soil2023",
			.wifi_pass = "soiltech",
	};

	const reboot_config default_reboot = {
			.enable = true,
			.reboot_timeout_sec = 14400, 	//4 hours in sec
	};

	const gps_config gps_config = {};
	const pivot_return_config return_config = {};
	const eco_mode_config default_eco =	{};
	const sector_config default_sector = {};
	const pivot_scheduling_date default_scheduling_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
	const pivot_scheduling_off_date default_scheduling_off_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
	const pivot_scheduling_angle default_scheduling_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};
	const pivot_scheduling_off_angle default_scheduling_off_angle = {};
	const pivot_history default_history[CONFIG_HISTORY_MAX_VALUE] = {};
	const time_t default_timestamp = 0;
	const bool default_barrier = false;

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

		if(nvs_data_get_size(DATA_GPS_CONFIG) == 0)
		{
			data_app_save(DATA_TYPE_GPS_CONFIG, &gps_config, sizeof(gps_config));
		}

		if(nvs_data_get_size(DATA_RETURN_CONFIG) == 0)
		{
			data_app_save(DATA_TYPE_RETURN_CONFIG, &return_config, sizeof(return_config));
		}

		if(nvs_data_get_size(DATA_REBOOT_CONFIG) == 0)
		{
			data_app_save(DATA_TYPE_REBOOT_CONFIG, &default_reboot, sizeof(default_reboot));
		}

		if(nvs_data_get_size(DATA_SCHEDULING_DATE) == 0)
		{
			data_app_save(DATA_TYPE_SCHEDULING_DATE, &default_scheduling_date, sizeof(default_scheduling_date));
		}

		if(nvs_data_get_size(DATA_SCHEDULING_OFF_DATE) == 0)
		{
			data_app_save(DATA_TYPE_SCHEDULING_OFF_DATE, &default_scheduling_off_date, sizeof(default_scheduling_off_date));
		}

		if(nvs_data_get_size(DATA_SCHEDULING_ANGLE) == 0)
		{
			data_app_save(DATA_TYPE_SCHEDULING_ANGLE, &default_scheduling_angle, sizeof(default_scheduling_angle));
		}

		if(nvs_data_get_size(DATA_SCHEDULING_OFF_ANGLE) == 0)
		{
			data_app_save(DATA_TYPE_SCHEDULING_OFF_ANGLE, &default_scheduling_off_angle, sizeof(default_scheduling_off_angle));
		}

		if(nvs_data_get_size(DATA_HISTORY) == 0)
		{
			nvs_data_set(DATA_HISTORY, &default_history, sizeof(default_history));
		}

		if(nvs_data_get_size(DATA_TIMESTAMP) == 0)
		{
			data_app_save(DATA_TYPE_TIMESTAMP, &default_timestamp, sizeof(default_timestamp));
		}
		if(nvs_data_get_size(DATA_BARRIER) == 0)
		{
			data_app_save(DATA_TYPE_BARRIER, &default_barrier, sizeof(default_barrier));
		}

		ESP_LOGI( DATA_APP_TAG, "%s, data application started successfully", __func__);
	}
	else
	{
		ESP_LOGE( DATA_APP_TAG, "%s, failed to start data application", __func__);
		LOG_DBG_ERROR(DATA_APP_TAG, "memory_error");
	}

	return err;
}

/**
 * @brief Saves data to the NVS based on the specified data type.
 * @param data_type Data type identifier.
 * @param data Pointer to the data to be saved.
 * @param data_size Size of the data.
 * @return esp_err_t Error code indicating the success of the operation.
 */
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
		case DATA_TYPE_GPS_CONFIG:
		{
			ret = nvs_data_set(DATA_GPS_CONFIG, data, data_size);
			break;
		}
		case DATA_TYPE_RETURN_CONFIG:
		{
			ret = nvs_data_set(DATA_RETURN_CONFIG, data, data_size);
			break;
		}
		case DATA_TYPE_REBOOT_CONFIG:
		{
			ret = nvs_data_set(DATA_REBOOT_CONFIG, data, data_size);
			break;
		}
		case DATA_TYPE_SCHEDULING_DATE:
		{
			ret = nvs_data_set(DATA_SCHEDULING_DATE, data, data_size);
			break;
		}
		case DATA_TYPE_SCHEDULING_OFF_DATE:
		{
			ret = nvs_data_set(DATA_SCHEDULING_OFF_DATE, data, data_size);
			break;
		}
		case DATA_TYPE_SCHEDULING_ANGLE:
		{
			ret = nvs_data_set(DATA_SCHEDULING_ANGLE, data, data_size);
			break;
		}
		case DATA_TYPE_SCHEDULING_OFF_ANGLE:
		{
			ret = nvs_data_set(DATA_SCHEDULING_OFF_ANGLE, data, data_size);
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

			ret = nvs_data_set(DATA_HISTORY, &history, sizeof(history));
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

			//adjust firt interation
			if(history[CONFIG_HISTORY_MAX_VALUE - 1].start_date != 0)
			{
				nvs_data_set(DATA_HISTORY, &history, sizeof(history));
			}

			ret = ESP_OK;
			break;
		}
		case DATA_TYPE_TIMESTAMP:
		{
			ret = nvs_data_set(DATA_TIMESTAMP, data, data_size);
			break;
		}
		case DATA_TYPE_BARRIER:
		{
			ret = nvs_data_set(DATA_BARRIER, data, data_size);
			break;
		}
		default:
		{
			break;
		}
	}

	return ret;
}

/**
 * @brief Loads data from the NVS based on the specified data type.
 * @param data_type Data type identifier.
 * @param data Pointer to store the loaded data.
 * @return esp_err_t Error code indicating the success of the operation.
 */
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
		case DATA_TYPE_GPS_CONFIG:
		{
			ret = nvs_data_get_blob(DATA_GPS_CONFIG, data);
			break;
		}
		case DATA_TYPE_RETURN_CONFIG:
		{
			ret = nvs_data_get_blob(DATA_RETURN_CONFIG, data);
			break;
		}
		case DATA_TYPE_REBOOT_CONFIG:
		{
			ret = nvs_data_get_blob(DATA_REBOOT_CONFIG, data);
			break;
		}
		case DATA_TYPE_SCHEDULING_DATE:
		{
			ret = nvs_data_get_blob(DATA_SCHEDULING_DATE, data);
			break;
		}
		case DATA_TYPE_SCHEDULING_OFF_DATE:
		{
			ret = nvs_data_get_blob(DATA_SCHEDULING_OFF_DATE, data);
			break;
		}
		case DATA_TYPE_SCHEDULING_ANGLE:
		{
			ret = nvs_data_get_blob(DATA_SCHEDULING_ANGLE, data);
			break;
		}
		case DATA_TYPE_SCHEDULING_OFF_ANGLE:
		{
			ret = nvs_data_get_blob(DATA_SCHEDULING_OFF_ANGLE, data);
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
		case DATA_TYPE_BARRIER:
		{
			ret = nvs_data_get_blob(DATA_BARRIER, data);
			break;
		}
		default:
		{
			break;
		}
	}

	return ret;
}

/**
 * @brief Deletes scheduling data based on the provided scheduling ID.
 * @param scheduling_id Scheduling ID to identify the data to be deleted.
 * @return esp_err_t Error code indicating the success of the operation.
 */
esp_err_t data_app_delete_scheduling(char* scheduling_id)
{
	esp_err_t ret = ESP_FAIL;

	pivot_scheduling_date scheduling_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
	ret = data_app_load(DATA_TYPE_SCHEDULING_DATE, &scheduling_date);
	if(ret == ESP_OK)
	{
		for(uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
		{
			if(strcmp(scheduling_date[position].scheduling_id, scheduling_id) == 0)
			{
				ESP_LOGW(DATA_APP_TAG, "deleting schedule date id : %s", scheduling_date[position].scheduling_id);

				pivot_scheduling_date scheduling_delete = {};
				memcpy(&scheduling_date[position], &scheduling_delete, sizeof(scheduling_delete));

				return data_app_save(DATA_TYPE_SCHEDULING_DATE, &scheduling_date, sizeof(scheduling_date));
			}
		}
	}

	pivot_scheduling_off_date scheduling_off_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
	ret = data_app_load(DATA_TYPE_SCHEDULING_OFF_DATE, &scheduling_off_date);
	if(ret == ESP_OK)
	{
		for(uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
		{
			if(strcmp(scheduling_off_date[position].scheduling_id, scheduling_id) == 0)
			{
				ESP_LOGW(DATA_APP_TAG, "deleting schedule date id : %s", scheduling_off_date[position].scheduling_id);

				pivot_scheduling_off_date scheduling_delete = {};
				memcpy(&scheduling_off_date[position], &scheduling_delete, sizeof(scheduling_delete));

				return data_app_save(DATA_TYPE_SCHEDULING_OFF_DATE, &scheduling_off_date, sizeof(scheduling_off_date));
			}
		}
	}

	pivot_scheduling_angle scheduling_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};
	ret = data_app_load(DATA_TYPE_SCHEDULING_ANGLE, &scheduling_angle);
	if(ret == ESP_OK)
	{
		for(uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
		{
			if(strcmp(scheduling_angle[position].scheduling_id, scheduling_id) == 0)
			{
				ESP_LOGW(DATA_APP_TAG, "deleting schedule angle id : %s", scheduling_angle[position].scheduling_id);

				pivot_scheduling_angle scheduling_delete = {};
				memcpy(&scheduling_angle[position], &scheduling_delete, sizeof(scheduling_delete));

				return data_app_save(DATA_TYPE_SCHEDULING_ANGLE, &scheduling_angle, sizeof(scheduling_angle));
			}
		}
	}

	pivot_scheduling_off_angle scheduling_off_angle = {};
	ret = data_app_load(DATA_TYPE_SCHEDULING_OFF_ANGLE, &scheduling_off_angle);
	if(ret == ESP_OK)
	{
		if(strcmp(scheduling_off_angle.scheduling_id, scheduling_id) == 0)
		{
			ESP_LOGW(DATA_APP_TAG, "deleting schedule angle id : %s", scheduling_off_angle.scheduling_id);

			pivot_scheduling_off_angle scheduling_delete = {};
			memcpy(&scheduling_off_angle, &scheduling_delete, sizeof(scheduling_delete));

			return data_app_save(DATA_TYPE_SCHEDULING_OFF_ANGLE, &scheduling_off_angle, sizeof(scheduling_off_angle));
		}
	}

	return ret;
}

/**
 * @brief Generates a scheduling key.
 * @param scheduling_id Pointer to store the generated scheduling key.
 */
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

/**
 * @brief Gets the size of the data stored in the NVS based on the label name.
 * @param label_name Label name to identify the data.
 * @return size_t Size of the data.
 */
size_t data_app_get_data_size(const char* label_name)
{
    return nvs_data_get_size(label_name);
}
