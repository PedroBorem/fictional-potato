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
 * @def DATA_NETWORK_CONFIG
 * @brief NVS access space for network configuration data.
 */
#define DATA_NETWORK_CONFIG "net_config"

/**
 * @def DATA_RUSH_MODE_CONFIG
 * @brief NVS access space for Rush Mode configuration data.
 */
#define DATA_RUSH_MODE_CONFIG "rush_config"

/**
 * @def DATA_RUSH_MODE_STATE
 * @brief NVS access space for Rush Mode runtime state data.
 */
#define DATA_RUSH_MODE_STATE "rush_state"

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
 * @def DATA_HISTORY
 * @brief NVS access space for history data.
 */
#define DATA_HISTORY "history"

/**
 * @def DATA_COMM_MAIN_MODE
 * @brief NVS access space for Communication Principal Mode
 */
#define DATA_COMM_MAIN_MODE "comm_main_mode"

/**
 * @def DATA_REASON_HANG_UP
 * @brief NVS access space for the last system shutdown reason.
 */
#define DATA_REASON_HANG_UP "reason_hangup"

/**
 * @def DATA_ACTUATION_ACTIONS
 * @brief NVS access space for new-product actuation commands.
 */
#define DATA_ACTUATION_ACTIONS "act_actions"

/**
 * @def DATA_ACTUATION_CONFIG
 * @brief NVS access space for new-product actuation configuration.
 */
#define DATA_ACTUATION_CONFIG "act_config"

static void data_app_remove_discarded_pivot_data(void)
{
	static const char *const discarded_keys[] = {
		"action",
		"pivot_config",
		"sector_config",
		"gps_config",
		"s_angle",
		"s_off_angle",
		"timestamp",
		"status_barrier",
		"virtual_barrier",
		"phy_barrier",
		"initial_angle",
		"manual_counter",
		"reboot_config",
		"s_start_state",
	};

	for (size_t index = 0; index < sizeof(discarded_keys) / sizeof(discarded_keys[0]); index++)
	{
		esp_err_t err = nvs_data_erase(discarded_keys[index]);
		if (err != ESP_OK)
		{
			LOG_WARNING(DATA_APP_TAG,
						"NVS",
						"failed to remove discarded NVS key %s: %s",
						discarded_keys[index],
						esp_err_to_name(err));
		}
	}
}

/**
 * @brief Deletes a scheduling entry from a specific persisted scheduling type.
 * @param scheduling_type Data type that stores the scheduling array.
 * @param scheduling_item_size Size of each scheduling item in the array.
 * @param scheduling_log_type Scheduling label used in log messages.
 * @param scheduling_id Scheduling identifier being deleted.
 * @return esp_err_t Error code indicating the success of the operation.
 */
