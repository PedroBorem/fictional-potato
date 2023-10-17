/*
 * system_manager.c
 *
 *  Created on: 31 de mai de 2022
 *      Author: brunolima
 */

// applications include
#include "system_manager.h"
#include "project_config.h"
#include "idp_parser.h"
#include "log.h"

#include "rtc_app.h"
#include "esp_system.h"
#include "FreeRTOS_defines.h"

// private include
#include <string.h>

#include "actuation_app.h"
#include "comm_app.h"
#include "data_app.h"

#include "system_monitoring.h"
#include "sectorization.h"
#include "scheaduling.h"

#define SYSTEM_MANAGER_TAG 	"system manager"

#define SYSTEM_REBOOT_DELAY_MS	 	(120000) // 2 minutos
#define SYSTEM_REBOOT_TIMEOUT_MS	(46800000) // 3 horas
#define SYSTEM_SAVE_FLASH_TIME_MS 	(600000) // 10 minutos

/* global variables */
uint16_t global_angle = 655;

/* local variables */
static uint16_t system_initial_angle = 655;
static char system_id[50] = {};
static time_t system_rtc_percent = 0;

static void system_manager_reboot(void);
static void system_manager_callback(const char* buffer_request, comm_type comm_mode);

static void system_manager_idp_00(const char* buffer, comm_type comm_mode);
static void system_manager_idp_01(const char* buffer, comm_type comm_mode);
static void system_manager_idp_02(const char* buffer, comm_type comm_mode);
static void system_manager_idp_03(const char* buffer, comm_type comm_mode);
static void system_manager_idp_04(const char* buffer, comm_type comm_mode);
static void system_manager_idp_05(const char* buffer, comm_type comm_mode);
static void system_manager_idp_06(const char* buffer, comm_type comm_mode);
static void system_manager_idp_07(const char* buffer, comm_type comm_mode);
static void system_manager_idp_12(const char* buffer, comm_type comm_mode);
static void system_manager_idp_13(const char* buffer, comm_type comm_mode);
static void system_manager_idp_14(const char* buffer, comm_type comm_mode);
static void system_manager_idp_15(const char* buffer, comm_type comm_mode);
static void system_manager_idp_16(const char* buffer, comm_type comm_mode);
static void system_manager_idp_17(const char* buffer, comm_type comm_mode);
static void system_manager_idp_18(const char* buffer, comm_type comm_mode);
static void system_manager_idp_30(const char* buffer, comm_type comm_mode);

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

	// system monitoring init
	system_monitoring_register_callback(&system_manager_callback);
	system_monitoring_start();

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

	data_app_load(DATA_TYPE_SCHEADULING_DATE, &scheduling_date);
	data_app_load(DATA_TYPE_SCHEADULING_ANGLE, &scheduling_angle);
	data_app_load(DATA_TYPE_SCHEADULING_OFF_DATE, &scheduling_off_date);
	data_app_load(DATA_TYPE_SCHEADULING_OFF_ANGLE, &scheduling_off_angle);
	scheduling_register_callback(&system_manager_callback);

	scheduling_start(IDP_14,scheduling_date);
	scheduling_start(IDP_15,scheduling_angle);
	scheduling_start(IDP_16,scheduling_off_date);
	scheduling_start(IDP_17,scheduling_off_angle);
}

static void system_manager_reboot(void)
{
	pivot_actions current_action = {};

	time_t timestamp_nvs = 0;
	time_t timestamp_now = 0;

	data_app_load(DATA_TYPE_TIMESTAMP, &timestamp_nvs);
	timestamp_now = rtc_app_get_timestamp(false);

	if((timestamp_now - timestamp_nvs) < SYSTEM_REBOOT_TIMEOUT_MS)
	{
		esp_reset_reason_t reset_cause = esp_reset_reason();
		if(reset_cause == ESP_RST_POWERON || reset_cause == ESP_RST_BROWNOUT)
		{
			data_app_load(DATA_TYPE_ACTIONS, &current_action);

			LOG_DATA(SYSTEM_MANAGER_TAG, "");
			LOG_DATA(SYSTEM_MANAGER_TAG, " ------ NVS Current Config ------");
			LOG_DATA(SYSTEM_MANAGER_TAG, " Power state: %d", current_action.power_state);
			LOG_DATA(SYSTEM_MANAGER_TAG, " Advance mode: %d", current_action.rotation);
			LOG_DATA(SYSTEM_MANAGER_TAG, " Watering state: %d", current_action.watering_state);
			LOG_DATA(SYSTEM_MANAGER_TAG, " Percentimeter %.3d %%", current_action.percentimeter);
			LOG_DATA(SYSTEM_MANAGER_TAG, " --------------------------------\n");

			vTaskDelay(pdMS_TO_TICKS(500));

			if(current_action.power_state == PIVOT_ON)
			{
				ESP_LOGW(SYSTEM_MANAGER_TAG,"waiting for power to stabilize ...");
				vTaskDelay(pdMS_TO_TICKS(SYSTEM_REBOOT_DELAY_MS));
				actuation_app_set_actions(current_action, false);
			}
		}
	}
	else
	{
		// save old history
		data_app_save(DATA_TYPE_TIMESTAMP, &timestamp_now, sizeof(timestamp_now));
	}
}

