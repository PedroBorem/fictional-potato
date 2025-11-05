/**
 * @file system_manager.c
 * @brief Source file for the system manager module.
 *
 * This module initializes and manages various components of the system, including actuation, communication, scheduling, and sectorization.
 *
 * @date 31 de mai de 2022
 * @author brunolima
 */

// Include header files
#include "system_manager.h"
#include "project_config.h"
#include "idp_parser.h"
#include "log.h"

#include "rtc_app.h"
#include "esp_system.h"
#include "FreeRTOS_defines.h"

// Private includes
#include <string.h>

#include "actuation_app.h"
#include "comm_app.h"
#include "data_app.h"
#include "rf_uart.h"
#include "pluviometer_app.h"

#include "scheduling.h"
#include "system_monitoring.h"
#include "sectorization.h"

/** @def SYSTEM_MONITORING_TAG_COMMAND
 *  @brief Tag used to determine where the command came from
 */
#define SYSTEM_MONITORING_TAG_COMMAND "system_monitoring"

/** @def SYSTEM_SCHEDULING_TAG_COMMAND
 *  @brief Tag used to determine where the command came from
 */
#define SYSTEM_SCHEDULING_TAG_COMMAND "scheduling"

/** @def SYSTEM_ACTUATION_TAG_COMMAND
 *  @brief Tag used to determine where the command came from
 */
#define SYSTEM_ACTUATION_TAG_COMMAND "actuation_app"

/** @def SYSTEM_MANAGER_TAG
 *  @brief Log tag for the system manager module.
 */
#define SYSTEM_MANAGER_TAG "system_manager"

/** @def SYSTEM_REBOOT_DELAY_MS
 *  @brief Delay time (in milliseconds) for system reboot.
 */
#define SYSTEM_REBOOT_DELAY_MS (120000) // 2 minutes

/** @def SYSTEM_REBOOT_TIMEOUT_MS
 *  @brief Timeout duration (in milliseconds) for system reboot.
 */
#define SYSTEM_REBOOT_TIMEOUT_SEC (14400) // 4 hours

/** @def SYSTEM_SAVE_FLASH_TIME_MS
 *  @brief Time interval (in milliseconds) for saving data to flash memory.
 */
#define SYSTEM_SAVE_FLASH_TIME_MS (600000) // 10 minutes

/** @var global_pressure
 *  @brief Global variable for the current pressure.
 */
uint16_t global_pressure = 0;

/** @var global_angle
 *  @brief Global variable for the current angle.
 */
uint16_t global_angle = 655;

/** @var global_angle
 *  @brief Global variable for the comm main mode of the system.
 */
comm_type comm_main_mode = COMM_MQTT;

/** @var system_id
 *  @brief Local variable for the system ID.
 */
char system_id[50] = {};

/** @var system_read_time
 *  @brief Local variable for the system read time.
 */
static uint8_t system_read_time = 0;

/** @var system_rtc_percent
 *  @brief Local variable for the RTC percentage.
 */
static time_t system_rtc_percent = 0;

/** @var system_initial_angle
 *  @brief Local variable for the initial angle.
 */
static uint16_t system_initial_angle = 655;

/** @var system_timer
 *  @brief Timer handle for the system manager.
 */
static TimerHandle_t system_timer = NULL;

/**
 * @var gps_flag_send_to_mqtt
 * @brief Variable that will indicate whether or not the next payload received from GPS will go to MQTT
 */
static bool gps_flag_send_to_mqtt = false;


uint32_t counter_reading_panel_off = NO_MANUAL_READING;

static void system_manager_reboot(void);
static void system_manager_callback(const char *buffer_request, comm_type comm_mode);
static void system_manager_timer_callback(TimerHandle_t pxTimer);

static void system_manager_idp_00(const char *buffer, comm_type comm_mode);
static void system_manager_idp_01(const char *buffer, comm_type comm_mode);
static void system_manager_idp_02(const char *buffer, comm_type comm_mode);
static void system_manager_idp_03(const char *buffer, comm_type comm_mode);
static void system_manager_idp_04(const char *buffer, comm_type comm_mode);
static void system_manager_idp_05(const char *buffer, comm_type comm_mode);
static void system_manager_idp_06(const char *buffer, comm_type comm_mode);
static void system_manager_idp_07(const char *buffer, comm_type comm_mode);
static void system_manager_idp_12(const char *buffer, comm_type comm_mode);
static void system_manager_idp_13(const char *buffer, comm_type comm_mode);
static void system_manager_idp_14(const char *buffer, comm_type comm_mode);
static void system_manager_idp_15(const char *buffer, comm_type comm_mode);
static void system_manager_idp_16(const char *buffer, comm_type comm_mode);
static void system_manager_idp_17(const char *buffer, comm_type comm_mode);
static void system_manager_idp_18(const char *buffer, comm_type comm_mode);
static void system_manager_idp_19(const char* buffer, comm_type comm_mode);
static void system_manager_idp_21(const char *buffer, comm_type comm_mode);
static void system_manager_idp_22(const char *buffer, comm_type comm_mode);
static void system_manager_idp_23(const char *buffer, comm_type comm_mode);
static void system_manager_idp_24(const char *buffer, comm_type comm_mode);
static void system_manager_idp_26(const char *buffer, comm_type comm_mode);
static void system_manager_idp_27(const char *buufer, comm_type comm_mode);
static void system_manager_idp_28(const char *buufer, comm_type comm_mode);
static void system_manager_idp_30(const char *buffer, comm_type comm_mode);
static void system_manager_idp_32(const char* buffer, comm_type comm_mode);
static void system_manager_idp_34(const char* buffer, comm_type comm_mode);
static void system_manager_idp_31(const char *buffer, comm_type comm_mode);
static void system_manager_idp_90(const char *buffer, comm_type comm_mode);
static void system_manager_idp_91(const char *buffer, comm_type comm_mode);
static void system_manager_idp_92(const char *buffer, comm_type comm_mode);

/**
 * @brief Initializes the system manager module.
 *
 * This function initializes various components of the system, including RTC, data storage, actuation, communication, sectorization, and scheduling.
 * It also triggers an automatic reboot based on certain conditions.
 */
void system_manager_init(void)
{
	// rtc init
	ESP_ERROR_CHECK(rtc_app_init());

	// nvs init
	ESP_ERROR_CHECK(data_app_init());

	// actuation init
	pivot_config config = {};
	data_app_load(DATA_TYPE_PIVOT_CONFIG, &config);
	actuation_app_set_config(config);
	ESP_ERROR_CHECK(actuation_app_init(&system_manager_callback));	
	actuation_app_hangs_up_callback(&system_monitoring_pivot_shutdown);

	// system monitoring init
	system_read_time = config.read_time;
	pivot_physical_config physical_config = {};
	data_app_load(DATA_TYPE_PHYSICAL_BARRIER, &physical_config);
	actuation_app_leaving_barrier_time(physical_config);

	pivot_virtual_config virtual_config = {};
	data_app_load(DATA_TYPE_VIRTUAL_BARRIER, &virtual_config);
	system_monitoring_register_callback(&system_manager_callback);

	pivot_comm_main_mode_config comm_main_mode_config = {};
	data_app_load(DATA_TYPE_COMM_MAIN_MODE, &comm_main_mode_config);
	comm_app_set_main_mode_config(comm_main_mode_config);

	system_monitoring_start(physical_config, virtual_config, system_read_time);

	// communication modules init
	network_config network = {};
	data_app_load(DATA_TYPE_NETWORK_CONFIG, &network);
	strcpy(system_id, network.gprs_id);

	comm_app_wifi_config(network.wifi_ssid, network.wifi_pass);
	ESP_ERROR_CHECK(comm_app_init(&system_manager_callback));

	// automatic boot
	system_manager_reboot();

	// init sectors
	sector_config sectors = {};
	data_app_load(DATA_TYPE_SECTOR_CONFIG, &sectors);
	sectorization_start(sectors);

	// init scheduling
	pivot_scheduling_date scheduling_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
	pivot_scheduling_angle scheduling_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};
	pivot_scheduling_off_date scheduling_off_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
	pivot_scheduling_off_angle scheduling_off_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};

	data_app_load(DATA_TYPE_SCHEDULING_DATE, &scheduling_date);
	data_app_load(DATA_TYPE_SCHEDULING_ANGLE, &scheduling_angle);
	data_app_load(DATA_TYPE_SCHEDULING_OFF_DATE, &scheduling_off_date);
	data_app_load(DATA_TYPE_SCHEDULING_OFF_ANGLE, &scheduling_off_angle);
	scheduling_register_callback(&system_manager_callback);

	scheduling_start(IDP_14, scheduling_date);
	scheduling_start(IDP_15, scheduling_angle);
	scheduling_start(IDP_16, scheduling_off_date);
	scheduling_start(IDP_17, scheduling_off_angle);
	scheduling_hangs_up_callback(&system_monitoring_pivot_shutdown);

	system_timer = xTimerCreate(
		"system_timer",					/* name */
		pdMS_TO_TICKS(90000),			/* period/time */
		pdFALSE,						/* auto reload */
		(void *)0,						/* timer ID */
		system_manager_timer_callback); /* callback */
}

/**
 * @brief Performs a system reboot based on certain conditions.
 *
 * This function checks the timestamp stored in non-volatile storage (NVS) and, if certain conditions are met, performs a system reboot.
 * It waits for the power to stabilize if the system is powered on and then sets the actions accordingly.
 * The timestamp is saved to NVS to keep track of the last reboot.
 */
static void system_manager_reboot(void)
{
	reboot_config current_reboot = {};
	data_app_load(DATA_TYPE_REBOOT_CONFIG, &current_reboot);

	pivot_comm_main_mode_config current_comm = {};
	data_app_load(DATA_TYPE_COMM_MAIN_MODE, &current_comm);

	uint16_t timeout = 0;

	pivot_actions current_action = {};
	uint8_t reboot_config_idp = IDP_24;

	time_t timestamp_nvs = 0;
	time_t timestamp_now = 0;

	data_app_load(DATA_TYPE_TIMESTAMP, &timestamp_nvs);
	timestamp_now = rtc_app_get_timestamp(false);
	LOG_DATA(SYSTEM_MANAGER_TAG, " --------------------------------\n");
	LOG_DATA(SYSTEM_MANAGER_TAG, " Main Communication Mode: %s", current_comm.comm_main_mode_config);
	LOG_DATA(SYSTEM_MANAGER_TAG, " --------------------------------\n");
	LOG_DATA(SYSTEM_MANAGER_TAG, " Timeout Configurado: %lld", current_reboot.reboot_timeout_sec);
	LOG_DATA(SYSTEM_MANAGER_TAG, " Timestamp_now: %lld", timestamp_now);
	LOG_DATA(SYSTEM_MANAGER_TAG, " Timestamp_nvs: %lld", timestamp_nvs);
	LOG_DATA(SYSTEM_MANAGER_TAG, " (timestamp_now - timestamp_nvs): %lld", (timestamp_now - timestamp_nvs));
	LOG_DATA(SYSTEM_MANAGER_TAG, " --------------------------------\n");

	if (current_reboot.reboot_timeout_sec < 1)
	{
		timeout = SYSTEM_REBOOT_TIMEOUT_SEC;
		ESP_LOGE(SYSTEM_MANAGER_TAG, "Invalid current reboot timeout, default applied...");
		LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, "Invalid current reboot timeout, default applied...");
	}
	else
	{
		timeout = current_reboot.reboot_timeout_sec;
	}

	if ((current_reboot.enable) && ((timestamp_now - timestamp_nvs) < (timeout)))
	{
		esp_reset_reason_t reset_cause = esp_reset_reason();
		if (reset_cause == ESP_RST_POWERON || reset_cause == ESP_RST_BROWNOUT)
		{
			data_app_load(DATA_TYPE_ACTIONS, &current_action);

			LOG_DATA(SYSTEM_MANAGER_TAG, "");
			LOG_DATA(SYSTEM_MANAGER_TAG, " ------ NVS Current Config ------");
			LOG_DATA(SYSTEM_MANAGER_TAG, " Power state: %d", current_action.power_state);
			LOG_DATA(SYSTEM_MANAGER_TAG, " Advance mode: %d", current_action.rotation);
			LOG_DATA(SYSTEM_MANAGER_TAG, " Watering state: %d", current_action.watering_state);
			LOG_DATA(SYSTEM_MANAGER_TAG, " Percentimeter %.3d %%", current_action.percentimeter);
			LOG_DATA(SYSTEM_MANAGER_TAG, " --------------------------------\n");

			system_monitoring_pivot_shutdown(TYPE_HANGS_UP_BROWNOUT, reboot_config_idp, "0", SYSTEM_MANAGER_TAG);

			vTaskDelay(pdMS_TO_TICKS(500));

			if (current_action.power_state == PIVOT_ON)
			{
				ESP_LOGW(SYSTEM_MANAGER_TAG, "waiting for power to stabilize ...");
				vTaskDelay(pdMS_TO_TICKS(SYSTEM_REBOOT_DELAY_MS));
				actuation_app_set_actions(current_action, false);
			}
		}
	}
	else
	{
		if (!current_reboot.enable)
		{
			ESP_LOGW(SYSTEM_MANAGER_TAG, "Automatic Reboot Disabled ...");
		}
		// save old history
		data_app_save(DATA_TYPE_TIMESTAMP, &timestamp_now, sizeof(timestamp_now));
	}
}

