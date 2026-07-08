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
 * @def DATA_SCHEDULING_START_STATE
 * @brief NVS access space for the active start schedule state.
 */
#define DATA_SCHEDULING_START_STATE "s_start_state"

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

#define DATA_STATUS_BARRIER	"status_barrier"

/**
 * @def DATA_VIRTUAL_BARRIER
 * @brief NVS access space for virtual barrier data.
 */
#define DATA_VIRTUAL_BARRIER "virtual_barrier"

/**
 * @def DATA_PHYSICS_BARRIER
 * @brief NVS access space for physical barrier data.
 */
#define DATA_PHYSICAL_BARRIER "phy_barrier"

/**
 * @def DATA_INITIAL_ANGLE
 * @brief NVS access space for initial angle data
*/
#define DATA_INITIAL_ANGLE	"initial_angle"

/**
 * @def DATA_MANUAL_COUNTER
 * @brief NVS access space for manual counter
 */
#define DATA_MANUAL_COUNTER "manual_counter"

/**
 * @def DATA_COMM_MAIN_MODE
 * @brief NVS access space for Communication Principal Mode
 */
#define DATA_COMM_MAIN_MODE "comm_main_mode"

/**
 * @def DATA_REASON_HANG_UP
 * @brief NVS access space for reason why the pivot turned off  
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

/**
 * @brief Clears the persisted active start scheduling state when it matches the deleted scheduling.
 * @param scheduling_start_state Loaded active start scheduling state.
 * @param scheduling_id Scheduling identifier being deleted.
 * @return esp_err_t Error code indicating the success of the operation.
 */
static esp_err_t data_app_clear_active_start_state(const pivot_scheduling_start_state *scheduling_start_state, const char *scheduling_id)
{
	if (scheduling_start_state == NULL || scheduling_id == NULL)
	{
		return ESP_ERR_INVALID_ARG;
	}

	if (!scheduling_start_state->active ||
		strcmp(scheduling_start_state->scheduling_id, scheduling_id) != 0)
	{
		return ESP_OK;
	}

	pivot_scheduling_start_state scheduling_start_state_clear = {};
	return data_app_save(DATA_TYPE_SCHEDULING_START_STATE, &scheduling_start_state_clear, sizeof(scheduling_start_state_clear));
}

/**
 * @brief Deletes a scheduling entry from a specific persisted scheduling type.
 * @param scheduling_type Data type that stores the scheduling array.
 * @param scheduling_item_size Size of each scheduling item in the array.
 * @param clear_active_start_state True when the deleted scheduling may own the active start state.
 * @param scheduling_log_type Scheduling label used in log messages.
 * @param scheduling_id Scheduling identifier being deleted.
 * @param scheduling_start_state Persisted active start scheduling state.
 * @return esp_err_t Error code indicating the success of the operation.
 */