static void system_manager_callback(const char* buffer_request, comm_type comm_mode)
{
	char str_idp[5] = {};
	char str_pkg[100] = {};

	idp_type idp_request = idp_parser_get(buffer_request, str_pkg);
    snprintf(str_idp, sizeof(str_idp), "%d", idp_request);

    LOG_COMM(SYSTEM_MANAGER_TAG, "%s", str_pkg);

	switch(idp_request)
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
		case IDP_30:
		{
			system_manager_idp_30(str_pkg, comm_mode);
			break;
		}
		case IDP_INVALID:
		{
			ESP_LOGE(SYSTEM_MANAGER_TAG, "Invalid Package (%s)", buffer_request);
			break;
		}
		default:
		{
			break;
		}
	}
}

static void system_manager_idp_00(const char* buffer, comm_type comm_mode)
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

	arg_pair_t arg_pairs[] = {
		{ "uint8_t", &idp },
		{ "string", system_id },
		{ "uint16_t", &dwp },
		{ "uint16_t", &actions.percentimeter },
		{ "uint16_t", &system_initial_angle },
		{ "uint16_t", &global_angle },
		{ "string", str_date_time },
		{ NULL, NULL }
	};

	if((timestamp - system_rtc_percent) < 65)
	{
		actions.percentimeter = CONFIG_ACTIONS_UNDEF_VALUE;
	}

	idp_parser_create_package(str_out,arg_pairs);

	if(comm_mode == COMM_HTTP_POST)
	{
		comm_app_send_idp_pack(str_out, COMM_HTTP_POST);
		comm_app_send_idp_pack(str_out, COMM_MQTT);
	}
	else
	{
		comm_app_send_idp_pack(str_out, comm_mode);
	}
}

static void system_manager_idp_01(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_POST || comm_mode == COMM_MQTT)
	{
		esp_err_t ret = ESP_FAIL;
		pivot_actions new_actions = {};
		pivot_history new_history = {};

		char pivot_id[50] = {};
		uint16_t dwp = 0;
		uint8_t idp = 0;

		arg_pair_t arg_pairs[] =
		{
			{ "uint8_t", &idp },
			{ "string", pivot_id },
			{ "uint16_t", &dwp },
			{ "uint16_t", &new_actions.percentimeter },
			{ NULL, NULL }
		};

		idp_parser_get_packet_data(buffer, arg_pairs);
		idp_parser_get_pwd(dwp, &new_actions);

		if(idp_parser_validate_actions(new_actions) == true)
		{
			// actions = 002
			if(new_actions.rotation == 0 && new_actions.watering_state == 0)
			{
				actuation_app_get_actions(&new_actions, sizeof(new_actions));
				new_actions.percentimeter = 0;
				new_actions.power_state = PIVOT_OFF;
				new_actions.watering_state = PIVOT_DRY;
			}

			ret = data_app_save(DATA_TYPE_ACTIONS, &new_actions, sizeof(new_actions));
			if(ret == ESP_OK)
			{
				// save old history
				pivot_history old_history = {};
				old_history.end_date = rtc_app_get_timestamp(false);
				old_history.end_angle = global_angle;

				data_app_save(DATA_TYPE_OLD_HISTORY, &old_history, sizeof(old_history));

				// act on the equipment
				actuation_app_set_actions(new_actions, false);

				// time for the percentage to stabilize
				system_rtc_percent = rtc_app_get_timestamp(false);

				// send current status
				system_manager_idp_00("#00$", comm_mode);

				// save new history
				if(new_actions.power_state != PIVOT_OFF)
				{
					new_history.start_date = rtc_app_get_timestamp(false);
					new_history.start_angle = global_angle;
					memcpy(&new_history.actions, &new_actions, sizeof(new_actions));
					data_app_save(DATA_TYPE_HISTORY, &new_history, sizeof(new_history));
				}
			}
			else
			{
				ESP_LOGE(SYSTEM_MANAGER_TAG, "Failed to save new state");
			}
		}
		else
		{
			ESP_LOGE(SYSTEM_MANAGER_TAG, "Invalid state (%s)", buffer);
		}
	}
}

static void system_manager_idp_02(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_POST || comm_mode == COMM_MQTT)
	{
		char str_out[200] = {};
		network_config net_config = {};
		uint8_t idp = 0;

		arg_pair_t arg_pairs[] =
		{
			{ "uint8_t", &idp },
			{ "string", net_config.gprs_id },
			{ "string", net_config.modem_apn },
			{ "string", net_config.wifi_ssid },
			{ "string", net_config.wifi_pass },
			{ NULL, NULL }
		};

		idp_parser_get_packet_data(buffer, arg_pairs);

		data_app_save(DATA_TYPE_NETWORK_CONFIG, &net_config, sizeof(net_config));
		comm_app_wifi_config(net_config.wifi_ssid, net_config.wifi_pass);

		strcpy(system_id, net_config.gprs_id);

		// send ACK
		arg_pair_t arg_pairs_2[] = {
			{ "uint8_t", &idp },
			{ "string", system_id },
			{ NULL, NULL }
		};
		idp_parser_create_package(str_out, arg_pairs_2);
		comm_app_send_idp_pack(str_out, COMM_HTTP_POST);

		// send GPRS module
		idp = IDP_6;
		arg_pair_t arg_pairs_3[] = {
			{ "uint8_t", &idp },
			{ "string", net_config.gprs_id },
			{ "string", net_config.modem_apn },
			{ NULL, NULL }
		};

		idp_parser_create_package(str_out, arg_pairs_3);
		comm_app_send_idp_pack(str_out, COMM_MQTT);
	}
	else if(comm_mode == COMM_HTTP_GET)
	{
		char str_out[200] = {};
		network_config net_config = {};
		uint8_t idp = IDP_2;

		data_app_load(DATA_TYPE_NETWORK_CONFIG, &net_config);

		arg_pair_t arg_pairs[] =
		{
			{ "uint8_t", &idp },
			{ "string", net_config.gprs_id },
			{ "string", net_config.modem_apn },
			{ "string", net_config.wifi_ssid },
			{ "string", net_config.wifi_pass },
			{ NULL, NULL }
		};

		idp_parser_create_package(str_out,arg_pairs);
		comm_app_send_idp_pack(str_out, COMM_HTTP_GET);
	}
}