/**
 * @brief Callback function for the system manager timer.
 *
 * This function is called when the system manager timer expires.
 *
 * @param pxTimer The timer handle.
 */
static void system_manager_timer_callback(TimerHandle_t pxTimer)
{
	system_manager_idp_00(NULL, comm_main_mode);
}

/**
 * @brief Callback function for the system manager.
 *
 * This function is called when a request is received by the system manager.
 *
 * @param buffer_request The buffer containing the request.
 * @param comm_mode The communication mode (e.g., COMM_MQTT, COMM_HTTP).
 */
static void system_manager_callback(const char *buffer_request, comm_type comm_mode)
{
	char str_idp[5] = {};
	char str_pkg[100] = {};

	idp_type idp_request = idp_parser_get(buffer_request, str_pkg);
	snprintf(str_idp, sizeof(str_idp), "%d", idp_request);

	LOG_COMM(SYSTEM_MANAGER_TAG, "%s", str_pkg);

	bool payload_ascii_valid = check_valid_characters(str_pkg, strlen(str_pkg));

	if (payload_ascii_valid == true)
	{
		switch (idp_request)
		{
		case IDP_0:
		{
			system_manager_idp_00(str_pkg, comm_mode);
			break;
		}
		case IDP_1:
		{
			system_manager_idp_01(str_pkg, comm_mode);
			break;
		}
		case IDP_2:
		{
			system_manager_idp_02(str_pkg, comm_mode);
			break;
		}
		case IDP_3:
		{
			system_manager_idp_03(str_pkg, comm_mode);
			break;
		}
		case IDP_4:
		{
			system_manager_idp_04(str_pkg, comm_mode);
			break;
		}
		case IDP_5:
		{
			system_manager_idp_05(str_pkg, comm_mode);
			break;
		}
		case IDP_6:
		{
			system_manager_idp_06(str_pkg, comm_mode);
			break;
		}
		case IDP_7:
		{
			system_manager_idp_07(str_pkg, comm_mode);
			break;
		}
		case IDP_12:
		{
			system_manager_idp_12(str_pkg, comm_mode);
			break;
		}
		case IDP_13:
		{
			system_manager_idp_13(str_pkg, comm_mode);
			break;
		}
		case IDP_14:
		{
			system_manager_idp_14(str_pkg, comm_mode);
			break;
		}
		case IDP_15:
		{
			system_manager_idp_15(str_pkg, comm_mode);
			break;
		}
		case IDP_16:
		{
			system_manager_idp_16(str_pkg, comm_mode);
			break;
		}
		case IDP_17:
		{
			system_manager_idp_17(str_pkg, comm_mode);
			break;
		}
		case IDP_18:
		{
			system_manager_idp_18(str_pkg, comm_mode);
			break;
		}
		case IDP_19:
		{
			system_manager_idp_19(str_pkg, comm_mode);
			break;
		}
		case IDP_21:
		{
			system_manager_idp_21(str_pkg, comm_mode);
			break;
		}
		case IDP_22:
		{
			system_manager_idp_22(str_pkg, comm_mode);
			break;
		}
		case IDP_23:
		{
			system_manager_idp_23(str_pkg, comm_mode);
			break;
		}
		case IDP_24:
		{
			system_manager_idp_24(str_pkg, comm_mode);
			break;
		}
		case IDP_26:
		{
			system_manager_idp_26(str_pkg, comm_mode);
			break;
		}
		case IDP_27:
		{
			system_manager_idp_27(str_pkg, comm_mode);
			break;
		}
		case IDP_28:
		{
			system_manager_idp_28(str_pkg, comm_mode);
			break;
		}
		case IDP_30:
		{
			system_manager_idp_30(str_pkg, comm_mode);
			break;
		}
		case IDP_32:
		{
			system_manager_idp_32(str_pkg, comm_mode);
			break;
		}
		case IDP_34:
		{
			system_manager_idp_34(str_pkg, comm_mode);
			break;
		}
		case IDP_31:
		{
			system_manager_idp_31(str_pkg, comm_mode);
			break;
		}
		case IDP_90:
		{
			system_manager_idp_90(str_pkg, comm_mode);
			break;
		}
		case IDP_91:
		{
			system_manager_idp_91(str_pkg, comm_mode);
			break;
		}
		case IDP_92:
		{
			system_manager_idp_92(str_pkg, comm_mode);
			break;
		}
		default:
		{
			ESP_LOGE(SYSTEM_MANAGER_TAG, "Invalid Package (%s)", buffer_request);
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, "Invalid Package");
			vTaskDelay(pdMS_TO_TICKS(2000));
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer_request);

			comm_app_send_idp_pack(CONFIG_HTTP_ERROR, COMM_HTTP_POST);
			break;
		}
		}
	}
	else
	{
		ESP_LOGE(SYSTEM_MANAGER_TAG, "%s, Invalid Payload from %i", str_pkg, comm_mode);
		LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, "Invalid characters found");
		vTaskDelay(pdMS_TO_TICKS(2000));
		LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, str_pkg);
	}
}

/**
 * @brief Handle IDP package type 0.
 *
 * This function handles IDP package type 0, extracting relevant data and sending the response.
 *
 * @param buffer The input buffer containing the request.
 * @param comm_mode The communication mode (e.g., COMM_HTTP_GET, COMM_MQTT).
 */
static void system_manager_idp_00(const char *buffer, comm_type comm_mode)
{
	if (comm_mode == COMM_HTTP_GET || comm_mode == COMM_MQTT || comm_mode == COMM_RF)
	{
		pivot_actions actions = {};
		char str_out[200] = {};
		char str_date_time[50] = {};
		uint16_t dwp = 0;
		uint8_t idp = 0;

		actuation_app_get_actions(&actions, sizeof(actions));
		dwp = idp_parser_create_pwd(actions);

		time_t timestamp = rtc_app_get_timestamp(false);
		rtc_app_get_str_date_time(timestamp, str_date_time);

		data_app_load(DATA_TYPE_INITIAL_ANGLE, &system_initial_angle);

		arg_pair_t arg_pairs[] = {
			{"uint8_t", &idp},
			{"string", system_id},
			{"uint16_t", &dwp},
			{"uint16_t", &actions.percentimeter},
			{"uint16_t", &system_initial_angle},
			{"uint16_t", &global_angle},
			{"string", str_date_time},
			{NULL, NULL}};

		if ((timestamp - system_rtc_percent) < 65)
		{
			actions.percentimeter = CONFIG_ACTIONS_UNDEF_VALUE;
		}

		idp_parser_create_package(str_out, arg_pairs);
		comm_app_send_idp_pack(str_out, comm_mode);
	}
}

/**
 * @brief Handle IDP package type 1.
 *
 * This function handles IDP package type 1, validates the data, saves the state, and sends the response.
 *
 * @param buffer The input buffer containing the request.
 * @param comm_mode The communication mode (e.g., COMM_HTTP_POST, COMM_MQTT).
 */
static void system_manager_idp_01(const char *buffer, comm_type comm_mode)
{
	if (comm_mode == COMM_HTTP_POST || comm_mode == COMM_MQTT || comm_mode == COMM_RF)
	{
		esp_err_t ret = ESP_FAIL;
		pivot_actions old_actions = {};
		pivot_actions new_actions = {};
		pivot_history new_history = {};

		char pivot_id[50] = {};
		uint16_t dwp = 0;
		uint8_t idp = 0;

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", pivot_id},
				{"uint16_t", &dwp},
				{"uint16_t", &new_actions.percentimeter},
				{"string", &new_actions.user},
				{NULL, NULL}};

		idp_parser_get_packet_data(buffer, arg_pairs);
		idp_parser_get_pwd(dwp, &new_actions);

		if (idp_parser_validate_actions(new_actions) == true)
		{
			data_app_load(DATA_TYPE_ACTIONS, &old_actions);

			if (new_actions.power_state == PIVOT_OFF)
			{
				if (global_angle != 655 && system_initial_angle != 655)
				{
					system_initial_angle = global_angle;
					data_app_save(DATA_TYPE_INITIAL_ANGLE, &system_initial_angle, sizeof(system_initial_angle));
				}
				else
				{
					data_app_load(DATA_TYPE_INITIAL_ANGLE, &system_initial_angle);
				}

				if(old_actions.power_state == PIVOT_ON)
				{
					if(strcmp(new_actions.user, "Irrigabras") == 0)
					{
						system_monitoring_pivot_shutdown(TYPE_HANGS_UP_IRRIGABRAS_APP, idp, "0", new_actions.user);
					}
					else if(strcmp(system_id, pivot_id) == 0)
					{
						system_monitoring_pivot_shutdown(TYPE_HANGS_UP_SOIL_APP, idp, "0", new_actions.user);
					}
				}

				actuation_app_get_actions(&new_actions, sizeof(new_actions));
				new_actions.percentimeter = 0;
				new_actions.power_state = PIVOT_OFF;
				new_actions.watering_state = PIVOT_DRY;

				// Save old History
				if (old_actions.power_state != PIVOT_OFF)
				{
					pivot_history old_history = {};
					old_history.end_date = rtc_app_get_timestamp(false);
					old_history.end_angle = global_angle;
					data_app_save(DATA_TYPE_OLD_HISTORY, &old_history, sizeof(old_history));
				}
			}

			ret = data_app_save(DATA_TYPE_ACTIONS, &new_actions, sizeof(new_actions));
			if (ret == ESP_OK)
			{
				if (comm_mode == COMM_HTTP_POST)
				{
					comm_app_send_idp_pack(CONFIG_HTTP_OK, COMM_HTTP_POST);
				}

				if ((new_actions.power_state != PIVOT_OFF) && (old_actions.power_state == PIVOT_OFF))
				{
					system_monitoring_barrier(new_actions, PHYSICAL_BARRIER);
				}
				// act on the equipment
				actuation_app_set_actions(new_actions, false);

				// time for the percentage to stabilize
				system_rtc_percent = rtc_app_get_timestamp(false);

				// send current status
				system_manager_idp_00("#00$", comm_main_mode);

				// save new history
				if ((new_actions.power_state != PIVOT_OFF) && (old_actions.power_state == PIVOT_OFF))
				{
					new_history.start_date = rtc_app_get_timestamp(false);
					new_history.start_angle = global_angle;
					memcpy(&new_history.actions, &new_actions, sizeof(new_actions));
					data_app_save(DATA_TYPE_HISTORY, &new_history, sizeof(new_history));
				}
			}
			else
			{
				if (comm_mode == COMM_HTTP_POST)
				{
					comm_app_send_idp_pack(CONFIG_HTTP_ERROR, COMM_HTTP_POST);
				}

				ESP_LOGE(SYSTEM_MANAGER_TAG, "Failed to save new state");
				LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, "Failed_to_save_new_state");
			}
		}
		else
		{
			if (comm_mode == COMM_HTTP_POST)
			{
				comm_app_send_idp_pack(CONFIG_HTTP_ERROR, COMM_HTTP_POST);
			}

			ESP_LOGE(SYSTEM_MANAGER_TAG, "Invalid state (%s)", buffer);
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, "Invalid state");
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
		}

		// start timer percent and pressure
		if (system_timer != NULL)
		{
			xTimerStop(system_timer, 1000);
			xTimerStart(system_timer, 1000);
		}

		if (strcmp(pivot_id, SYSTEM_MONITORING_TAG_COMMAND) != 0)
		{
			counter_reading_panel_off = NO_MANUAL_READING;
			data_app_save(DATA_TYPE_MANUAL_COUNTER, &counter_reading_panel_off, sizeof(counter_reading_panel_off));
		}
	}
}

/**
 * @brief Handle IDP package type 2.
 *
 * This function handles IDP package type 2, either saving network configuration or retrieving it based on the communication mode.
 *
 * @param buffer The input buffer containing the request.
 * @param comm_mode The communication mode (e.g., COMM_HTTP_GET, COMM_HTTP_POST).
 */