static esp_err_t data_app_delete_scheduling_internal(data_type_t scheduling_type,
													 size_t scheduling_item_size,
													 const char *scheduling_log_type,
													 const char *scheduling_id)
{
	typedef union
	{
		pump_scheduling_date scheduling_date[CONFIG_SCHEDULING_MAX_VALUE];
		pump_scheduling_off_date scheduling_off_date[CONFIG_SCHEDULING_MAX_VALUE];
	} data_app_scheduling_buffer;

	data_app_scheduling_buffer scheduling_buffer = {};
	void *scheduling_data = NULL;
	size_t scheduling_data_size = 0;
	esp_err_t ret = ESP_FAIL;

	switch (scheduling_type)
	{
		case DATA_TYPE_SCHEDULING_DATE:
		{
			scheduling_data = scheduling_buffer.scheduling_date;
			scheduling_data_size = sizeof(scheduling_buffer.scheduling_date);
			break;
		}
		case DATA_TYPE_SCHEDULING_OFF_DATE:
		{
			scheduling_data = scheduling_buffer.scheduling_off_date;
			scheduling_data_size = sizeof(scheduling_buffer.scheduling_off_date);
			break;
		}
		default:
		{
			return ESP_ERR_INVALID_ARG;
		}
	}

	ret = data_app_load(scheduling_type, scheduling_data);
	if (ret != ESP_OK)
	{
		return ret;
	}

	for (uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
	{
		char *current_scheduling_id = (char *)scheduling_data + (position * scheduling_item_size);

		if (strcmp(current_scheduling_id, scheduling_id) == 0)
		{
			LOG_WARNING(DATA_APP_TAG, "NVS", "deleting schedule %s id: %s", scheduling_log_type, current_scheduling_id);
			memset((uint8_t *)scheduling_data + (position * scheduling_item_size), 0x00, scheduling_item_size);

			ret = data_app_save(scheduling_type, scheduling_data, scheduling_data_size);
			if (ret != ESP_OK)
			{
				return ret;
			}

			return ESP_OK;
		}
	}

	return ESP_ERR_NOT_FOUND;
}

/**
 * @brief Initializes the data application.
 * @return esp_err_t Error code indicating the success of the operation.
 */
esp_err_t data_app_init(void)
{
	esp_err_t err = ESP_FAIL;

	const actuation_actions default_actuation_actions = {};

	const actuation_config default_actuation_config = {
			.relay_pulse_time_ms = CONFIG_ACTUATION_DEFAULT_RELAY_PULSE_MS,
			.idle_read_time_sec = CONFIG_ACTUATION_DEFAULT_IDLE_READ_TIME_SEC,
			.status_active_level = CONFIG_ACTUATION_DEFAULT_STATUS_ACTIVE_LEVEL,
			.ramp_1_delay_sec = CONFIG_PUMP_RAMP_1_DELAY_MS / 1000U,
			.stage_1_delay_sec = CONFIG_PUMP_STAGE_1_DELAY_MS / 1000U,
			.ramp_2_delay_sec = CONFIG_PUMP_RAMP_2_DELAY_MS / 1000U,
			.stage_2_delay_sec = CONFIG_PUMP_STAGE_2_DELAY_MS / 1000U,
			.ramp_3_delay_sec = CONFIG_PUMP_RAMP_3_DELAY_MS / 1000U,
			.stage_3_delay_sec = CONFIG_PUMP_STAGE_3_DELAY_MS / 1000U,
			.ramp_4_delay_sec = CONFIG_PUMP_RAMP_4_DELAY_MS / 1000U,
			.stage_4_delay_sec = CONFIG_PUMP_STAGE_4_DELAY_MS / 1000U,
			.status_publish_time_min = CONFIG_ACTUATION_DEFAULT_STATUS_PUBLISH_MIN,
	};

	const network_config default_network = {
			.gprs_id = "soilteste_1",
			.modem_apn = "virtueyes.com.br",
			.wifi_ssid = "soil2023",
			.wifi_pass = "soiltech",
	};

	const comm_main_mode_config default_comm_main_mode = {
			.comm_main_mode_config = "RF",
	};

	const rush_mode_config default_rush_mode = {};
	const rush_mode_saved_state default_rush_mode_state = {};
	const pump_scheduling_date default_scheduling_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
	const pump_scheduling_off_date default_scheduling_off_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
	const pivot_history default_history[CONFIG_HISTORY_MAX_VALUE] = {};

	err = nvs_data_init();
	if(err == ESP_OK)
	{
		data_app_remove_discarded_pivot_data();

		if(nvs_data_get_size(DATA_NETWORK_CONFIG) == 0)
		{
			data_app_save(DATA_TYPE_NETWORK_CONFIG, &default_network, sizeof(default_network));
		}

		if(nvs_data_get_size(DATA_RUSH_MODE_CONFIG) == 0)
		{
			data_app_save(DATA_TYPE_RUSH_MODE_CONFIG, &default_rush_mode, sizeof(default_rush_mode));
		}

		if(nvs_data_get_size(DATA_RUSH_MODE_STATE) == 0)
		{
			data_app_save(DATA_TYPE_RUSH_MODE_STATE, &default_rush_mode_state, sizeof(default_rush_mode_state));
		}

		if(nvs_data_get_size(DATA_SCHEDULING_DATE) != sizeof(default_scheduling_date))
		{
			data_app_save(DATA_TYPE_SCHEDULING_DATE, &default_scheduling_date, sizeof(default_scheduling_date));
		}

		if(nvs_data_get_size(DATA_SCHEDULING_OFF_DATE) != sizeof(default_scheduling_off_date))
		{
			data_app_save(DATA_TYPE_SCHEDULING_OFF_DATE, &default_scheduling_off_date, sizeof(default_scheduling_off_date));
		}

		if(nvs_data_get_size(DATA_HISTORY) == 0)
		{
			nvs_data_set(DATA_HISTORY, &default_history, sizeof(default_history));
		}

		if(nvs_data_get_size(DATA_COMM_MAIN_MODE) == 0)
		{
			data_app_save(DATA_TYPE_COMM_MAIN_MODE, &default_comm_main_mode, sizeof(default_comm_main_mode));
		}

		if(nvs_data_get_size(DATA_ACTUATION_ACTIONS) == 0)
		{
			data_app_save(DATA_TYPE_ACTUATION_ACTIONS, &default_actuation_actions, sizeof(default_actuation_actions));
		}

		if(nvs_data_get_size(DATA_ACTUATION_CONFIG) == 0)
		{
			data_app_save(DATA_TYPE_ACTUATION_CONFIG, &default_actuation_config, sizeof(default_actuation_config));
		}

		LOG_NVS(DATA_APP_TAG, "%s, data application started successfully", __func__);
	}
	else
	{
		LOG_ERROR(DATA_APP_TAG, "NVS", "%s, failed to start data application", __func__);
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
		case DATA_TYPE_NETWORK_CONFIG:
		{
			ret = nvs_data_set(DATA_NETWORK_CONFIG, data, data_size);
			break;
		}
		case DATA_TYPE_RUSH_MODE_CONFIG:
		{
			ret = nvs_data_set(DATA_RUSH_MODE_CONFIG, data, data_size);
			break;
		}
		case DATA_TYPE_RUSH_MODE_STATE:
		{
			ret = nvs_data_set(DATA_RUSH_MODE_STATE, data, data_size);
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
		case DATA_TYPE_REASON_HANG_UP:
		{
			ret = nvs_data_set(DATA_REASON_HANG_UP, data, data_size);
			break; 
		}
		case DATA_TYPE_COMM_MAIN_MODE:
		{
			ret = nvs_data_set(DATA_COMM_MAIN_MODE, data, data_size);
			break;
		}
		case DATA_TYPE_ACTUATION_ACTIONS:
		{
			ret = nvs_data_set(DATA_ACTUATION_ACTIONS, data, data_size);
			break;
		}
		case DATA_TYPE_ACTUATION_CONFIG:
		{
			ret = nvs_data_set(DATA_ACTUATION_CONFIG, data, data_size);
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
		case DATA_TYPE_NETWORK_CONFIG:
		{
			ret = nvs_data_get_blob(DATA_NETWORK_CONFIG, data);
			break;
		}
		case DATA_TYPE_RUSH_MODE_CONFIG:
		{
			ret = nvs_data_get_blob(DATA_RUSH_MODE_CONFIG, data);
			break;
		}
		case DATA_TYPE_RUSH_MODE_STATE:
		{
			ret = nvs_data_get_blob(DATA_RUSH_MODE_STATE, data);
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
		case DATA_TYPE_HISTORY:
		{
			ret = nvs_data_get_blob(DATA_HISTORY, data);
			break;
		}
		case DATA_TYPE_REASON_HANG_UP:
		{
			ret = nvs_data_get_blob(DATA_REASON_HANG_UP, data);
			break;
		}
		case DATA_TYPE_COMM_MAIN_MODE:
		{
			ret = nvs_data_get_blob(DATA_COMM_MAIN_MODE, data);
			break;
		}
		case DATA_TYPE_ACTUATION_ACTIONS:
		{
			ret = nvs_data_get_blob(DATA_ACTUATION_ACTIONS, data);
			break;
		}
		case DATA_TYPE_ACTUATION_CONFIG:
		{
			ret = nvs_data_get_blob(DATA_ACTUATION_CONFIG, data);
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
	esp_err_t ret = ESP_OK;

	if (scheduling_id == NULL || scheduling_id[0] == '\0')
	{
		return ESP_ERR_INVALID_ARG;
	}

	ret = data_app_delete_scheduling_internal(DATA_TYPE_SCHEDULING_DATE,
											  sizeof(pump_scheduling_date),
											  "date",
											  scheduling_id);
	if (ret == ESP_OK)
	{
		return ret;
	}

	if (ret != ESP_ERR_NOT_FOUND)
	{
		return ret;
	}

	ret = data_app_delete_scheduling_internal(DATA_TYPE_SCHEDULING_OFF_DATE,
											  sizeof(pump_scheduling_off_date),
											  "off-date",
											  scheduling_id);
	if (ret == ESP_OK)
	{
		return ret;
	}

	if (ret != ESP_ERR_NOT_FOUND)
	{
		return ret;
	}

	return ESP_ERR_NOT_FOUND;
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