static void system_manager_idp_03(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_POST || comm_mode == COMM_MQTT)
	{
		char str_out[200] = {};

		uint8_t idp = 0;
		pivot_config new_config = {};

		arg_pair_t arg_pairs[] =
		{
			{ "uint8_t", &idp },
			{ "string", new_config.contactor },
			{ "string", new_config.pressure },
			{ "uint16_t", &new_config.pressurization_time },
			{ "uint8_t", &new_config.on_off_time },
			{ NULL, NULL }
		};

		idp_parser_get_packet_data(buffer, arg_pairs);

		esp_err_t ret = data_app_save(DATA_TYPE_PIVOT_CONFIG, &new_config, sizeof(new_config));
		if(ret == ESP_OK)
		{
			// send ACK
			arg_pair_t arg_pairs_2[] = {
				{ "uint8_t", &idp },
				{ "string", system_id },
				{ NULL, NULL }
			};

			actuation_app_set_config(new_config);
			idp_parser_create_package(str_out, arg_pairs_2);
			comm_app_send_idp_pack(str_out, COMM_HTTP_POST);
		}
	}
	else if(comm_mode == COMM_HTTP_GET)
	{
		char str_out[200] = {};

		uint8_t idp = IDP_3;
		pivot_config config = {};

		data_app_load(DATA_TYPE_PIVOT_CONFIG, &config);

		arg_pair_t arg_pairs[] =
		{
			{ "uint8_t", &idp },
			{ "string", config.contactor },
			{ "string", config.pressure },
			{ "uint16_t", &config.pressurization_time },
			{ "uint8_t", &config.on_off_time },
			{ NULL, NULL }
		};

		// send
		idp_parser_create_package(str_out, arg_pairs);
		comm_app_send_idp_pack(str_out, COMM_HTTP_GET);
	}
}

static void system_manager_idp_04(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_POST || comm_mode == COMM_MQTT)
	{
		char str_out[200] = {};

		uint8_t idp = 0;
		eco_mode_config eco_mode = {};

		arg_pair_t arg_pairs[] =
		{
			{ "uint8_t", &idp },
			{ "uint32_t", &eco_mode.start_time }, //todo isso deve ser timestamp?
			{ "uint32_t", &eco_mode.end_time },
			{ NULL, NULL }
		};

		idp_parser_get_packet_data(buffer, arg_pairs);
		data_app_save(DATA_TYPE_ECO_MODE_CONFIG, &eco_mode, sizeof(eco_mode));

		// todo se eco_mode.start_time  e end forem 0 nao considero
		// todo aplicar na task

		// send ACK
		arg_pair_t arg_pairs_2[] = {
			{ "uint8_t", &idp },
			{ NULL, NULL }
		};

		idp_parser_create_package(str_out, arg_pairs_2);
		comm_app_send_idp_pack(str_out, COMM_HTTP_POST);
	}
	else if(comm_mode == COMM_HTTP_GET)
	{
		char str_out[200] = {};

		uint8_t idp = IDP_4;
		eco_mode_config eco_mode = {};

		data_app_load(DATA_TYPE_ECO_MODE_CONFIG, &eco_mode);

		arg_pair_t arg_pairs[] =
		{
			{ "uint8_t", &idp },
			{ "uint32_t", &eco_mode.start_time },
			{ "uint32_t", &eco_mode.end_time },
			{ NULL, NULL }
		};

		idp_parser_create_package(str_out, arg_pairs);
		comm_app_send_idp_pack(str_out, COMM_HTTP_GET);
	}
}