static void system_manager_idp_02(const char *buffer, comm_type comm_mode)
{
	bool mqtt_load_pkg = false;
	bool mqtt_save_pkg = false;
	uint8_t delimiter_num = idp_parser_get_delimiter(buffer);
	uint8_t expected_delimiter_num = (NETWORK_CONFIG_VAR_COUNT + 1);

	if (comm_mode == COMM_MQTT || comm_mode == COMM_RF)
	{
		if (delimiter_num >= expected_delimiter_num)
		{
			mqtt_save_pkg = true;
		}
		else if (delimiter_num == 1 || delimiter_num == 0)
		{
			mqtt_load_pkg = true;
		}
	}

	if (comm_mode == COMM_HTTP_POST || mqtt_save_pkg)
	{
		char pivot_id[50] = {};
		char str_out[200] = {};
		network_config net_config = {};
		uint8_t idp = 0;

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", pivot_id},
				{"string", net_config.gprs_id},
				{"string", net_config.modem_apn},
				{"string", net_config.wifi_ssid},
				{"string", net_config.wifi_pass},
				{NULL, NULL}};

		idp_parser_get_packet_data(buffer, arg_pairs);

		if (idp_parser_validate_idp_02(net_config))
		{
			// load old configuration
			network_config net_nvs_config = {};
			data_app_load(DATA_TYPE_NETWORK_CONFIG, &net_nvs_config);

			// save new configuration
			data_app_save(DATA_TYPE_NETWORK_CONFIG, &net_config, sizeof(net_config));

			if ((strcmp(net_config.wifi_ssid, net_nvs_config.wifi_ssid) != 0) || strcmp(net_config.wifi_pass, net_nvs_config.wifi_pass) != 0)
			{
				comm_app_wifi_config(net_config.wifi_ssid, net_config.wifi_pass);
				comm_app_wifi_reloader();
			}

			// send ACK
			comm_app_send_idp_pack(CONFIG_HTTP_OK, comm_mode);

			// send GPRS module
			idp = IDP_6;
			strcpy(system_id, net_config.gprs_id);

			arg_pair_t arg_pairs_3[] = {
				{"uint8_t", &idp},
				{"string", system_id},
				{"string", net_config.gprs_id},
				{"string", net_config.modem_apn},
				{NULL, NULL}};

			idp_parser_create_package(str_out, arg_pairs_3);
			comm_app_send_idp_pack(str_out, COMM_MQTT);
		}
		else
		{
			if (comm_mode == COMM_HTTP_POST)
			{
				comm_app_send_idp_pack(CONFIG_HTTP_ERROR, COMM_HTTP_POST);
			}

			ESP_LOGE(SYSTEM_MANAGER_TAG, "Network Config invalid packed data (%s)", buffer);
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, "Invalid data");
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
		}
	}
	else if (comm_mode == COMM_HTTP_GET || mqtt_load_pkg)
	{
		char str_out[200] = {};
		network_config net_config = {};
		uint8_t idp = IDP_2;

		data_app_load(DATA_TYPE_NETWORK_CONFIG, &net_config);

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", system_id},
				{"string", net_config.gprs_id},
				{"string", net_config.modem_apn},
				{"string", net_config.wifi_ssid},
				{"string", net_config.wifi_pass},
				{NULL, NULL}};

		idp_parser_create_package(str_out, arg_pairs);
		comm_app_send_idp_pack(str_out, comm_mode);
	}
	else
	{
		ESP_LOGE(SYSTEM_MANAGER_TAG, "Invalid configuration payload >> expected {%d} paramters, but receveid {%d}", (expected_delimiter_num + 1), (delimiter_num + 1));
		LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
	}
}

/**
 * @brief Handle IDP package type 3.
 *
 * This function handles IDP package type 3, saving or retrieving pivot configuration based on the communication mode.
 *
 * @param buffer The input buffer containing the request.
 * @param comm_mode The communication mode (e.g., COMM_HTTP_GET, COMM_HTTP_POST).
 */
static void system_manager_idp_03(const char *buffer, comm_type comm_mode)
{
	bool mqtt_load_pkg = false;
	bool mqtt_save_pkg = false;
	uint8_t delimiter_num = idp_parser_get_delimiter(buffer);
	uint8_t expected_delimiter_num = (PIVOT_CONFIG_VAR_COUNT + 1);

	if (comm_mode == COMM_MQTT || comm_mode == COMM_RF)
	{
		if (delimiter_num >= expected_delimiter_num) // number of fields in the payload - 1
		{
			mqtt_save_pkg = true;
		}
		else if (delimiter_num == 1 || delimiter_num == 0)
		{
			mqtt_load_pkg = true;
		}
	}

	if (comm_mode == COMM_HTTP_POST || mqtt_save_pkg)
	{
		uint8_t idp = 0;
		char pivot_id[50] = {};
		pivot_config new_config = {};

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", pivot_id},
				{"string", new_config.contactor},
				{"string", new_config.pressure},
				{"uint16_t", &new_config.pressurization_time},
				{"uint8_t", &new_config.on_time},
				{"uint8_t", &new_config.off_time},
				{"uint8_t", &new_config.read_time},
				{NULL, NULL}};

		idp_parser_get_packet_data(buffer, arg_pairs);

		if (idp_parser_validate_idp_03(new_config))
		{
			esp_err_t ret = data_app_save(DATA_TYPE_PIVOT_CONFIG, &new_config, sizeof(new_config));
			if (ret == ESP_OK)
			{
				pivot_virtual_config virtual_config = {};
				data_app_load(DATA_TYPE_VIRTUAL_BARRIER, &virtual_config);

				pivot_physical_config physical_config = {};
				data_app_load(DATA_TYPE_PHYSICAL_BARRIER, &physical_config);

				// send ACK
				comm_app_send_idp_pack(CONFIG_HTTP_OK, comm_mode);
				actuation_app_set_config(new_config);
				system_read_time = new_config.read_time;

				system_monitoring_stop();
				system_monitoring_start(physical_config, virtual_config, system_read_time);
			}
		}
		else
		{
			comm_app_send_idp_pack(CONFIG_HTTP_ERROR, comm_mode);
		}
	}
	else if (comm_mode == COMM_HTTP_GET || mqtt_load_pkg)
	{
		char str_out[200] = {};

		uint8_t idp = IDP_3;
		pivot_config config = {};

		data_app_load(DATA_TYPE_PIVOT_CONFIG, &config);

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", system_id},
				{"string", config.contactor},
				{"string", config.pressure},
				{"uint16_t", &config.pressurization_time},
				{"uint8_t", &config.on_time},
				{"uint8_t", &config.off_time},
				{"uint8_t", &config.read_time},
				{NULL, NULL}};

		// send
		idp_parser_create_package(str_out, arg_pairs);
		comm_app_send_idp_pack(str_out, comm_mode);
	}
	else
	{
		ESP_LOGE(SYSTEM_MANAGER_TAG, "Invalid configuration payload >> expected {%d} paramters, but receveid {%d}", (expected_delimiter_num + 1), (delimiter_num + 1));
		LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
	}
}

/**
 * @brief Handle IDP package type 4.
 *
 * This function handles IDP package type 4, saving or retrieving eco mode configuration based on the communication mode.
 *
 * @param buffer The input buffer containing the request.
 * @param comm_mode The communication mode (e.g., COMM_HTTP_GET, COMM_HTTP_POST).
 */
static void system_manager_idp_04(const char *buffer, comm_type comm_mode)
{
	bool mqtt_load_pkg = false;
	bool mqtt_save_pkg = false;
	uint8_t delimiter_num = idp_parser_get_delimiter(buffer);
	uint8_t expected_delimiter_num = (ECO_MODE_CONFIG_VAR_COUNT + 1);

	if (comm_mode == COMM_MQTT || comm_mode == COMM_RF)
	{
		if (delimiter_num >= expected_delimiter_num) // number of fields in the payload - 1
		{
			mqtt_save_pkg = true;
		}
		else if (delimiter_num == 1 || delimiter_num == 0)
		{
			mqtt_load_pkg = true;
		}
	}

	if (comm_mode == COMM_HTTP_POST || mqtt_save_pkg)
	{
		uint8_t idp = 0;
		char pivot_id[50] = {};
		eco_mode_config eco_mode = {};

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", pivot_id},
				{"uint32_t", &eco_mode.start_time},
				{"uint32_t", &eco_mode.end_time},
				{NULL, NULL}};

		idp_parser_get_packet_data(buffer, arg_pairs);
		if (idp_parser_validate_idp_04(eco_mode))
		{
			esp_err_t ret = data_app_save(DATA_TYPE_ECO_MODE_CONFIG, &eco_mode, sizeof(eco_mode));

			if (ret == ESP_OK)
			{
				// send ACK
				comm_app_send_idp_pack(CONFIG_HTTP_OK, comm_mode);
			}
			else
			{
				comm_app_send_idp_pack(CONFIG_HTTP_ERROR, comm_mode);
			}

			/*
			eco_mode_stop();
			eco_mode_start(eco_mode);
			*/
		}
		else
		{
			if (comm_mode == COMM_HTTP_POST)
			{
				comm_app_send_idp_pack(CONFIG_HTTP_ERROR, COMM_HTTP_POST);
			}

			ESP_LOGE(SYSTEM_MANAGER_TAG, "Eco Mode Config invalid packed data (%s)", buffer);
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, "Invalid data");
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
		}
	}
	else if (comm_mode == COMM_HTTP_GET || mqtt_load_pkg)
	{
		char str_out[200] = {};

		uint8_t idp = IDP_4;
		eco_mode_config eco_mode = {};

		data_app_load(DATA_TYPE_ECO_MODE_CONFIG, &eco_mode);

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", system_id},
				{"uint32_t", &eco_mode.start_time},
				{"uint32_t", &eco_mode.end_time},
				{NULL, NULL}};

		idp_parser_create_package(str_out, arg_pairs);
		comm_app_send_idp_pack(str_out, comm_mode);
	}
	else
	{
		ESP_LOGE(SYSTEM_MANAGER_TAG, "Invalid configuration payload >> expected {%d} paramters, but receveid {%d}", (expected_delimiter_num + 1), (delimiter_num + 1));
		LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
	}
}

/**
 * @brief Handle IDP package type 5.
 *
 * This function handles IDP package type 5, saving or retrieving sector configuration based on the communication mode.
 *
 * @param buffer The input buffer containing the request.
 * @param comm_mode The communication mode (e.g., COMM_HTTP_GET, COMM_HTTP_POST).
 */
static void system_manager_idp_05(const char *buffer, comm_type comm_mode)
{
	bool mqtt_load_pkg = false;
	bool mqtt_save_pkg = false;
	uint8_t delimiter_num = idp_parser_get_delimiter(buffer);
	uint8_t expected_delimiter_num = (SECTOR_CONFIG_VAR_COUNT + 1);

	if (comm_mode == COMM_MQTT || comm_mode == COMM_RF)
	{
		if (delimiter_num >= expected_delimiter_num)
		{
			mqtt_save_pkg = true;
		}
		else if (delimiter_num == 1 || delimiter_num == 0)
		{
			mqtt_load_pkg = true;
		}
	}

	if (comm_mode == COMM_HTTP_POST || mqtt_save_pkg)
	{
		uint8_t idp = 0;
		char pivot_id[50] = {};
		sector_config sector = {};

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", pivot_id},
				{"uint8_t", &sector.sector_number},
				{"uint16_t", &sector.sectors[0].start_angle},
				{"uint16_t", &sector.sectors[0].end_angle},
				{"uint16_t", &sector.sectors[1].start_angle},
				{"uint16_t", &sector.sectors[1].end_angle},
				{"uint16_t", &sector.sectors[2].start_angle},
				{"uint16_t", &sector.sectors[2].end_angle},
				{"uint16_t", &sector.sectors[3].start_angle},
				{"uint16_t", &sector.sectors[3].end_angle},
				{NULL, NULL}};

		idp_parser_get_packet_data(buffer, arg_pairs);
		if (idp_parser_validate_idp_05(sector))
		{
			esp_err_t ret = data_app_save(DATA_TYPE_SECTOR_CONFIG, &sector, sizeof(sector));

			if (ret == ESP_OK)
			{
				// send ACK
				comm_app_send_idp_pack(CONFIG_HTTP_OK, comm_mode);
				sectorization_stop();
				sectorization_start(sector);
			}
			else
			{
				comm_app_send_idp_pack(CONFIG_HTTP_ERROR, comm_mode);
			}
		}
		else
		{
			if (comm_mode == COMM_HTTP_POST)
			{
				comm_app_send_idp_pack(CONFIG_HTTP_ERROR, COMM_HTTP_POST);
			}

			ESP_LOGE(SYSTEM_MANAGER_TAG, "Sector Config invalid packed data (%s)", buffer);
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, "Invalid data");
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
		}
	}
	else if (comm_mode == COMM_HTTP_GET || mqtt_load_pkg)
	{
		char str_out[200] = {};

		uint8_t idp = IDP_5;
		sector_config sector = {};

		data_app_load(DATA_TYPE_SECTOR_CONFIG, &sector);

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", system_id},
				{"uint8_t", &sector.sector_number},
				{"uint16_t", &sector.sectors[0].start_angle},
				{"uint16_t", &sector.sectors[0].end_angle},
				{"uint16_t", &sector.sectors[1].start_angle},
				{"uint16_t", &sector.sectors[1].end_angle},
				{"uint16_t", &sector.sectors[2].start_angle},
				{"uint16_t", &sector.sectors[2].end_angle},
				{"uint16_t", &sector.sectors[3].start_angle},
				{"uint16_t", &sector.sectors[3].end_angle},
				{NULL, NULL}};

		idp_parser_create_package(str_out, arg_pairs);
		comm_app_send_idp_pack(str_out, comm_mode);
	}
	else
	{
		ESP_LOGE(SYSTEM_MANAGER_TAG, "Invalid configuration payload >> expected {%d} paramters, but receveid {%d}", (expected_delimiter_num + 1), (delimiter_num + 1));
		LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
	}
}

/**
 * @brief Handle IDP package type 6.
 *
 * This function handles IDP package type 6 for MQTT communication, sending GPRS module information.
 *
 * @param buffer The input buffer containing the request.
 * @param comm_mode The communication mode (e.g., COMM_MQTT).
 */
static void system_manager_idp_06(const char *buffer, comm_type comm_mode)
{
	if (comm_mode == COMM_MQTT || comm_mode == COMM_RF)
	{
		char str_out[200] = {};
		network_config net_config = {};
		uint8_t idp = 0;

		data_app_load(DATA_TYPE_NETWORK_CONFIG, &net_config);

		// send GPRS module
		idp = IDP_6;
		arg_pair_t arg_pairs_3[] = {
			{"uint8_t", &idp},
			{"string", net_config.gprs_id},
			{"string", net_config.modem_apn},
			{NULL, NULL}};

		idp_parser_create_package(str_out, arg_pairs_3);
		comm_app_send_idp_pack(str_out, COMM_MQTT);
	}
}