static esp_err_t data_app_delete_scheduling_internal(data_type_t scheduling_type,
													 size_t scheduling_item_size,
													 bool clear_active_start_state,
													 const char *scheduling_log_type,
													 const char *scheduling_id,
													 const pivot_scheduling_start_state *scheduling_start_state)
{
	typedef union
	{
		pivot_scheduling_date scheduling_date[CONFIG_SCHEDULING_MAX_VALUE];
		pivot_scheduling_off_date scheduling_off_date[CONFIG_SCHEDULING_MAX_VALUE];
		pivot_scheduling_angle scheduling_angle[CONFIG_SCHEDULING_MAX_VALUE];
		pivot_scheduling_off_angle scheduling_off_angle[CONFIG_SCHEDULING_MAX_VALUE];
	} data_app_scheduling_buffer;

	data_app_scheduling_buffer scheduling_buffer = {};
	void *scheduling_data = NULL;
	size_t scheduling_data_size = 0;
	esp_err_t ret = ESP_FAIL;
	esp_err_t state_ret = ESP_OK;

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
		case DATA_TYPE_SCHEDULING_ANGLE:
		{
			scheduling_data = scheduling_buffer.scheduling_angle;
			scheduling_data_size = sizeof(scheduling_buffer.scheduling_angle);
			break;
		}
		case DATA_TYPE_SCHEDULING_OFF_ANGLE:
		{
			scheduling_data = scheduling_buffer.scheduling_off_angle;
			scheduling_data_size = sizeof(scheduling_buffer.scheduling_off_angle);
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
			ESP_LOGW(DATA_APP_TAG, "deleting schedule %s id : %s", scheduling_log_type, current_scheduling_id);
			memset((uint8_t *)scheduling_data + (position * scheduling_item_size), 0x00, scheduling_item_size);

			if (clear_active_start_state)
			{
				state_ret = data_app_clear_active_start_state(scheduling_start_state, scheduling_id);
			}

			ret = data_app_save(scheduling_type, scheduling_data, scheduling_data_size);
			if (ret != ESP_OK)
			{
				return ret;
			}

			return state_ret;
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

	const pivot_actions default_action = {
			.power_state = PIVOT_OFF,
			.rotation = PIVOT_CCW,
			.watering_state = PIVOT_DRY,
			.percentimeter = 0,
	};

	const pivot_config default_config = {
			.contactor = "NA",
			.pressure = "NA",
			.pressurization_time = 300,
			.on_time = 1,
			.off_time = 2,
			.read_time = 10
	};

	const actuation_actions default_actuation_actions = {};

	const actuation_config default_actuation_config = {
			.relay_pulse_time_ms = CONFIG_ACTUATION_DEFAULT_RELAY_PULSE_MS,
			.read_time_sec = CONFIG_ACTUATION_DEFAULT_READ_TIME_SEC,
			.status_active_level = CONFIG_ACTUATION_DEFAULT_STATUS_ACTIVE_LEVEL,
			.stage_1_delay_sec = CONFIG_PUMP_STAGE_1_DELAY_MS / 1000U,
			.stage_2_delay_sec = CONFIG_PUMP_STAGE_2_DELAY_MS / 1000U,
			.stage_3_delay_sec = CONFIG_PUMP_STAGE_3_DELAY_MS / 1000U,
			.status_publish_time_sec = CONFIG_ACTUATION_DEFAULT_READ_TIME_SEC,
	};

	const network_config default_network = {
			.gprs_id = "soilteste_1",
			.modem_apn = "virtueyes.com.br",
			.wifi_ssid = "soil2023",
			.wifi_pass = "soiltech",
	};

	const reboot_config default_reboot = {
			.enable = false,
			.reboot_timeout_sec = 3600, 	//1 hours in sec
	};

	const pivot_physical_config default_phy_barrier = {
			.start_angle_physical_barrier = 0,
			.end_angle_physical_barrier = 0,
			.automatic_return = 0, 
			.water_return = 0,
			.time_leaving_barrier = 3,
	};

	const pivot_comm_main_mode_config default_comm_main_mode = {
			.comm_main_mode_config = "RF",
	};

	const gps_config gps_config = {};
	const rush_mode_config default_rush_mode = {};
	const rush_mode_saved_state default_rush_mode_state = {};
	const sector_config default_sector = {};
	const pivot_scheduling_date default_scheduling_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
	const pivot_scheduling_off_date default_scheduling_off_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
	const pivot_scheduling_angle default_scheduling_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};
	const pivot_scheduling_off_angle default_scheduling_off_angle = {};
	const pivot_scheduling_start_state default_scheduling_start_state = {};
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

		if(nvs_data_get_size(DATA_RUSH_MODE_CONFIG) == 0)
		{
			data_app_save(DATA_TYPE_RUSH_MODE_CONFIG, &default_rush_mode, sizeof(default_rush_mode));
		}

		if(nvs_data_get_size(DATA_RUSH_MODE_STATE) == 0)
		{
			data_app_save(DATA_TYPE_RUSH_MODE_STATE, &default_rush_mode_state, sizeof(default_rush_mode_state));
		}

		if(nvs_data_get_size(DATA_SECTOR_CONFIG) == 0)
		{
			data_app_save(DATA_TYPE_SECTOR_CONFIG, &default_sector, sizeof(default_sector));
		}

		if(nvs_data_get_size(DATA_GPS_CONFIG) == 0)
		{
			data_app_save(DATA_TYPE_GPS_CONFIG, &gps_config, sizeof(gps_config));
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

		if(nvs_data_get_size(DATA_SCHEDULING_START_STATE) == 0)
		{
			data_app_save(DATA_TYPE_SCHEDULING_START_STATE, &default_scheduling_start_state, sizeof(default_scheduling_start_state));
		}

		if(nvs_data_get_size(DATA_HISTORY) == 0)
		{
			nvs_data_set(DATA_HISTORY, &default_history, sizeof(default_history));
		}

		if(nvs_data_get_size(DATA_TIMESTAMP) == 0)
		{
			data_app_save(DATA_TYPE_TIMESTAMP, &default_timestamp, sizeof(default_timestamp));
		}
		if(nvs_data_get_size(DATA_STATUS_BARRIER) == 0)
		{
			data_app_save(DATA_TYPE_BARRIER_STATUS, &default_barrier, sizeof(default_barrier));
		}
		if(nvs_data_get_size(DATA_VIRTUAL_BARRIER) == 0)
		{
			data_app_save(DATA_TYPE_VIRTUAL_BARRIER, &default_barrier, sizeof(default_barrier));
		}
		if(nvs_data_get_size(DATA_PHYSICAL_BARRIER) == 0)
		{
			data_app_save(DATA_TYPE_PHYSICAL_BARRIER, &default_phy_barrier, sizeof(default_phy_barrier));
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
		case DATA_TYPE_SCHEDULING_START_STATE:
		{
			ret = nvs_data_set(DATA_SCHEDULING_START_STATE, data, data_size);
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
		case DATA_TYPE_BARRIER_STATUS:
		{
			ret = nvs_data_set(DATA_STATUS_BARRIER, data, data_size);
			break;
		}
		case DATA_TYPE_VIRTUAL_BARRIER:
		{
			ret = nvs_data_set(DATA_VIRTUAL_BARRIER, data, data_size);
			break;
		}
		case DATA_TYPE_PHYSICAL_BARRIER:
		{
			ret = nvs_data_set(DATA_PHYSICAL_BARRIER, data, data_size);
			break;
		}
		case DATA_TYPE_INITIAL_ANGLE:
		{
			ret = nvs_data_set(DATA_INITIAL_ANGLE, data, data_size);
			break;
		}
		case DATA_TYPE_MANUAL_COUNTER:
		{
			ret = nvs_data_set(DATA_MANUAL_COUNTER, data, data_size);
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
		case DATA_TYPE_SCHEDULING_START_STATE:
		{
			ret = nvs_data_get_blob(DATA_SCHEDULING_START_STATE, data);
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
		case DATA_TYPE_BARRIER_STATUS:
		{
			ret = nvs_data_get_blob(DATA_STATUS_BARRIER, data);
			break;
		}
		case DATA_TYPE_VIRTUAL_BARRIER:
		{
			ret = nvs_data_get_blob(DATA_VIRTUAL_BARRIER, data);
			break;
		}
		case DATA_TYPE_PHYSICAL_BARRIER:
		{
			ret = nvs_data_get_blob(DATA_PHYSICAL_BARRIER, data);
			break;
		}
		case DATA_TYPE_INITIAL_ANGLE:
		{
			ret = nvs_data_get_blob(DATA_INITIAL_ANGLE, data);
			break;
		}
		case DATA_TYPE_MANUAL_COUNTER:
		{
			ret = nvs_data_get_blob(DATA_MANUAL_COUNTER, data);
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
	pivot_scheduling_start_state scheduling_start_state = {};

	if (scheduling_id == NULL)
	{
		return ESP_ERR_INVALID_ARG;
	}

	data_app_load(DATA_TYPE_SCHEDULING_START_STATE, &scheduling_start_state);

	ret = data_app_delete_scheduling_internal(DATA_TYPE_SCHEDULING_DATE,
											  sizeof(pivot_scheduling_date),
											  true,
											  "date",
											  scheduling_id,
											  &scheduling_start_state);
	if (ret == ESP_OK)
	{
		return ret;
	}

	if (ret != ESP_ERR_NOT_FOUND)
	{
		return ret;
	}

	ret = data_app_delete_scheduling_internal(DATA_TYPE_SCHEDULING_OFF_DATE,
											  sizeof(pivot_scheduling_off_date),
											  false,
											  "date",
											  scheduling_id,
											  &scheduling_start_state);
	if (ret == ESP_OK)
	{
		return ret;
	}

	if (ret != ESP_ERR_NOT_FOUND)
	{
		return ret;
	}

	ret = data_app_delete_scheduling_internal(DATA_TYPE_SCHEDULING_ANGLE,
											  sizeof(pivot_scheduling_angle),
											  true,
											  "angle",
											  scheduling_id,
											  &scheduling_start_state);
	if (ret == ESP_OK)
	{
		return ret;
	}

	if (ret != ESP_ERR_NOT_FOUND)
	{
		return ret;
	}

	ret = data_app_delete_scheduling_internal(DATA_TYPE_SCHEDULING_OFF_ANGLE,
											  sizeof(pivot_scheduling_off_angle),
											  false,
											  "angle",
											  scheduling_id,
											  &scheduling_start_state);
	if (ret == ESP_OK)
	{
		return ret;
	}

	if (ret != ESP_ERR_NOT_FOUND)
	{
		return ret;
	}

	return ESP_OK;
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