static void system_manager_idp_05(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_POST || comm_mode == COMM_MQTT)
	{
		char str_out[200] = {};

		uint8_t idp = 0;
		sector_config sector = {};

		arg_pair_t arg_pairs[] =
		{
			{ "uint8_t", &idp },
			{ "uint8_t", &sector.sector_number },
			{ "uint16_t", &sector.sectors[0].start_angle },
			{ "uint16_t", &sector.sectors[0].end_angle },
			{ "uint16_t", &sector.sectors[1].start_angle },
			{ "uint16_t", &sector.sectors[1].end_angle },
			{ "uint16_t", &sector.sectors[2].start_angle },
			{ "uint16_t", &sector.sectors[2].end_angle },
			{ "uint16_t", &sector.sectors[3].start_angle },
			{ "uint16_t", &sector.sectors[3].end_angle },
			{ NULL, NULL }
		};

		idp_parser_get_packet_data(buffer, arg_pairs);
		data_app_save(DATA_TYPE_SECTOR_CONFIG, &sector, sizeof(sector));

		sectorization_stop();
		sectorization_start(sector);

		// send ACK
		arg_pair_t arg_pairs_2[] = {
			{ "uint8_t", &idp },
			{ NULL, NULL }
		};
		idp_parser_create_package(str_out, arg_pairs_2);
		comm_app_send_idp_pack(str_out, COMM_HTTP_POST);
	}
	else if(comm_mode == COMM_HTTP_GET)
	{
		char str_out[200] = {};

		uint8_t idp = IDP_5;
		sector_config sector = {};

		data_app_load(DATA_TYPE_SECTOR_CONFIG, &sector);

		arg_pair_t arg_pairs[] =
		{
			{ "uint8_t", &idp },
			{ "uint8_t", &sector.sector_number },
			{ "uint16_t", &sector.sectors[0].start_angle },
			{ "uint16_t", &sector.sectors[0].end_angle },
			{ "uint16_t", &sector.sectors[1].start_angle },
			{ "uint16_t", &sector.sectors[1].end_angle },
			{ "uint16_t", &sector.sectors[2].start_angle },
			{ "uint16_t", &sector.sectors[2].end_angle },
			{ "uint16_t", &sector.sectors[3].start_angle },
			{ "uint16_t", &sector.sectors[3].end_angle },
			{ NULL, NULL }
		};

		idp_parser_create_package(str_out, arg_pairs);
		comm_app_send_idp_pack(str_out, COMM_HTTP_GET);
	}
}

static void system_manager_idp_06(const char* buffer, comm_type comm_mode)
{
	char str_out[200] = {};
	network_config net_config = {};
	uint8_t idp = 0;

	data_app_load(DATA_TYPE_NETWORK_CONFIG, &net_config);

	// send GPRS module
	idp = IDP_6;
	arg_pair_t arg_pairs_3[] = {
		{ "uint8_t", &idp },
		{ "string", net_config.gprs_id },
		{ "string", net_config.modem_apn },
		{ NULL, NULL }
	};

	idp_parser_create_package(str_out, arg_pairs_3);
	comm_app_send_idp_pack(str_out, COMM_MQTT);
}

static void system_manager_idp_07(const char* buffer, comm_type comm_mode)
{
	uint8_t idp = 0;
	uint32_t timestamp;
	char utc[10] = {};
	// todo transformar em time_t (adicionar time_t no idp_parser_get_packet_data)
	// get angle
	arg_pair_t arg_pairs[] =
	{
		{ "uint8_t", &idp },
		{ "uint16_t", &global_angle },
		{ "uint32_t", &timestamp },
		{ "string", utc },
		{ NULL, NULL }
	};

	idp_parser_get_packet_data(buffer, arg_pairs);
	rtc_app_set_timestamp((time_t)timestamp);

	if(system_initial_angle == 655)
	{
		system_initial_angle = global_angle;
		ESP_LOGW(SYSTEM_MANAGER_TAG, "Initial angle : %d", system_initial_angle);
	}
}

static void system_manager_idp_12(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_GET)
	{
		char buffer_out[600] = {};
		char str_out[200] = {};

		uint16_t dwp = 0;
		uint8_t idp = IDP_12;

		pivot_history load_history[CONFIG_HISTORY_MAX_VALUE] = {};
		data_app_load(DATA_TYPE_HISTORY, &load_history);

		for(uint8_t position = 0; position < CONFIG_HISTORY_MAX_VALUE; position++)
		{
			//if(load_history[position].end_date != 0)
			//{
			dwp = idp_parser_create_pwd(load_history[position].actions);

			arg_pair_t arg_pairs[] =
			{
				{ "uint8_t", &idp },
				{ "string", system_id },
				{ "uint16_t", &load_history[position].start_angle },
				{ "uint16_t", &load_history[position].end_angle },
				{ "uint32_t", &load_history[position].start_date },
				{ "uint32_t", &load_history[position].end_date },
				{ "uint16_t", &dwp },
				{ "uint16_t", &load_history[position].actions.percentimeter },
				{ NULL, NULL }
			};

			idp_parser_create_package(str_out, arg_pairs);

			strcat(buffer_out, str_out);
			strcat(buffer_out, "\n");
			//}
		}

		comm_app_send_idp_pack(buffer_out, COMM_HTTP_GET);
	}
}

static void system_manager_idp_13(const char* buffer, comm_type comm_mode)
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
	char scheaduling_id[20] = {};

	arg_pair_t arg_pairs[] =
	{
		{ "uint8_t", &idp },
		{ "string", pivot_id },
		{ "string", scheaduling_id },
		{ "string", str_author },
		{ NULL, NULL }
	};

	idp_parser_get_packet_data(buffer, arg_pairs);

	data_app_delete(scheaduling_id);

	data_app_load(DATA_TYPE_SCHEADULING_DATE, &scheduling_date);
	data_app_load(DATA_TYPE_SCHEADULING_ANGLE, &scheduling_angle);
	data_app_load(DATA_TYPE_SCHEADULING_OFF_DATE, &scheduling_off_date);
	data_app_load(DATA_TYPE_SCHEADULING_OFF_ANGLE, &scheduling_off_angle);

	scheduling_stop(IDP_14);
	scheduling_stop(IDP_15);
	scheduling_stop(IDP_16);
	scheduling_stop(IDP_17);

	scheduling_start(IDP_14,scheduling_date);
	scheduling_start(IDP_15,scheduling_angle);
	scheduling_start(IDP_16,scheduling_off_date);
	scheduling_start(IDP_17,scheduling_off_angle);

	// send ack
	arg_pair_t arg_pairs_2[] =
	{
		{ "uint8_t", &idp },
		{ "string", system_id },
		{ "string", scheaduling_id },
		{ NULL, NULL }
	};

	idp_parser_create_package(str_out, arg_pairs_2);

	comm_app_send_idp_pack(str_out, COMM_HTTP_POST);
	comm_app_send_idp_pack(str_out, COMM_MQTT);
}