/**
 * @brief Handle IDP package type 7.
 *
 * This function handles IDP package type 7 for MQTT communication, updating the system with angle and timestamp information.
 *
 * @param buffer The input buffer containing the request.
 * @param comm_mode The communication mode (e.g., COMM_MQTT).
 */
static void system_manager_idp_07(const char *buffer, comm_type comm_mode)
{
	if (comm_mode == COMM_RF)
	{
		uint8_t idp = 0;
		time_t timestamp;
		// char utc[30] = {};

		// get angle
		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"uint16_t", &global_angle},
				{"uint16_t", &global_pressure},
				{"time_t", &timestamp},
				// {"string", utc},
				{NULL, NULL}};

		idp_parser_get_packet_data(buffer, arg_pairs);

		rtc_app_set_timestamp(timestamp);

		if (system_initial_angle == 655 && global_angle != 655)
		{
			system_initial_angle = global_angle;
			ESP_LOGW(SYSTEM_MANAGER_TAG, "Initial angle : %d", system_initial_angle);
			data_app_save(DATA_TYPE_INITIAL_ANGLE, &system_initial_angle, sizeof(system_initial_angle));
		}

		if (gps_flag_send_to_mqtt)
		{
			gprs_uart_send_event(buffer, strlen(buffer));
		}

		gps_flag_send_to_mqtt = false;
	}
	if (comm_mode == COMM_MQTT)
	{
		gps_flag_send_to_mqtt = true;
	}
}

/**
 * @brief Handle IDP package type 12.
 *
 * This function handles IDP package type 12, providing historical data for HTTP GET requests.
 *
 * @param buffer The input buffer containing the request.
 * @param comm_mode The communication mode (e.g., COMM_HTTP_GET).
 */
static void system_manager_idp_12(const char *buffer, comm_type comm_mode)
{
	if (comm_mode == COMM_HTTP_GET)
	{
		char buffer_out[600] = {};
		char str_out[200] = {};

		uint16_t dwp = 0;
		uint8_t idp = IDP_12;

		pivot_history load_history[CONFIG_HISTORY_MAX_VALUE] = {};
		data_app_load(DATA_TYPE_HISTORY, &load_history);

		for (uint8_t position = 0; position < CONFIG_HISTORY_MAX_VALUE; position++)
		{
			if (load_history[position].end_date != 0)
			{
				dwp = idp_parser_create_pwd(load_history[position].actions);

				arg_pair_t arg_pairs[] =
					{
						{"uint8_t", &idp},
						{"string", system_id},
						{"uint16_t", &load_history[position].start_angle},
						{"uint16_t", &load_history[position].end_angle},
						{"uint32_t", &load_history[position].start_date},
						{"uint32_t", &load_history[position].end_date},
						{"uint16_t", &dwp},
						{"uint16_t", &load_history[position].actions.percentimeter},
						{NULL, NULL}};

				idp_parser_create_package(str_out, arg_pairs);

				strcat(buffer_out, str_out);
				strcat(buffer_out, "\n");
			}
		}

		comm_app_send_idp_pack(buffer_out, COMM_HTTP_GET);
	}
}

/**
 * @brief Handles IDP 13 requests for scheduling deletion.
 *
 * This function handles the deletion of schedules based on the provided parameters.
 *
 * @param buffer The input buffer containing request data.
 * @param comm_mode The communication mode (HTTP or MQTT).
 */
static void system_manager_idp_13(const char *buffer, comm_type comm_mode)
{
	if (comm_mode == COMM_HTTP_POST || comm_mode == COMM_MQTT || comm_mode == COMM_RF)
	{
		// init scheduling
		pivot_scheduling_date scheduling_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
		pivot_scheduling_angle scheduling_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};
		pivot_scheduling_off_date scheduling_off_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
		pivot_scheduling_off_angle scheduling_off_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};

		uint8_t idp = 0;
		char str_out[200] = {};
		char pivot_id[50] = {};
		char str_author[30] = {};
		char scheduling_id[20] = {};

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", pivot_id},
				{"string", scheduling_id},
				{"string", str_author},
				{NULL, NULL}};
		idp_parser_get_packet_data(buffer, arg_pairs);

		esp_err_t err = data_app_delete_scheduling(scheduling_id);
		if (err == ESP_OK)
		{
			// send ack
			arg_pair_t arg_pairs_2[] =
				{
					{"uint8_t", &idp},
					{"string", system_id},
					{"string", scheduling_id},
					{NULL, NULL}};
			idp_parser_create_package(str_out, arg_pairs_2);

			if (comm_mode == COMM_HTTP_POST)
			{
				comm_app_send_idp_pack(CONFIG_HTTP_OK, COMM_HTTP_POST);
			}
			comm_app_send_idp_pack(str_out, comm_main_mode);

			data_app_load(DATA_TYPE_SCHEDULING_DATE, &scheduling_date);
			data_app_load(DATA_TYPE_SCHEDULING_ANGLE, &scheduling_angle);
			data_app_load(DATA_TYPE_SCHEDULING_OFF_DATE, &scheduling_off_date);
			data_app_load(DATA_TYPE_SCHEDULING_OFF_ANGLE, &scheduling_off_angle);

			scheduling_start(IDP_14, &scheduling_date);
			scheduling_start(IDP_15, &scheduling_angle);
			scheduling_start(IDP_16, &scheduling_off_date);
			scheduling_start(IDP_17, &scheduling_off_angle);
		}
		else
		{
			if (comm_mode == COMM_HTTP_POST)
			{
				comm_app_send_idp_pack(CONFIG_HTTP_ERROR, COMM_HTTP_POST);
			}

			ESP_LOGE(SYSTEM_MANAGER_TAG, "deleting schedule angle id : %s", scheduling_id);
		}
	}
}

/**
 * @brief Handles IDP 14 requests for scheduling creation.
 *
 * This function handles the creation of schedules based on the provided parameters.
 *
 * @param buffer The input buffer containing request data.
 * @param comm_mode The communication mode (HTTP or MQTT).
 */
static void system_manager_idp_14(const char *buffer, comm_type comm_mode)
{
	if (comm_mode == COMM_HTTP_POST || comm_mode == COMM_MQTT || comm_mode == COMM_RF)
	{
		char str_out[200] = {};
		char pivot_id[50] = {};
		// char str_author[30] = {};

		pivot_scheduling_date scheduling = {};
		uint16_t dwp = 0;
		uint8_t idp = 0;

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", pivot_id},
				{"uint32_t", &scheduling.start_date},
				{"uint32_t", &scheduling.end_date},
				{"uint16_t", &dwp},
				{"uint16_t", &scheduling.actions.percentimeter},
				{"string", &scheduling.str_author},
				{NULL, NULL}};

		idp_parser_get_packet_data(buffer, arg_pairs);
		idp_parser_get_pwd(dwp, &scheduling.actions);

		if (idp_parser_validate_idp_14(scheduling, scheduling.str_author))
		{
			pivot_scheduling_date scheduling_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
			data_app_load(DATA_TYPE_SCHEDULING_DATE, &scheduling_date);

			for (uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
			{
				if (strcmp(scheduling_date[position].scheduling_id, "") == 0)
				{
					memcpy(&scheduling_date[position], &scheduling, sizeof(scheduling_date[position]));

					if (idp_parser_validate_actions(scheduling.actions) == true)
					{
						// get_rtc
						scheduling_date[position].start_date += rtc_app_get_timestamp(false);
						scheduling_date[position].end_date += rtc_app_get_timestamp(false);

						// gen Key
						data_app_gen_scheduling_key((char *)&scheduling_date[position].scheduling_id);
						strcpy(scheduling.scheduling_id, (char *)&scheduling_date[position].scheduling_id);

						data_app_save(DATA_TYPE_SCHEDULING_DATE, &scheduling_date, sizeof(scheduling_date));

						scheduling_start(idp, scheduling_date);

						// send ack
						if (comm_mode == COMM_HTTP_POST)
						{
							arg_pair_t arg_pairs_2[] =
								{
									{"uint8_t", &idp},
									{"string", system_id},
									{"string", scheduling.scheduling_id},
									{"uint32_t", &scheduling.start_date},
									{"uint32_t", &scheduling.end_date},
									{"uint16_t", &dwp},
									{"uint16_t", &scheduling.actions.percentimeter},
									{NULL, NULL}};

							idp_parser_create_package(str_out, arg_pairs_2);

							comm_app_send_idp_pack(CONFIG_HTTP_OK, COMM_HTTP_POST);
							comm_app_send_idp_pack(str_out, comm_main_mode);
						}
						else
						{
							arg_pair_t arg_pairs_2[] =
								{
									{"uint8_t", &idp},
									{"string", system_id},
									{"string", scheduling.scheduling_id},
									{NULL, NULL}};

							idp_parser_create_package(str_out, arg_pairs_2);
							comm_app_send_idp_pack(str_out, comm_mode);
						}

						ESP_LOGI(SYSTEM_MANAGER_TAG, "Save schedule date id : %s", scheduling_date[position].scheduling_id);
					}
					else
					{
						if (comm_mode == COMM_HTTP_POST)
						{
							comm_app_send_idp_pack(CONFIG_HTTP_ERROR, COMM_HTTP_POST);
						}

						ESP_LOGE(SYSTEM_MANAGER_TAG, "Scheduling invalid state (%s)", buffer);
						LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, "Invalid state");
						LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
					}

					break;
				}
			}
		}
		else
		{
			if (comm_mode == COMM_HTTP_POST)
			{
				comm_app_send_idp_pack(CONFIG_HTTP_ERROR, COMM_HTTP_POST);
			}

			ESP_LOGE(SYSTEM_MANAGER_TAG, "Scheduling invalid packed data (%s)", buffer);
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, "Invalid data");
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
		}
	}
	else if (comm_mode == COMM_HTTP_GET)
	{
		char buffer_out[500] = {};
		char str_out[200] = {};

		uint16_t dwp = 0;
		uint8_t idp = IDP_14;

		pivot_scheduling_date scheduling_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
		data_app_load(DATA_TYPE_SCHEDULING_DATE, &scheduling_date);

		for (uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
		{
			dwp = idp_parser_create_pwd(scheduling_date[position].actions);

			if (dwp != 0)
			{
				arg_pair_t arg_pairs[] =
					{
						{"uint8_t", &idp},
						{"string", system_id},
						{"string", scheduling_date[position].scheduling_id},
						{"uint32_t", &scheduling_date[position].start_date},
						{"uint32_t", &scheduling_date[position].end_date},
						{"uint16_t", &dwp},
						{"uint16_t", &scheduling_date[position].actions.percentimeter},
						{NULL, NULL}};

				idp_parser_create_package(str_out, arg_pairs);

				strcat(buffer_out, str_out);
				strcat(buffer_out, "\n");
			}
		}

		comm_app_send_idp_pack(buffer_out, COMM_HTTP_GET);
	}
}

/**
 * @brief Handles IDP 15 requests for angle scheduling creation.
 *
 * This function handles the creation of angle-based schedules.
 *
 * @param buffer The input buffer containing request data.
 * @param comm_mode The communication mode (HTTP or MQTT).
 */