static void system_manager_idp_14(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_POST || comm_mode == COMM_MQTT)
	{
		char str_out[200] = {};
		char pivot_id[50] = {};
		char str_author[30] = {};

		pivot_scheduling_date scheduling = {};
		uint16_t dwp = 0;
		uint8_t idp = 0;

		// todo alterar os arg_pairs para nomes diferentes
		arg_pair_t arg_pairs[] =
		{
			{ "uint8_t", &idp },
			{ "string", pivot_id},
			{ "uint32_t", &scheduling.start_date },
			{ "uint32_t", &scheduling.end_date },
			{ "uint16_t", &dwp },
			{ "uint16_t", &scheduling.actions.percentimeter },
			{ "string", str_author },
			{ NULL, NULL }
		};

		idp_parser_get_packet_data(buffer, arg_pairs);
		idp_parser_get_pwd(dwp, &scheduling.actions);

		pivot_scheduling_date scheduling_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
		data_app_load(DATA_TYPE_SCHEADULING_DATE, &scheduling_date);

		for(uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
		{
			if(strcmp(scheduling_date[position].scheduling_id, "") == 0)
			{
				memcpy(&scheduling_date[position], &scheduling, sizeof(scheduling_date[position]));

				if(idp_parser_validate_actions(scheduling.actions) == true)
				{
					// get_rtc
					scheduling_date[position].start_date += rtc_app_get_timestamp(false);
					scheduling_date[position].end_date += rtc_app_get_timestamp(false);

					// gen Key
					data_app_gen_scheduling_key((char*)&scheduling_date[position].scheduling_id);
					strcpy(scheduling.scheduling_id, (char*)&scheduling_date[position].scheduling_id);

					data_app_save(DATA_TYPE_SCHEADULING_DATE, &scheduling_date, sizeof(scheduling_date));

					scheduling_stop(idp);
					scheduling_start(idp, scheduling_date);

					ESP_LOGI(SYSTEM_MANAGER_TAG, "Save schedule date id : %s", scheduling_date[position].scheduling_id);

					// send ack
					if(comm_mode == COMM_HTTP_POST)
					{
						arg_pair_t arg_pairs_2[] =
						{
							{ "uint8_t", &idp },
							{ "string", system_id },
							{ "string", scheduling.scheduling_id },
							{ "uint32_t", &scheduling.start_date },
							{ "uint32_t", &scheduling.end_date },
							{ "uint16_t", &dwp },
							{ "uint16_t", &scheduling.actions.percentimeter },
							{ NULL, NULL }
						};

						idp_parser_create_package(str_out, arg_pairs_2);

						comm_app_send_idp_pack(str_out, COMM_HTTP_POST);
						comm_app_send_idp_pack(str_out, COMM_MQTT);
					}
					else if(comm_mode == COMM_MQTT)
					{
						arg_pair_t arg_pairs_2[] =
						{
							{ "uint8_t", &idp },
							{ "string", system_id },
							{ "string", scheduling.scheduling_id },
							{ NULL, NULL }
						};

						idp_parser_create_package(str_out, arg_pairs_2);
						comm_app_send_idp_pack(str_out, COMM_MQTT);
					}
				}
				else
				{
					ESP_LOGE(SYSTEM_MANAGER_TAG, "Scheduling invalid state (%s)", buffer);
				}

				break;
			}
		}
	}
	else if(comm_mode == COMM_HTTP_GET)
	{
		char buffer_out[500] = {};
		char str_out[200] = {};

		uint16_t dwp = 0;
		uint8_t idp = IDP_14;
		uint8_t scheduling_size = 0;

		pivot_scheduling_date scheduling_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
		data_app_load(DATA_TYPE_SCHEADULING_DATE, &scheduling_date);

		for(uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
		{
			if(strcmp(scheduling_date[position].scheduling_id, "") == 0)
			{
				scheduling_size = position;
				break;
			}
		}

		if(scheduling_size != 0)
		{
			for(uint8_t position = 0; position < scheduling_size; position++)
			{
				dwp = idp_parser_create_pwd(scheduling_date[position].actions);

				arg_pair_t arg_pairs[] =
				{
					{ "uint8_t", &idp },
					{ "string", system_id },
					{ "string", scheduling_date[position].scheduling_id },
					{ "uint32_t", &scheduling_date[position].start_date },
					{ "uint32_t", &scheduling_date[position].end_date },
					{ "uint16_t", &dwp },
					{ "uint16_t", &scheduling_date[position].actions.percentimeter },
					{ NULL, NULL }
				};

				idp_parser_create_package(str_out, arg_pairs);

				strcat(buffer_out, str_out);
				strcat(buffer_out, "\n");
			}
		}

		comm_app_send_idp_pack(buffer_out, COMM_HTTP_GET);
	}
}

static void system_manager_idp_15(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_POST || comm_mode == COMM_MQTT)
	{
		char str_out[200] = {};
		char pivot_id[50] = {};
		char str_author[30] = {};

		pivot_scheduling_angle scheduling = {};
		uint16_t dwp = 0;
		uint8_t idp = 0;

		arg_pair_t arg_pairs[] =
		{
			{ "uint8_t", &idp },
			{ "string", pivot_id },
			{ "uint32_t", &scheduling.start_date },
			{ "uint16_t", &scheduling.end_angle },
			{ "uint16_t", &dwp },
			{ "uint16_t", &scheduling.actions.percentimeter },
			{ "string", str_author },
			{ NULL, NULL }
		};

		idp_parser_get_packet_data(buffer, arg_pairs);
		idp_parser_get_pwd(dwp, &scheduling.actions);

		pivot_scheduling_angle scheduling_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};
		data_app_load(DATA_TYPE_SCHEADULING_ANGLE, &scheduling_angle);

		for(uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
		{
			if(strcmp(scheduling_angle[position].scheduling_id, "") == 0)
			{
				memcpy(&scheduling_angle[position], &scheduling, sizeof(scheduling_angle[position]));

				if(idp_parser_validate_actions(scheduling.actions) == true)
				{
					// get_rtc
					scheduling_angle[position].start_date += rtc_app_get_timestamp(false);

					// gen key
					data_app_gen_scheduling_key((char*)&scheduling_angle[position].scheduling_id);
					data_app_save(DATA_TYPE_SCHEADULING_ANGLE, &scheduling_angle, sizeof(scheduling_angle));

					strcpy((char*)&scheduling.scheduling_id, (char*)&scheduling_angle[position].scheduling_id);
					scheduling_stop(idp);
					scheduling_start(idp, scheduling_angle);

					ESP_LOGI(SYSTEM_MANAGER_TAG, "Save schedule angle id : %s", scheduling_angle[position].scheduling_id);

					// send ack
					if(comm_mode == COMM_HTTP_POST)
					{
						arg_pair_t arg_pairs_2[] =
						{
							{ "uint8_t", &idp },
							{ "string", system_id },
							{ "string", scheduling.scheduling_id },
							{ "uint32_t", &scheduling.start_date },
							{ "uint16_t", &scheduling.end_angle },
							{ "uint16_t", &dwp },
							{ "uint16_t", &scheduling.actions.percentimeter },
							{ NULL, NULL }
						};

						idp_parser_create_package(str_out, arg_pairs_2);

						comm_app_send_idp_pack(str_out, COMM_HTTP_POST);
						comm_app_send_idp_pack(str_out, COMM_MQTT);
					}
					else if(comm_mode == COMM_MQTT)
					{
						arg_pair_t arg_pairs_2[] =
						{
							{ "uint8_t", &idp },
							{ "string", system_id },
							{ "string", scheduling.scheduling_id },
							{ NULL, NULL }
						};

						idp_parser_create_package(str_out, arg_pairs_2);
						comm_app_send_idp_pack(str_out, COMM_MQTT);
					}
				}
				else
				{
					ESP_LOGE(SYSTEM_MANAGER_TAG, "Scheduling invalid state (%s)", buffer);
				}

				break;
			}
		}
	}
	else if(comm_mode == COMM_HTTP_GET)
	{
		char buffer_out[500] = {};
		char str_out[200] = {};

		uint16_t dwp = 0;
		uint8_t idp = IDP_15;
		uint8_t scheduling_size = 0;

		pivot_scheduling_angle scheduling_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};
		data_app_load(DATA_TYPE_SCHEADULING_ANGLE, &scheduling_angle);

		for(uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
		{
			if(strcmp(scheduling_angle[position].scheduling_id, "") == 0)
			{
				scheduling_size = position;
				break;
			}
		}

		if(scheduling_size != 0)
		{
			for(uint8_t position = 0; position < scheduling_size; position++)
			{
				dwp = idp_parser_create_pwd(scheduling_angle[position].actions);

				arg_pair_t arg_pairs[] =
				{
					{ "uint8_t", &idp },
					{ "string", system_id },
					{ "string", scheduling_angle[position].scheduling_id },
					{ "uint32_t", &scheduling_angle[position].start_date },
					{ "uint16_t", &scheduling_angle[position].end_angle },
					{ "uint16_t", &dwp },
					{ "uint16_t", &scheduling_angle[position].actions.percentimeter },
					{ NULL, NULL }
				};

				idp_parser_create_package(str_out, arg_pairs);

				strcat(buffer_out, str_out);
				strcat(buffer_out, "\n");
			}
		}

		comm_app_send_idp_pack(buffer_out, COMM_HTTP_GET);
	}
}