static void system_manager_idp_15(const char *buffer, comm_type comm_mode)
{
	if (comm_mode == COMM_HTTP_POST || comm_mode == COMM_MQTT || comm_mode == COMM_RF)
	{
		char str_out[200] = {};
		char pivot_id[50] = {};
		char str_author[30] = {};

		pivot_scheduling_angle scheduling = {};
		uint16_t dwp = 0;
		uint8_t idp = 0;

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", pivot_id},
				{"uint32_t", &scheduling.start_date},
				{"uint16_t", &scheduling.end_angle},
				{"uint16_t", &dwp},
				{"uint16_t", &scheduling.actions.percentimeter},
				{"string", str_author},
				{NULL, NULL}};

		idp_parser_get_packet_data(buffer, arg_pairs);
		idp_parser_get_pwd(dwp, &scheduling.actions);

		if (idp_parser_validate_idp_15(scheduling, str_author))
		{
			pivot_scheduling_angle scheduling_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};
			data_app_load(DATA_TYPE_SCHEDULING_ANGLE, &scheduling_angle);

			for (uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
			{
				if (strcmp(scheduling_angle[position].scheduling_id, "") == 0)
				{
					memcpy(&scheduling_angle[position], &scheduling, sizeof(scheduling_angle[position]));

					if (idp_parser_validate_actions(scheduling.actions) == true)
					{
						// get_rtc
						scheduling_angle[position].start_date += rtc_app_get_timestamp(false);

						// gen key
						data_app_gen_scheduling_key((char *)&scheduling_angle[position].scheduling_id);
						data_app_save(DATA_TYPE_SCHEDULING_ANGLE, &scheduling_angle, sizeof(scheduling_angle));

						strcpy((char *)&scheduling.scheduling_id, (char *)&scheduling_angle[position].scheduling_id);

						scheduling_start(idp, scheduling_angle);

						// send ack
						if (comm_mode == COMM_HTTP_POST)
						{
							arg_pair_t arg_pairs_2[] =
								{
									{"uint8_t", &idp},
									{"string", system_id},
									{"string", scheduling.scheduling_id},
									{"uint32_t", &scheduling.start_date},
									{"uint16_t", &scheduling.end_angle},
									{"uint16_t", &dwp},
									{"uint16_t", &scheduling.actions.percentimeter},
									{NULL, NULL}};

							idp_parser_create_package(str_out, arg_pairs_2);

							comm_app_send_idp_pack(CONFIG_HTTP_OK, COMM_HTTP_POST);
							comm_app_send_idp_pack(str_out, comm_main_mode);
						}
						else
						{
							arg_pair_t arg_pairs_2[] =
								{
									{"uint8_t", &idp},
									{"string", system_id},
									{"string", scheduling.scheduling_id},
									{NULL, NULL}};

							idp_parser_create_package(str_out, arg_pairs_2);
							comm_app_send_idp_pack(str_out, comm_mode);
						}

						ESP_LOGI(SYSTEM_MANAGER_TAG, "Save schedule angle id : %s", scheduling_angle[position].scheduling_id);
					}
					else
					{
						if (comm_mode == COMM_HTTP_POST)
						{
							comm_app_send_idp_pack(CONFIG_HTTP_ERROR, COMM_HTTP_POST);
						}

						ESP_LOGE(SYSTEM_MANAGER_TAG, "Scheduling invalid state (%s)", buffer);
						LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, "Invalid state");
						LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
					}

					break;
				}
			}
		}
		else
		{
			if (comm_mode == COMM_HTTP_POST)
			{
				comm_app_send_idp_pack(CONFIG_HTTP_ERROR, COMM_HTTP_POST);
			}

			ESP_LOGE(SYSTEM_MANAGER_TAG, "Scheduling invalid packed data (%s)", buffer);
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, "Invalid data");
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
		}
	}
	else if (comm_mode == COMM_HTTP_GET)
	{
		char buffer_out[500] = {};
		char str_out[200] = {};

		uint16_t dwp = 0;
		uint8_t idp = IDP_15;

		pivot_scheduling_angle scheduling_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};
		data_app_load(DATA_TYPE_SCHEDULING_ANGLE, &scheduling_angle);

		for (uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
		{
			dwp = idp_parser_create_pwd(scheduling_angle[position].actions);

			if (dwp != 0)
			{
				arg_pair_t arg_pairs[] =
					{
						{"uint8_t", &idp},
						{"string", system_id},
						{"string", scheduling_angle[position].scheduling_id},
						{"uint32_t", &scheduling_angle[position].start_date},
						{"uint16_t", &scheduling_angle[position].end_angle},
						{"uint16_t", &dwp},
						{"uint16_t", &scheduling_angle[position].actions.percentimeter},
						{NULL, NULL}};

				idp_parser_create_package(str_out, arg_pairs);

				strcat(buffer_out, str_out);
				strcat(buffer_out, "\n");
			}
		}

		comm_app_send_idp_pack(buffer_out, COMM_HTTP_GET);
	}
}

/**
 * @brief Handles IDP 16 requests for scheduling off-dates creation.
 *
 * This function handles the creation of schedules with off-dates.
 *
 * @param buffer The input buffer containing request data.
 * @param comm_mode The communication mode (HTTP or MQTT).
 */
static void system_manager_idp_16(const char *buffer, comm_type comm_mode)
{
	if (comm_mode == COMM_HTTP_POST || comm_mode == COMM_MQTT || comm_mode == COMM_RF)
	{
		char str_out[200] = {};
		char str_author[30] = {};
		char pivot_id[50] = {};

		pivot_scheduling_off_date scheduling = {};
		uint8_t idp = 0;

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", pivot_id},
				{"uint32_t", &scheduling.end_date},
				{"string", str_author},
				{NULL, NULL}};

		idp_parser_get_packet_data(buffer, arg_pairs);

		if (idp_parser_validate_idp_16(scheduling, str_author))
		{
			pivot_scheduling_off_date scheduling_off_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
			data_app_load(DATA_TYPE_SCHEDULING_OFF_DATE, &scheduling_off_date);

			for (uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
			{
				if (strcmp(scheduling_off_date[position].scheduling_id, "") == 0)
				{
					memcpy(&scheduling_off_date[position], &scheduling, sizeof(scheduling_off_date[position]));

					// get_rtc
					scheduling_off_date[position].end_date += rtc_app_get_timestamp(false);

					data_app_gen_scheduling_key((char *)&scheduling_off_date[position].scheduling_id);
					data_app_save(DATA_TYPE_SCHEDULING_OFF_DATE, &scheduling_off_date, sizeof(scheduling_off_date));

					scheduling_start(idp, scheduling_off_date);

					ESP_LOGI(SYSTEM_MANAGER_TAG, "Save schedule date id : %s", scheduling_off_date[position].scheduling_id);

					// send ack
					if (comm_mode == COMM_HTTP_POST)
					{
						arg_pair_t arg_pairs_2[] =
							{
								{"uint8_t", &idp},
								{"string", system_id},
								{"string", scheduling_off_date[position].scheduling_id},
								{"uint32_t", &scheduling.end_date},
								{NULL, NULL}};

						idp_parser_create_package(str_out, arg_pairs_2);

						comm_app_send_idp_pack(CONFIG_HTTP_OK, COMM_HTTP_POST);
						comm_app_send_idp_pack(str_out, comm_main_mode);
					}
					else
					{
						arg_pair_t arg_pairs_2[] =
							{
								{"uint8_t", &idp},
								{"string", system_id},
								{"string", scheduling_off_date[position].scheduling_id},
								{NULL, NULL}};

						idp_parser_create_package(str_out, arg_pairs_2);
						comm_app_send_idp_pack(str_out, comm_mode);
					}

					break;
				}
			}
		}
		else
		{
			if (comm_mode == COMM_HTTP_POST)
			{
				comm_app_send_idp_pack(CONFIG_HTTP_ERROR, COMM_HTTP_POST);
			}

			ESP_LOGE(SYSTEM_MANAGER_TAG, "Scheduling invalid packed data (%s)", buffer);
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, "Invalid data");
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
		}
	}
	else if (comm_mode == COMM_HTTP_GET)
	{
		char buffer_out[500] = {};
		char str_out[200] = {};

		uint8_t idp = IDP_16;

		pivot_scheduling_off_date scheduling_off_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
		data_app_load(DATA_TYPE_SCHEDULING_OFF_DATE, &scheduling_off_date);

		for (uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
		{
			if (scheduling_off_date[position].end_date != 0)
			{
				if (scheduling_off_date[position].end_date != 0)
				{
					arg_pair_t arg_pairs[] =
						{
							{"uint8_t", &idp},
							{"string", system_id},
							{"string", scheduling_off_date[position].scheduling_id},
							{"uint32_t", &scheduling_off_date[position].end_date},
							{NULL, NULL}};

					idp_parser_create_package(str_out, arg_pairs);

					strcat(buffer_out, str_out);
					strcat(buffer_out, "\n");
				}
			}
		}

		comm_app_send_idp_pack(buffer_out, COMM_HTTP_GET);
	}
}

/**
 * @brief Handles IDP 17 requests for scheduling off-angles creation.
 *
 * This function handles the creation of schedules with off-angles.
 *
 * @param buffer The input buffer containing request data.
 * @param comm_mode The communication mode (HTTP or MQTT).
 */
static void system_manager_idp_17(const char *buffer, comm_type comm_mode)
{
	if (comm_mode == COMM_HTTP_POST || comm_mode == COMM_MQTT || comm_mode == COMM_RF)
	{
		char str_out[200] = {};
		char str_author[30] = {};
		char pivot_id[50] = {};

		pivot_scheduling_off_angle scheduling = {};
		uint8_t idp = 0;

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", pivot_id},
				{"uint16_t", &scheduling.end_angle},
				{"string", str_author},
				{NULL, NULL}};

		idp_parser_get_packet_data(buffer, arg_pairs);

		if (idp_parser_validate_idp_17(scheduling, str_author))
		{
			pivot_scheduling_off_angle scheduling_off_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};
			data_app_load(DATA_TYPE_SCHEDULING_OFF_ANGLE, &scheduling_off_angle);

			for (uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
			{
				if (strcmp(scheduling_off_angle[position].scheduling_id, "") == 0)
				{
					memcpy(&scheduling_off_angle[position], &scheduling, sizeof(scheduling_off_angle[position]));

					data_app_gen_scheduling_key((char *)scheduling_off_angle[position].scheduling_id);
					data_app_save(DATA_TYPE_SCHEDULING_OFF_ANGLE, &scheduling_off_angle, sizeof(scheduling_off_angle));

					scheduling_start(idp, &scheduling_off_angle);

					ESP_LOGI(SYSTEM_MANAGER_TAG, "Save schedule date id : %s", scheduling_off_angle[position].scheduling_id);

					// send ack
					if (comm_mode == COMM_HTTP_POST)
					{
						arg_pair_t arg_pairs_2[] =
							{
								{"uint8_t", &idp},
								{"string", system_id},
								{"string", scheduling_off_angle[position].scheduling_id},
								{"uint16_t", &scheduling.end_angle},
								{NULL, NULL}};

						idp_parser_create_package(str_out, arg_pairs_2);

						comm_app_send_idp_pack(CONFIG_HTTP_OK, COMM_HTTP_POST);
						comm_app_send_idp_pack(str_out, comm_main_mode);
					}
					else if (comm_mode == COMM_MQTT)
					{
						arg_pair_t arg_pairs_2[] =
							{
								{"uint8_t", &idp},
								{"string", system_id},
								{"string", scheduling_off_angle[position].scheduling_id},
								{NULL, NULL}};

						idp_parser_create_package(str_out, arg_pairs_2);
						comm_app_send_idp_pack(str_out, comm_mode);
					}

					break;
				}
			}
		}
		else
		{
			if (comm_mode == COMM_HTTP_POST)
			{
				comm_app_send_idp_pack(CONFIG_HTTP_ERROR, COMM_HTTP_POST);
			}

			ESP_LOGE(SYSTEM_MANAGER_TAG, "Scheduling invalid packed data (%s)", buffer);
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, "Invalid data");
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
		}
	}
	else if (comm_mode == COMM_HTTP_GET)
	{
		char buffer_out[500] = {};
		char str_out[200] = {};

		uint8_t idp = IDP_16;
		pivot_scheduling_off_angle scheduling_off_angle = {};

		data_app_load(DATA_TYPE_SCHEDULING_OFF_ANGLE, &scheduling_off_angle);
		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", system_id},
				{"string", scheduling_off_angle.scheduling_id},
				{"uint16_t", &scheduling_off_angle.end_angle},
				{NULL, NULL}};

		idp_parser_create_package(str_out, arg_pairs);

		strcat(buffer_out, str_out);
		strcat(buffer_out, "\n");

		comm_app_send_idp_pack(buffer_out, COMM_HTTP_GET);
	}

	return;
}

/**
 * @brief Handles IDP 18 requests for schedule deletion.
 *
 * This function handles the deletion of schedules based on provided parameters.
 *
 * @param buffer The input buffer containing request data.
 * @param comm_mode The communication mode (MQTT).
 */
static void system_manager_idp_18(const char *buffer, comm_type comm_mode)
{
	if (comm_mode == COMM_MQTT || comm_mode == COMM_RF)
	{
		char str_out[200] = {};
		char pivot_id[50] = {};
		char scheduling_id[10] = {};
		uint8_t idp = 0;

		arg_pair_t arg_get[] =
			{
				{"uint8_t", &idp},
				{"string", pivot_id},
				{"string", scheduling_id},
				{NULL, NULL}};

		idp_parser_get_packet_data(buffer, arg_get);

		// create package - send IDP 18
		arg_pair_t arg_send[] = {
			{"uint8_t", &idp},
			{"string", system_id},
			{"string", scheduling_id},
			{NULL, NULL}};

		idp_parser_create_package(str_out, arg_send);
		comm_app_send_idp_pack(str_out, comm_mode);
	}
}

/**
 * @brief Handle IDP package type 19.
 *
 * This function handles IDP package type 19, extracting Pressure and Rain Total data, and sending the response.
 *
 * @param buffer The input buffer containing the request.
 * @param comm_mode The communication mode (e.g., COMM_HTTP_GET, COMM_MQTT).
 */
static void system_manager_idp_19(const char* buffer, comm_type comm_mode)
{
    if (comm_mode == COMM_HTTP_GET || comm_mode == COMM_MQTT)
    {
        pivot_actions actions = {};
        char str_out[200] = {};
        char str_date_time[50] = {};
        uint16_t dwp = 0;
        uint8_t idp = 19;

        actuation_app_get_actions(&actions, sizeof(actions));
        dwp = idp_parser_create_pwd(actions);

        time_t timestamp = rtc_app_get_timestamp(false);
        rtc_app_get_str_date_time(timestamp, str_date_time);

        arg_pair_t arg_pairs[] = {
            { "uint8_t", &idp },
            { "string", system_id },
            { "uint16_t", &dwp },
            { "uint16_t", &global_pressure },
            { "string", str_date_time },
            { NULL, NULL }
        };

        idp_parser_create_package(str_out, arg_pairs);
        comm_app_send_idp_pack(str_out, comm_mode);
    }
}

/**
 * @brief Handles IDP 21 requests for timestamp update.
 *
 * This function handles the update of the system timestamp based on the provided parameters.
 *
 * @param buffer The input buffer containing request data.
 * @param comm_mode The communication mode (MQTT).
 */
static void system_manager_idp_21(const char *buffer, comm_type comm_mode)
{
	if (comm_mode == COMM_MQTT || comm_mode == COMM_RF)
	{
		uint8_t idp = 0;
		time_t timestamp;
		char pivot_id[50] = {};

		// get timestamp
		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", pivot_id},
				{"time_t", &timestamp},
				{NULL, NULL}};

		idp_parser_get_packet_data(buffer, arg_pairs);
		if (idp_parser_validate_idp_21(timestamp))
		{
			rtc_app_set_timestamp(timestamp);
		}
		else
		{
			ESP_LOGE(SYSTEM_MANAGER_TAG, "Scheduling invalid packed data (%s)", buffer);
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, "Invalid data");
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
		}
	}
}

/**
 * @brief Handles IDP 22 requests for return configuration modification.
 *
 * This function handles the modification of return configuration.
 *
 * @param buffer The input buffer containing request data.
 * @param comm_mode The communication mode (HTTP or MQTT).
 */
static void system_manager_idp_22(const char *buffer, comm_type comm_mode)
{
	bool mqtt_load_pkg = false;
	bool mqtt_save_pkg = false;
	uint8_t delimiter_num = idp_parser_get_delimiter(buffer);
	uint8_t expected_delimiter_num = (PIVOT_PHYSICAL_BARRIER_CONFIG_VAR_COUNT + 1);

	if (comm_mode == COMM_MQTT || comm_mode == COMM_RF)
	{
		if (delimiter_num >= expected_delimiter_num)
		{
			mqtt_save_pkg = true;
		}
		else if (delimiter_num == 1 || delimiter_num == 0)
		{
			mqtt_load_pkg = true;
		}
	}

	if (comm_mode == COMM_HTTP_POST || mqtt_save_pkg)
	{
		uint8_t idp = 0;
		char pivot_id[50] = {};
		pivot_physical_config physical_config = {};
		pivot_virtual_config virtual_config = {};

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", pivot_id},
				{"uint16_t", &physical_config.start_angle_physical_barrier},
				{"uint16_t", &physical_config.end_angle_physical_barrier},
				{"bool", &physical_config.automatic_return},
				{"bool", &physical_config.water_return},
				{"uint8_t", &physical_config.time_leaving_barrier},
				{NULL, NULL}};

		idp_parser_get_packet_data(buffer, arg_pairs);

		if (idp_parser_validate_idp_22(physical_config))
		{
			esp_err_t ret = data_app_save(DATA_TYPE_PHYSICAL_BARRIER, &physical_config, sizeof(physical_config));
			if (ret == ESP_OK)
			{
				actuation_app_leaving_barrier_time(physical_config);
				// send ACK
				comm_app_send_idp_pack(CONFIG_HTTP_OK, comm_mode);
				system_monitoring_stop();
				system_monitoring_start(physical_config, virtual_config, system_read_time);
			}
			else
			{
				comm_app_send_idp_pack(CONFIG_HTTP_ERROR, comm_mode);
			}
		}
		else
		{
			if (comm_mode == COMM_HTTP_POST)
			{
				comm_app_send_idp_pack(CONFIG_HTTP_ERROR, COMM_HTTP_POST);
			}

			ESP_LOGE(SYSTEM_MANAGER_TAG, "Return Config invalid packed data (%s)", buffer);
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, "Invalid data");
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
		}
	}
	else if (comm_mode == COMM_HTTP_GET || mqtt_load_pkg)
	{
		char str_out[200] = {};

		uint8_t idp = IDP_22;
		pivot_physical_config physical_config = {};

		data_app_load(DATA_TYPE_PHYSICAL_BARRIER, &physical_config);

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", system_id},
				{"uint16_t", &physical_config.start_angle_physical_barrier},
				{"uint16_t", &physical_config.end_angle_physical_barrier},
				{"bool", &physical_config.automatic_return},
				{"bool", &physical_config.water_return},
				{"uint8_t", &physical_config.time_leaving_barrier},
				{NULL, NULL}};
		// send
		idp_parser_create_package(str_out, arg_pairs);
		comm_app_send_idp_pack(str_out, comm_mode);
	}
	else
	{
		ESP_LOGE(SYSTEM_MANAGER_TAG, "Invalid configuration payload >> expected {%d} paramters, but receveid {%d}", (expected_delimiter_num + 1), (delimiter_num + 1));
		LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
	}
}

/**
 * @brief Handles IDP 23 requests for GPS configuration modification via LoraMesh.
 *
 * This function handles the configuration of the GPS through LoraMesh communication.
 *
 * @param buffer The input buffer containing request data.
 * @param comm_mode The communication mode (HTTP or MQTT).
 */
static void system_manager_idp_23(const char *buffer, comm_type comm_mode)
{
	bool mqtt_load_pkg = false;
	bool mqtt_save_pkg = false;
	uint8_t delimiter_num = idp_parser_get_delimiter(buffer);
	uint8_t expected_delimiter_num = (GPS_CONFIG_VAR_COUNT + 1);

	if (comm_mode == COMM_MQTT)
	{
		if (delimiter_num >= expected_delimiter_num)
		{
			mqtt_save_pkg = true;
		}
		else if (delimiter_num == 1 || delimiter_num == 0)
		{
			mqtt_load_pkg = true;
		}
	}

	if (mqtt_save_pkg || comm_mode == COMM_HTTP_POST)
	{
		size_t len_buffer = strlen(buffer);
		size_t len_buffer_gps_config = len_buffer + 3; // for 0x01 and 0x00 and  null terminator \0
		char buffer_gps_config[len_buffer_gps_config];

		prepare_gps_config_message(buffer, buffer_gps_config);

		esp_err_t ret = rf_uart_send_event(buffer_gps_config, len_buffer_gps_config);

		if (ret == ESP_OK)
		{
			// send ACK
			comm_app_send_idp_pack(CONFIG_HTTP_OK, comm_mode);
		}
		else
		{
			comm_app_send_idp_pack(CONFIG_HTTP_ERROR, comm_mode);
		}
	}
	else if (comm_mode == COMM_HTTP_GET)
	{
		char str_out[200] = {};

		uint8_t idp = IDP_23;
		gps_config gps_config = {};

		data_app_load(DATA_TYPE_GPS_CONFIG, &gps_config);

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", system_id},
				{"uint8_t", &gps_config.sinal_lat},
				{"string", &gps_config.latitude},
				{"uint8_t", &gps_config.sinal_lon},
				{"string", &gps_config.longitude},
				{"uint16_t", &gps_config.time_payload},
				{"uint16_t", &gps_config.offset},
				{NULL, NULL}};

		// send nvs saved config
		idp_parser_create_package(str_out, arg_pairs);
		comm_app_send_idp_pack(str_out, COMM_HTTP_GET);
	}
	else if (mqtt_load_pkg)
	{
		pivot_actions actions = {};
		actuation_app_get_actions(&actions, sizeof(actions));
		if (actions.power_state == PIVOT_ON)
		{
			size_t len_buffer = strlen(buffer);
			size_t len_buffer_gps_config = len_buffer + 3; // for 0x01 and 0x00 and  null terminator \0
			char buffer_gps_config[len_buffer_gps_config];

			prepare_gps_config_message(buffer, buffer_gps_config);

			esp_err_t ret = rf_uart_send_event(buffer_gps_config, len_buffer_gps_config);

			if (ret == ESP_OK)
			{
				// send ACK
				comm_app_send_idp_pack(CONFIG_HTTP_OK, comm_mode);
			}
			else
			{
				comm_app_send_idp_pack(CONFIG_HTTP_ERROR, comm_mode);
			}
		}
		else
		{
			char str_out[200] = {};

			uint8_t idp = IDP_23;
			gps_config gps_config = {};

			data_app_load(DATA_TYPE_GPS_CONFIG, &gps_config);

			arg_pair_t arg_pairs[] =
				{
					{"uint8_t", &idp},
					{"string", system_id},
					{"uint8_t", &gps_config.sinal_lat},
					{"string", &gps_config.latitude},
					{"uint8_t", &gps_config.sinal_lon},
					{"string", &gps_config.longitude},
					{"uint16_t", &gps_config.time_payload},
					{"uint16_t", &gps_config.offset},
					{NULL, NULL}};

			// send nvs saved config
			idp_parser_create_package(str_out, arg_pairs);
			comm_app_send_idp_pack(str_out, COMM_MQTT);
		}
	}
	else if (comm_mode == COMM_RF)
	{
		uint8_t idp = IDP_23;
		char pivot_id[50] = {};
		gps_config gps_config = {};

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", pivot_id},
				{"uint8_t", &gps_config.sinal_lat},
				{"string", &gps_config.latitude},
				{"uint8_t", &gps_config.sinal_lon},
				{"string", &gps_config.longitude},
				{"uint16_t", &gps_config.time_payload},
				{"uint16_t", &gps_config.offset},
				{NULL, NULL}};

		idp_parser_get_packet_data(buffer, arg_pairs);
		if (idp_parser_validate_idp_23(gps_config))
		{
			esp_err_t ret = data_app_save(DATA_TYPE_GPS_CONFIG, &gps_config, sizeof(gps_config));
			if (ret == ESP_OK)
			{
				gprs_uart_send_event(buffer, strlen(buffer));
			}
			else
			{
				comm_app_send_idp_pack(CONFIG_HTTP_ERROR, COMM_MQTT);
			}
		}
		else
		{
			if (comm_mode == COMM_HTTP_POST)
			{
				comm_app_send_idp_pack(CONFIG_HTTP_ERROR, COMM_HTTP_POST);
			}

			ESP_LOGE(SYSTEM_MANAGER_TAG, "RF_GPS invalid packed data (%s)", buffer);
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, "Invalid data");
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
		}
	}
	else
	{
		ESP_LOGE(SYSTEM_MANAGER_TAG, "Invalid configuration payload >> expected {%d} paramters, but receveid {%d}", (expected_delimiter_num + 1), (delimiter_num + 1));
		LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
	}
}

/**
 * @brief Handles IDP 24 requests for return configuration modification.
 *
 * This function handles the modification of return configuration.
 *
 * @param buffer The input buffer containing request data.
 * @param comm_mode The communication mode (HTTP or MQTT).
 */
static void system_manager_idp_24(const char *buffer, comm_type comm_mode)
{
	bool mqtt_load_pkg = false;
	bool mqtt_save_pkg = false;
	uint8_t delimiter_num = idp_parser_get_delimiter(buffer);
	uint8_t expected_delimiter_num = (REBOOT_CONFIG_VAR_COUNT + 1);

	if (comm_mode == COMM_MQTT || comm_mode ==  COMM_RF)
	{
		if (delimiter_num >= expected_delimiter_num)
		{
			mqtt_save_pkg = true;
		}
		else if (delimiter_num == 1 || delimiter_num == 0)
		{
			mqtt_load_pkg = true;
		}
	}

	if (comm_mode == COMM_HTTP_POST || mqtt_save_pkg)
	{
		uint8_t idp = 0;
		char pivot_id[50] = {};
		reboot_config reboot_config = {};

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", pivot_id},
				{"bool", &reboot_config.enable},
				{"uint32_t", &reboot_config.reboot_timeout_sec},
				{NULL, NULL}};

		idp_parser_get_packet_data(buffer, arg_pairs);

		esp_err_t ret = data_app_save(DATA_TYPE_REBOOT_CONFIG, &reboot_config, sizeof(reboot_config));
		if (ret == ESP_OK)
		{
			// send ACK
			comm_app_send_idp_pack(CONFIG_HTTP_OK, comm_mode);
		}
		else
		{
			comm_app_send_idp_pack(CONFIG_HTTP_ERROR, comm_mode);
		}
	}
	else if (comm_mode == COMM_HTTP_GET || mqtt_load_pkg)
	{
		char str_out[200] = {};

		uint8_t idp = IDP_24;
		reboot_config reboot_config = {};

		data_app_load(DATA_TYPE_REBOOT_CONFIG, &reboot_config);

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", system_id},
				{"bool", &reboot_config.enable},
				{"uint32_t", &reboot_config.reboot_timeout_sec},
				{NULL, NULL}};

		// send
		idp_parser_create_package(str_out, arg_pairs);
		comm_app_send_idp_pack(str_out, comm_mode);
	}
	else
	{
		ESP_LOGE(SYSTEM_MANAGER_TAG, "Invalid configuration payload >> expected {%d} paramters, but receveid {%d}", (expected_delimiter_num + 1), (delimiter_num + 1));
		LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
	}
}

/**
 * @brief Handles IDP 26 requests for return configuration modification.
 *
 * This function handles the modification of return configuration.
 *
 * @param buffer The input buffer containing request data.
 * @param comm_mode The communication mode (HTTP or MQTT).
 */