static void system_manager_idp_16(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_POST || comm_mode == COMM_MQTT)
	{
		char str_out[200] = {};
		char str_author[30] = {};
		char pivot_id[50] = {};

		pivot_scheduling_off_date scheduling = {};
		uint8_t idp = 0;

		arg_pair_t arg_pairs[] =
		{
			{ "uint8_t", &idp },
			{ "string", pivot_id },
			{ "uint32_t", &scheduling.end_date },
			{ "string", str_author },
			{ NULL, NULL }
		};

		idp_parser_get_packet_data(buffer, arg_pairs);

		pivot_scheduling_off_date scheduling_off_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
		data_app_load(DATA_TYPE_SCHEADULING_OFF_DATE, &scheduling_off_date);

		for(uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
		{
			if(strcmp(scheduling_off_date[position].scheduling_id, "") == 0)
			{
				memcpy(&scheduling_off_date[position], &scheduling, sizeof(scheduling_off_date[position]));

				// get_rtc
				scheduling_off_date[position].end_date += rtc_app_get_timestamp(false);

				data_app_gen_scheduling_key((char*)&scheduling_off_date[position].scheduling_id);
				data_app_save(DATA_TYPE_SCHEADULING_OFF_DATE, &scheduling_off_date, sizeof(scheduling_off_date));

				scheduling_stop(idp);
				scheduling_start(idp, scheduling_off_date);

				ESP_LOGI(SYSTEM_MANAGER_TAG, "Save schedule date id : %s", scheduling_off_date[position].scheduling_id);

				// send ack
				if(comm_mode == COMM_HTTP_POST)
				{
					arg_pair_t arg_pairs_2[] =
					{
						{ "uint8_t", &idp },
						{ "string", system_id },
						{ "string", scheduling_off_date[position].scheduling_id },
						{ "uint32_t", &scheduling.end_date },
						{ NULL, NULL }
					};

					idp_parser_create_package(str_out, arg_pairs_2);

					comm_app_send_idp_pack(str_out, COMM_HTTP_POST);
					comm_app_send_idp_pack(str_out, COMM_MQTT);
				}
				else if(comm_mode == COMM_MQTT)
				{
					arg_pair_t arg_pairs_2[] =
					{
						{ "uint8_t", &idp },
						{ "string", system_id },
						{ "string", scheduling_off_date[position].scheduling_id },
						{ NULL, NULL }
					};

					idp_parser_create_package(str_out, arg_pairs_2);
					comm_app_send_idp_pack(str_out, COMM_MQTT);
				}
				break;
			}
		}
	}
	else if(comm_mode == COMM_HTTP_GET)
	{
		char buffer_out[500] = {};
		char str_out[200] = {};

		uint8_t idp = IDP_16;
		uint8_t scheduling_size = 0;

		pivot_scheduling_off_date scheduling_off_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
		data_app_load(DATA_TYPE_SCHEADULING_OFF_DATE, &scheduling_off_date);

		for(uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
		{
			if(strcmp(scheduling_off_date[position].scheduling_id, "") == 0)
			{
				scheduling_size = position;
				break;
			}
		}

		if(scheduling_size != 0)
		{
			for(uint8_t position = 0; position < scheduling_size; position++)
			{
				if(scheduling_off_date[position].end_date != 0)
				{
					arg_pair_t arg_pairs[] =
					{
						{ "uint8_t", &idp },
						{ "string", system_id },
						{ "string", scheduling_off_date[position].scheduling_id },
						{ "uint32_t", &scheduling_off_date[position].end_date },
						{ NULL, NULL }
					};

					idp_parser_create_package(str_out, arg_pairs);

					strcat(buffer_out, str_out);
					strcat(buffer_out, "\n");
				}
			}
		}

		comm_app_send_idp_pack(buffer_out, COMM_HTTP_GET);
	}
}

static void system_manager_idp_17(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_POST || comm_mode == COMM_MQTT)
	{
		char str_out[200] = {};
		char str_author[30] = {};
		char pivot_id[50] = {};

		pivot_scheduling_off_angle scheduling_off_angle = {};
		uint8_t idp = 0;

		arg_pair_t arg_pairs[] =
		{
			{ "uint8_t", &idp },
			{ "string", pivot_id },
			{ "uint16_t", &scheduling_off_angle.end_angle },
			{ "string", str_author },
			{ NULL, NULL }
		};

		idp_parser_get_packet_data(buffer, arg_pairs);

		data_app_gen_scheduling_key(scheduling_off_angle.scheduling_id);
		data_app_save(DATA_TYPE_SCHEADULING_OFF_ANGLE, &scheduling_off_angle, sizeof(scheduling_off_angle));

		scheduling_stop(idp);
		scheduling_start(idp, &scheduling_off_angle);

		ESP_LOGI(SYSTEM_MANAGER_TAG, "Save schedule date id : %s", scheduling_off_angle.scheduling_id);

		// send ack
		if(comm_mode == COMM_HTTP_POST)
		{
			arg_pair_t arg_pairs_2[] =
			{
				{ "uint8_t", &idp },
				{ "string", system_id },
				{ "string", scheduling_off_angle.scheduling_id },
				{ "uint16_t", &scheduling_off_angle.end_angle },
				{ NULL, NULL }
			};

			idp_parser_create_package(str_out, arg_pairs_2);

			comm_app_send_idp_pack(str_out, COMM_HTTP_POST);
			comm_app_send_idp_pack(str_out, COMM_MQTT);
		}
		else if(comm_mode == COMM_MQTT)
		{
			arg_pair_t arg_pairs_2[] =
			{
				{ "uint8_t", &idp },
				{ "string", system_id },
				{ "string", scheduling_off_angle.scheduling_id },
				{ NULL, NULL }
			};

			idp_parser_create_package(str_out, arg_pairs_2);
			comm_app_send_idp_pack(str_out, COMM_MQTT);
		}
	}
	else if(comm_mode == COMM_HTTP_GET)
	{
		char buffer_out[500] = {};
		char str_out[200] = {};

		uint8_t idp = IDP_16;
		pivot_scheduling_off_angle scheduling_off_angle = {};

		data_app_load(DATA_TYPE_SCHEADULING_OFF_ANGLE, &scheduling_off_angle);
		arg_pair_t arg_pairs[] =
		{
			{ "uint8_t", &idp },
			{ "string", system_id },
			{ "string",scheduling_off_angle.scheduling_id },
			{ "uint16_t", &scheduling_off_angle.end_angle },
			{ NULL, NULL }
		};

		idp_parser_create_package(str_out, arg_pairs);

		strcat(buffer_out, str_out);
		strcat(buffer_out, "\n");

		comm_app_send_idp_pack(buffer_out, COMM_HTTP_GET);
	}

	return;
}