static void system_manager_idp_26(const char *buffer, comm_type comm_mode)
{
	bool mqtt_load_pkg = false;
	bool mqtt_save_pkg = false;
	uint8_t delimiter_num = idp_parser_get_delimiter(buffer);
	uint8_t expected_delimiter_num = (PIVOT_VIRTUAL_CONFIG_VAR_COUNT + 1);

	if (comm_mode == COMM_MQTT || comm_mode == COMM_RF)
	{
		if (delimiter_num >= expected_delimiter_num)
		{
			mqtt_save_pkg = true;
		}
		else if (delimiter_num == 1 || delimiter_num == 0)
		{
			mqtt_load_pkg = true;
		}
	}

	if (comm_mode == COMM_HTTP_POST || mqtt_save_pkg)
	{
		uint8_t idp = 0;
		char pivot_id[50] = {};
		pivot_virtual_config virtual_config = {};
		pivot_physical_config physical_config = {};

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", pivot_id},
				{"uint16_t", &virtual_config.start_angle_virtual_barrier},
				{"uint16_t", &virtual_config.end_angle_virtual_barrier},
				{"bool", &virtual_config.automatic_return},
				{"bool", &virtual_config.water_return},
				{NULL, NULL}};

		idp_parser_get_packet_data(buffer, arg_pairs);

		if (idp_parser_validate_idp_26(virtual_config))
		{
			esp_err_t ret = data_app_save(DATA_TYPE_VIRTUAL_BARRIER, &virtual_config, sizeof(virtual_config));
			if (ret == ESP_OK)
			{
				// send ACK
				comm_app_send_idp_pack(CONFIG_HTTP_OK, comm_mode);
				system_monitoring_stop();
				system_monitoring_start(physical_config, virtual_config, system_read_time);
			}
			else
			{
				comm_app_send_idp_pack(CONFIG_HTTP_ERROR, comm_mode);
			}
		}
		else
		{
			if (comm_mode == COMM_HTTP_POST)
			{
				comm_app_send_idp_pack(CONFIG_HTTP_ERROR, COMM_HTTP_POST);
			}

			ESP_LOGE(SYSTEM_MANAGER_TAG, "Return Config invalid packed data (%s)", buffer);
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, "Invalid data");
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
		}
	}
	else if (comm_mode == COMM_HTTP_GET || mqtt_load_pkg)
	{
		char str_out[200] = {};

		uint8_t idp = IDP_26;
		pivot_virtual_config virtual_config = {};

		data_app_load(DATA_TYPE_VIRTUAL_BARRIER, &virtual_config);

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", system_id},
				{"uint16_t", &virtual_config.start_angle_virtual_barrier},
				{"uint16_t", &virtual_config.end_angle_virtual_barrier},
				{"bool", &virtual_config.automatic_return},
				{"bool", &virtual_config.water_return},
				{NULL, NULL}};
		// send
		idp_parser_create_package(str_out, arg_pairs);
		comm_app_send_idp_pack(str_out, comm_mode);
	}
	else
	{
		ESP_LOGE(SYSTEM_MANAGER_TAG, "Invalid configuration payload >> expected {%d} paramters, but receveid {%d}", (expected_delimiter_num + 1), (delimiter_num + 1));
		LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
	}
}

/**
 * @brief Handles IDP 27 returns all schedules present on the control board
 *
 * This function works like a GET all for schedules
 *
 * @param buffer The input buffer containing request data.
 * @param comm_mode The communication mode (HTTP or MQTT).
 */
static void system_manager_idp_27(const char *buffer, comm_type comm_mode)
{
	if (comm_mode == COMM_HTTP_GET || comm_mode == COMM_MQTT)
	{
		uint16_t dwp = 0;
		uint8_t scheduling_counter = 0;

		uint8_t idp_27 = IDP_27;
		char buffer_out[1500] = "";
		char str_out[1500] = "";

		uint8_t idp_14 = IDP_14;
		char buffer_scheduling_14[100] = "";
		char str_out_scheduling_14[100] = "";

		uint8_t idp_15 = IDP_15;
		char buffer_scheduling_15[100] = "";
		char str_out_scheduling_15[100] = "";

		uint8_t idp_16 = IDP_16;
		char buffer_scheduling_16[100] = "";
		char str_out_scheduling_16[100] = "";

		uint8_t idp_17 = IDP_17;
		char buffer_scheduling_17[100] = "";
		char str_out_scheduling_17[100] = "";

		pivot_scheduling_date scheduling_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
		data_app_load(DATA_TYPE_SCHEDULING_DATE, &scheduling_date);

		pivot_scheduling_angle scheduling_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};
		data_app_load(DATA_TYPE_SCHEDULING_ANGLE, &scheduling_angle);

		pivot_scheduling_off_date scheduling_off_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
		data_app_load(DATA_TYPE_SCHEDULING_OFF_DATE, &scheduling_off_date);

		pivot_scheduling_off_angle scheduling_off_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};
		data_app_load(DATA_TYPE_SCHEDULING_OFF_ANGLE, &scheduling_off_angle);

		strncat(buffer_out, "@", sizeof(buffer_out) - strlen(buffer_out) - 1);

		for (uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
		{
			dwp = idp_parser_create_pwd(scheduling_date[position].actions);

			if (dwp != 0)
			{
				arg_pair_t arg_pairs_scheduling_14[] =
					{
						{"uint8_t", &idp_14},
						{"string", scheduling_date[position].scheduling_id},
						{"uint32_t", &scheduling_date[position].start_date},
						{"uint32_t", &scheduling_date[position].end_date},
						{"uint16_t", &dwp},
						{"uint16_t", &scheduling_date[position].actions.percentimeter},
						{NULL, NULL}};

				idp_parser_create_package(str_out_scheduling_14, arg_pairs_scheduling_14);
				if (idp_parser_remove_hashtag_cipher(str_out_scheduling_14, buffer_scheduling_14, sizeof(buffer_scheduling_14)) != true)
				{
					ESP_LOGW(SYSTEM_MANAGER_TAG, "Error: Insufficient output buffer or invalid pointers.");
				}

				strncat(buffer_out, buffer_scheduling_14, sizeof(buffer_out) - strlen(buffer_out) - 1);
				strncat(buffer_out, "@", sizeof(buffer_out) - strlen(buffer_out) - 1);

				scheduling_counter++;
			}
		}

		dwp = 0;

		for (uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
		{
			dwp = idp_parser_create_pwd(scheduling_angle[position].actions);

			if (dwp != 0)
			{
				arg_pair_t arg_pairs_scheduling_15[] =
					{
						{"uint8_t", &idp_15},
						{"string", scheduling_angle[position].scheduling_id},
						{"uint32_t", &scheduling_angle[position].start_date},
						{"uint16_t", &scheduling_angle[position].end_angle},
						{"uint16_t", &dwp},
						{"uint16_t", &scheduling_angle[position].actions.percentimeter},
						{NULL, NULL}};

				idp_parser_create_package(str_out_scheduling_15, arg_pairs_scheduling_15);
				if (idp_parser_remove_hashtag_cipher(str_out_scheduling_15, buffer_scheduling_15, sizeof(buffer_scheduling_15)) != true)
				{
					ESP_LOGW(SYSTEM_MANAGER_TAG, "Error: Insufficient output buffer or invalid pointers.");
				}

				strncat(buffer_out, buffer_scheduling_15, sizeof(buffer_out) - strlen(buffer_out) - 1);
				strncat(buffer_out, "@", sizeof(buffer_out) - strlen(buffer_out) - 1);

				scheduling_counter++;
			}
		}

		for (uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
		{
			if (scheduling_off_date[position].end_date != 0)
			{
				arg_pair_t arg_pairs_scheduling_16[] =
					{
						{"uint8_t", &idp_16},
						{"string", scheduling_off_date[position].scheduling_id},
						{"uint32_t", &scheduling_off_date[position].end_date},
						{NULL, NULL}};

				idp_parser_create_package(str_out_scheduling_16, arg_pairs_scheduling_16);
				if (idp_parser_remove_hashtag_cipher(str_out_scheduling_16, buffer_scheduling_16, sizeof(buffer_scheduling_16)) != true)
				{
					ESP_LOGW(SYSTEM_MANAGER_TAG, "Error: Insufficient output buffer or invalid pointers.");
				}

				strncat(buffer_out, buffer_scheduling_16, sizeof(buffer_out) - strlen(buffer_out) - 1);
				strncat(buffer_out, "@", sizeof(buffer_out) - strlen(buffer_out) - 1);

				scheduling_counter++;
			}
		}

		for (uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
		{
			if (strcmp(scheduling_off_angle[position].scheduling_id, "") != 0)
			{
				arg_pair_t arg_pairs_scheduling_17[] =
					{
						{"uint8_t", &idp_17},
						{"string", scheduling_off_angle[position].scheduling_id},
						{"uint16_t", &scheduling_off_angle[position].end_angle},
						{NULL, NULL}};

				idp_parser_create_package(str_out_scheduling_17, arg_pairs_scheduling_17);
				if (idp_parser_remove_hashtag_cipher(str_out_scheduling_17, buffer_scheduling_17, sizeof(buffer_scheduling_17)) != true)
				{
					ESP_LOGW(SYSTEM_MANAGER_TAG, "Error: Insufficient output buffer or invalid pointers.");
				}

				strncat(buffer_out, buffer_scheduling_17, sizeof(buffer_out) - strlen(buffer_out) - 1);
				strncat(buffer_out, "@", sizeof(buffer_out) - strlen(buffer_out) - 1);

				scheduling_counter++;
			}
		}

		arg_pair_t arg_pairs_idp_27[] =
			{
				{"uint8_t", &idp_27},
				{"string", system_id},
				{"uint8_t", &scheduling_counter},
				{"string", buffer_out},
				{NULL, NULL}};

		idp_parser_create_package(str_out, arg_pairs_idp_27);

		comm_app_send_idp_pack(str_out, COMM_MQTT);
	}
}

/**
 * @brief Handles IDP 28 requests for system actions.
 *
 * This function handles system actions based on the provided parameters.
 *
 * @param buffer The input buffer containing request data.
 * @param comm_mode The communication mode (MQTT).
 */
static void system_manager_idp_28(const char *buffer, comm_type comm_mode)
{
	char str_pkg_out[200] = {};
	data_app_load(DATA_TYPE_REASON_HANG_UP, &str_pkg_out);
	comm_app_send_idp_pack(str_pkg_out, comm_mode);
}

/**
 * @brief Handles IDP 30 requests for system actions.
 *
 * This function handles system actions based on the provided parameters.
 *
 * @param buffer The input buffer containing request data.
 * @param comm_mode The communication mode (MQTT).
 */
static void system_manager_idp_30(const char *buffer, comm_type comm_mode)
{
	esp_err_t ret = ESP_FAIL;
	pivot_actions old_actions = {};
	pivot_actions new_actions = {};
	pivot_history new_history = {};

	char str_out[200] = {};
	char str_date_time[50] = {};

	uint16_t dwp = 0;
	uint8_t idp = 0;

	arg_pair_t arg_pairs[] = {
		{"uint8_t", &idp},
		{"uint16_t", &dwp},
		{"uint16_t", &new_actions.percentimeter},
		{NULL, NULL}};

	idp_parser_get_packet_data(buffer, arg_pairs);
	idp_parser_get_pwd(dwp, &new_actions);

	data_app_load(DATA_TYPE_ACTIONS, &old_actions);

	if(new_actions.watering_state == PIVOT_DRY && old_actions.watering_state == PIVOT_WET)
	{
		new_actions.power_state = PIVOT_OFF;
		system_monitoring_pivot_shutdown(TYPE_HANGS_UP_PIVOT_WITHOUT_WATER, IDP_30, "0", SYSTEM_MANAGER_TAG);
	}

	if (new_actions.power_state == PIVOT_OFF)
	{
		if (global_angle != 655 && system_initial_angle != 655)
		{
			system_initial_angle = global_angle;
			data_app_save(DATA_TYPE_INITIAL_ANGLE, &system_initial_angle, sizeof(system_initial_angle));
		}
		else
		{
			data_app_load(DATA_TYPE_INITIAL_ANGLE, &system_initial_angle);
		}

		actuation_app_get_actions(&new_actions, sizeof(new_actions));
		new_actions.percentimeter = old_actions.percentimeter;
		new_actions.power_state = PIVOT_OFF;
		new_actions.watering_state = PIVOT_DRY;

		// Save old History and notify shutdown
		if (old_actions.power_state != PIVOT_OFF)
		{
			system_monitoring_pivot_shutdown(TYPE_HANGS_UP_MANUAL, IDP_30, "0", SYSTEM_MANAGER_TAG);
			pivot_history old_history = {};
			old_history.end_date = rtc_app_get_timestamp(false);
			old_history.end_angle = global_angle;
			data_app_save(DATA_TYPE_OLD_HISTORY, &old_history, sizeof(old_history));
		}

		counter_reading_panel_off++;
		data_app_save(DATA_TYPE_MANUAL_COUNTER, &counter_reading_panel_off, sizeof(counter_reading_panel_off));

	}

	ret = data_app_save(DATA_TYPE_ACTIONS, &new_actions, sizeof(new_actions));
	if (ret == ESP_OK)
	{
		if (comm_mode == COMM_HTTP_POST)
		{
			comm_app_send_idp_pack(CONFIG_HTTP_OK, COMM_HTTP_POST);
		}

		// act on the equipment
		actuation_app_set_actions(new_actions, true);
		system_monitoring_barrier(new_actions, PHYSICAL_BARRIER);

		// time for the percentage to stabilize
		system_rtc_percent = rtc_app_get_timestamp(false);

		// send current status
		system_manager_idp_00("#00$", comm_main_mode);

		// save new history
		if ((new_actions.power_state != PIVOT_OFF) && (old_actions.power_state == PIVOT_OFF))
		{
			counter_reading_panel_off = NO_MANUAL_READING;
			data_app_save(DATA_TYPE_MANUAL_COUNTER, &counter_reading_panel_off, sizeof(counter_reading_panel_off));

			new_history.start_date = rtc_app_get_timestamp(false);
			new_history.start_angle = global_angle;
			memcpy(&new_history.actions, &new_actions, sizeof(new_actions));
			data_app_save(DATA_TYPE_HISTORY, &new_history, sizeof(new_history));
		}
	}
	else
	{
		ESP_LOGE(SYSTEM_MANAGER_TAG, "Failed to save new state");
		LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, "Failed_to_save_new_state");
	}

	// start timer percent and pressure
	if (system_timer != NULL)
	{
		xTimerStop(system_timer, 1000);
		xTimerStart(system_timer, 1000);
	}

	// Wait modem sync
	vTaskDelay(pdMS_TO_TICKS(1500));

	// send ack
	dwp = idp_parser_create_pwd(new_actions);

	time_t timestamp = rtc_app_get_timestamp(false);
	rtc_app_get_str_date_time(timestamp, str_date_time);

	arg_pair_t arg_pairs_ack[] = {
		{"uint8_t", &idp},
		{"string", system_id},
		{"uint16_t", &dwp},
		{"uint16_t", &new_actions.percentimeter},
		{"uint16_t", &system_initial_angle},
		{"uint16_t", &global_angle},
		{"string", str_date_time},
		{NULL, NULL}};

	idp_parser_create_package(str_out, arg_pairs_ack);
	comm_app_send_idp_pack(str_out, comm_main_mode);

}

static void system_manager_idp_31(const char *buffer, comm_type comm_mode)
{
	bool mqtt_load_pkg = false;
	bool mqtt_save_pkg = false;
	uint8_t delimiter_num = idp_parser_get_delimiter(buffer);
	uint8_t expected_delimiter_num = (PIVOT_COMM_MAIN_MODE_CONFIG_VAR_COUNT + 1);

	if (comm_mode == COMM_MQTT || comm_mode == COMM_RF)
	{
		if (delimiter_num >= expected_delimiter_num)
		{
			mqtt_save_pkg = true;
		}
		else if (delimiter_num == 1 || delimiter_num == 0)
		{
			mqtt_load_pkg = true;
		}
	}

	if (comm_mode == COMM_HTTP_POST || mqtt_save_pkg)
	{
		char pivot_id[50] = {};
		pivot_comm_main_mode_config comm_config = {};
		uint8_t idp = 0;

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", pivot_id},
				{"string", comm_config.comm_main_mode_config},
				{NULL, NULL}};

		idp_parser_get_packet_data(buffer, arg_pairs);

		if (idp_parser_validate_idp_31(comm_config))
		{
			esp_err_t ret = data_app_save(DATA_TYPE_COMM_MAIN_MODE, &comm_config, sizeof(comm_config));

			if (ret == ESP_OK)
			{
				comm_app_send_idp_pack(CONFIG_HTTP_OK, comm_mode);
			}
			else
			{
				comm_app_send_idp_pack(CONFIG_HTTP_ERROR, comm_mode);
			}

		}
		else
		{
			if (comm_mode == COMM_HTTP_POST)
			{
				comm_app_send_idp_pack(CONFIG_HTTP_ERROR, COMM_HTTP_POST);
			}

			ESP_LOGE(SYSTEM_MANAGER_TAG, "Comm Main Mode Config invalid packed data (%s)", buffer);
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, "Invalid data");
			LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
		}
	}
	else if (comm_mode == COMM_HTTP_GET || mqtt_load_pkg)
	{
		char str_out[200] = {};
		pivot_comm_main_mode_config current_comm = {};
		uint8_t idp = IDP_31;

		data_app_load(DATA_TYPE_COMM_MAIN_MODE, &current_comm);

		arg_pair_t arg_pairs[] =
			{
				{"uint8_t", &idp},
				{"string", system_id},
				{"string", current_comm.comm_main_mode_config},
				{NULL, NULL}};

		idp_parser_create_package(str_out, arg_pairs);
		comm_app_send_idp_pack(str_out, comm_mode);
	}
	else
	{
		ESP_LOGE(SYSTEM_MANAGER_TAG, "Invalid configuration payload >> expected {%d} paramters, but receveid {%d}", (expected_delimiter_num + 1), (delimiter_num + 1));
		LOG_DBG_ERROR(SYSTEM_MANAGER_TAG, buffer);
	}
}

/**
 * @brief Handle IDP package type 32.
 *
 * This function handles IDP package type 32, listing all saved rainfall data with timestamps,
 * formatted as #32-system_id-@rain_total-date@rain_total-date@...$
 *
 * @param buffer The input buffer containing the request.
 * @param comm_mode The communication mode (e.g., COMM_HTTP_GET, COMM_MQTT).
 */
static void system_manager_idp_32(const char* buffer, comm_type comm_mode)
{
    if (comm_mode == COMM_HTTP_GET || comm_mode == COMM_MQTT)
    {
        char str_out[1000] = {};
        char rain_total_data[1000] = {}; 
        char entry[50] = {}; 
        uint8_t idp = IDP_32;
		rain_per_day_data* pluviometer = get_rain_data_array();
		float current_rain_value = get_rain_total(); 

        for (int i = 0; i < MAX_RAINFALL_ENTRIES; i++) 
        {
            if (pluviometer[i].rain_per_hour > 0.0f) 
            {
                snprintf(entry, sizeof(entry), "@%.2f-%s", pluviometer[i].rain_per_hour, pluviometer[i].str_date_time);
                strncat(rain_total_data, entry, sizeof(rain_total_data) - strlen(rain_total_data) - 1);
            }
        }

        arg_pair_t arg_pairs[] = {
            { "uint8_t", &idp },
            { "string", system_id },
			{"float", &current_rain_value},
            { "string", rain_total_data },
            { NULL, NULL }
        };

        idp_parser_create_package(str_out, arg_pairs);
        comm_app_send_idp_pack(str_out, comm_mode);
    }
}

/**
 * @brief Handle IDP package type 34.
 *
 * This function handles IDP package type 34, extracting or saving rain_per_pulse and rain_shutdown_value data via MQTT.
 *
 * @param buffer The input buffer containing the request.
 * @param comm_mode The communication mode (COMM_MQTT).
 */
static void system_manager_idp_34(const char *buffer, comm_type comm_mode)
{
    if (comm_mode == COMM_MQTT)
    {
        bool mqtt_load_pkg = false;
        bool mqtt_save_pkg = false;

        uint8_t delimiter_num = idp_parser_get_delimiter(buffer);
        uint8_t expected_delimiter_num = 2;

        if (delimiter_num >= expected_delimiter_num)
        {
            mqtt_save_pkg = true;
        }
        else if (delimiter_num == 1 || delimiter_num == 0)
        {
            mqtt_load_pkg = true;
        }

        if (mqtt_save_pkg)
        {
            uint8_t idp = 0;
            char pivot_id[50] = {};
            float new_rain_per_pulse = 0.0f;
			float new_rain_shutdown_value = 0.0f;

            arg_pair_t arg_pairs[] = {
                {"uint8_t", &idp},
                {"string", pivot_id},
                {"float", &new_rain_per_pulse},
				{"float", &new_rain_shutdown_value},
                {NULL, NULL}};

            idp_parser_get_packet_data(buffer, arg_pairs);

            if (new_rain_per_pulse > 0.0 && new_rain_per_pulse <= 10.0)
            {
                esp_err_t ret = data_app_save(DATA_TYPE_RAIN_PER_PULSE, &new_rain_per_pulse, sizeof(new_rain_per_pulse));
                if (ret == ESP_OK)
                {
					set_rain_per_pulse_flag(true);
                    ESP_LOGI(SYSTEM_MANAGER_TAG, "RAIN_PER_PULSE updated to %.2f", new_rain_per_pulse);
                }
                else
                {
                    ESP_LOGE(SYSTEM_MANAGER_TAG, "Failed to save RAIN_PER_PULSE to NVS.");
                }
            }
            else
            {
                ESP_LOGW(SYSTEM_MANAGER_TAG, "Invalid RAIN_PER_PULSE value: %.2f", new_rain_per_pulse);
            }

			if(new_rain_shutdown_value >= 0.0f && new_rain_shutdown_value < 200.0f)
			{
				esp_err_t ret = data_app_save(DATA_TYPE_RAIN_SHUTDOWN_VALUE, &new_rain_shutdown_value, sizeof(new_rain_shutdown_value));
				if(ret == ESP_OK)
				{
					ESP_LOGI(SYSTEM_MANAGER_TAG, "RAIN_SHUTDOWN_VALUE updated to %.2f", new_rain_shutdown_value);
				}
				else
				{
					ESP_LOGE(SYSTEM_MANAGER_TAG, "Failed to save RAIN_SHUTDOWN_VALUE to NVS.");
				}
			}
			else
            {
                ESP_LOGW(SYSTEM_MANAGER_TAG, "Invalid RAIN_SHUTDOWN_VALUE value: %.2f", new_rain_shutdown_value);
            }
        }
        else if (mqtt_load_pkg)
        {
            char str_out[200] = {};
            uint8_t idp = IDP_34;
            set_rain_per_pulse(0.1f);
			float rain_shutdown_value = 5.0f;

            if (get_rain_per_pulse() <= 0.0 || get_rain_per_pulse() > 10.0)
            {
                set_rain_per_pulse(0.1f);
            }

			float rain_per_pulse = get_rain_per_pulse();

			esp_err_t ret_shutdown = data_app_load(DATA_TYPE_RAIN_PER_PULSE, &rain_shutdown_value);
            if (ret_shutdown != ESP_OK || rain_shutdown_value <= 0.0 || rain_shutdown_value > 10.0)
            {
                rain_shutdown_value = 5.0f; 
            }

            arg_pair_t arg_pairs[] = {
                {"uint8_t", &idp},
                {"string", system_id},
                {"float", &rain_per_pulse},
				{"float", &rain_shutdown_value},
                {NULL, NULL}};

            idp_parser_create_package(str_out, arg_pairs);
            comm_app_send_idp_pack(str_out, comm_mode);
        }
    }
}

/**
 * @brief Handles IDP 90 requests for firmware version retrieval.
 *
 * This function retrieves the firmware version.
 *
 * @param buffer The input buffer containing request data.
 * @param comm_mode The communication mode (HTTP or MQTT).
 */
static void system_manager_idp_90(const char *buffer, comm_type comm_mode)
{
	if (comm_mode == COMM_HTTP_GET || comm_mode == COMM_MQTT || comm_mode == COMM_RF)
	{
		uint8_t idp = IDP_90;
		if (comm_mode == COMM_HTTP_GET)
		{
			comm_app_send_idp_pack(CONFIG_FW_VERSION, COMM_HTTP_GET);
		}
		else
		{
			char str_out[200] = {};
			arg_pair_t arg_pairs_ack[] = {
				{"uint8_t", &idp},
				{"string", system_id},
				{"string", CONFIG_FW_VERSION},
				{NULL, NULL}};

			idp_parser_create_package(str_out, arg_pairs_ack);
			comm_app_send_idp_pack(str_out, comm_mode);
		}
	}
}

/**
 * @brief Handles IDP 91 requests for system reboot.
 *
 * This function handles system reboot requests.
 *
 * @param buffer The input buffer containing request data.
 * @param comm_mode The communication mode (HTTP or MQTT).
 */
static void system_manager_idp_91(const char *buffer, comm_type comm_mode)
{
	if (comm_mode == COMM_HTTP_POST || comm_mode == COMM_MQTT || comm_mode == COMM_RF)
	{
		uint8_t idp = IDP_91;
		if (comm_mode == COMM_HTTP_POST)
		{
			comm_app_send_idp_pack(CONFIG_HTTP_OK, COMM_HTTP_POST);
		}
		else
		{
			char str_out[200] = {};
			arg_pair_t arg_pairs_ack[] = {
				{"uint8_t", &idp},
				{"string", system_id},
				{NULL, NULL}};

			idp_parser_create_package(str_out, arg_pairs_ack);
			comm_app_send_idp_pack(str_out, comm_mode);
		}

		vTaskDelay(pdMS_TO_TICKS(2000)); // 2 seconds

		esp_restart();
	}
}

/**
 * @brief Handles IDP 92 requests for system restart.
 *
 * This function handles system restart requests.
 *
 * @param buffer The input buffer containing request data.
 * @param comm_mode The communication mode (HTTP or MQTT).
 */
static void system_manager_idp_92(const char *buffer, comm_type comm_mode)
{
	char str_out[200] = {};

	if (comm_mode == COMM_HTTP_POST || comm_mode == COMM_MQTT || comm_mode == COMM_RF)
	{
		uint8_t idp = IDP_92;
		if (comm_mode == COMM_HTTP_POST)
		{
			comm_app_send_idp_pack(CONFIG_HTTP_OK, COMM_HTTP_POST);
		}

		arg_pair_t arg_pairs_ack[] = {
			{"uint8_t", &idp},
			{"string", system_id},
			{NULL, NULL}};

		idp_parser_create_package(str_out, arg_pairs_ack);
		comm_app_send_idp_pack(str_out, comm_mode);
	}
}