static void system_manager_idp_18(const char* buffer, comm_type comm_mode)
{
	char str_out[200] = {};
	char pivot_id[50] = {};
	char scheaduling_id[10] = {};
	uint8_t idp = 0;

	arg_pair_t arg_get[] =
	{
		{ "uint8_t", &idp },
		{ "string", pivot_id },
		{ "string", scheaduling_id},
		{ NULL, NULL }
	};

	idp_parser_get_packet_data(buffer, arg_get);

	// create package - send IDP 18
	arg_pair_t arg_send[] = {
		{ "uint8_t", &idp },
		{ "string", system_id },
		{ "string", scheaduling_id},
		{ NULL, NULL }
	};

	idp_parser_create_package(str_out, arg_send);
	comm_app_send_idp_pack(str_out, COMM_MQTT);
}

static void system_manager_idp_30(const char* buffer, comm_type comm_mode)
{
	uint8_t idp = 0;
	uint16_t dwp = 0;
	pivot_actions actions = {};

	char str_out[200] = {};
	char str_date_time[50] = {};
	char pivot_id[50] = {};

	arg_pair_t arg_buffer[] =
	{
		{ "uint8_t", &idp },
		{ "string", pivot_id },
		{ NULL, NULL }
	};
	idp_parser_get_packet_data(buffer, arg_buffer);

	if(strcmp(pivot_id, "off") == 0)
	{
		pivot_actions current_action = {
				.power_state = PIVOT_OFF,
				.rotation = PIVOT_UNKNOWN,
				.watering_state = PIVOT_UNKNOWN,
				.percentimeter = PIVOT_UNKNOWN,
		};

		// act on the equipment
		actuation_app_set_actions(current_action, true);
		actuation_app_shutdown();

		// save current config
		data_app_save(DATA_TYPE_ACTIONS, &current_action, sizeof(current_action));

		// save old history
		pivot_history current_history = {};
		current_history.end_date = rtc_app_get_timestamp(false);
		current_history.end_angle = global_angle;

		data_app_save(DATA_TYPE_OLD_HISTORY, &current_history, sizeof(current_history));
	}
	else
	{
		esp_err_t ret = ESP_FAIL;
		pivot_actions new_actions = {};
		pivot_history new_history = {};

		uint16_t dwp = 0;

		arg_pair_t arg_pairs[] =
		{
			{ "uint8_t", &idp },
			{ "uint16_t", &dwp },
			{ "uint16_t", &new_actions.percentimeter },
			{ NULL, NULL }
		};

		idp_parser_get_packet_data(buffer, arg_pairs);
		idp_parser_get_pwd(dwp, &new_actions);

		ret = data_app_save(DATA_TYPE_ACTIONS, &new_actions, sizeof(new_actions));
		if(ret == ESP_OK)
		{
			// save old history
			pivot_history old_history = {};
			old_history.end_date = rtc_app_get_timestamp(false);
			old_history.end_angle = global_angle;

			data_app_save(DATA_TYPE_OLD_HISTORY, &old_history, sizeof(old_history));

			// act on the equipment
			actuation_app_set_actions(new_actions, true);

			// save new history
			if(new_actions.power_state != PIVOT_OFF)
			{
				new_history.start_date = rtc_app_get_timestamp(false);
				new_history.start_angle = global_angle;
				memcpy(&new_history.actions, &new_actions, sizeof(new_actions));
				data_app_save(DATA_TYPE_HISTORY, &new_history, sizeof(new_history));
			}
		}
	}

	// send ack
	actuation_app_get_actions(&actions, sizeof(actions));
	dwp = idp_parser_create_pwd(actions);

	time_t timestamp = rtc_app_get_timestamp(false);
	rtc_app_get_str_date_time(timestamp, str_date_time);

	arg_pair_t arg_pairs_ack[] = {
		{ "uint8_t", &idp },
		{ "string", system_id },
		{ "uint16_t", &dwp },
		{ "uint16_t", &actions.percentimeter },
		{ "uint16_t", &system_initial_angle },
		{ "uint16_t", &global_angle },
		{ "string", str_date_time },
		{ NULL, NULL }
	};

	idp_parser_create_package(str_out, arg_pairs_ack);
	comm_app_send_idp_pack(str_out, COMM_MQTT);
}
