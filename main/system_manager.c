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

// private include
#include <string.h>

#include "actuation_app.h"
#include "comm_app.h"
#include "data_app.h"

#include "system_monitoring.h"
#include "sectorization.h"
#include "scheaduling.h"

#define SYSTEM_MANAGER_TAG 	"system manager"

/* global variables */
uint16_t global_angle = 0xFFFF;
static uint16_t initial_angle = 0xFFFF;

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
static void system_manager_idp_30(const char* buffer, comm_type comm_mode);

void system_manager_init(void)
{
	// rtc init
	ESP_ERROR_CHECK(rtc_app_init());

	// actuation init
	ESP_ERROR_CHECK(actuation_app_init(&system_manager_callback));

	// nvs init
	ESP_ERROR_CHECK(data_app_init());

	// system monitoring init
	system_monitoring_register_callback(&system_manager_callback);
	system_monitoring_start();

	// communication modules init
	network_config network = {};
	data_app_load(DATA_TYPE_NETWORK_CONFIG, &network);

	comm_app_wifi_config(network.wifi_ssid, network.wifi_pass);
	ESP_ERROR_CHECK(comm_app_init(&system_manager_callback));

	// init sectors
	sector_config sectors = {};
	data_app_load(DATA_TYPE_SECTOR_CONFIG, &sectors);
	sectorization_start(sectors);

	// init scheduling
	pivot_scheduling_date scheduling_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
	pivot_scheduling_angle scheduling_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};

	data_app_load(DATA_TYPE_SCHEADULING_DATE, &scheduling_date);
	data_app_load(DATA_TYPE_SCHEADULING_ANGLE, &scheduling_angle);
	scheduling_register_callback(&system_manager_callback);
	scheduling_start(scheduling_date, scheduling_angle);
}

static void system_manager_callback(const char* buffer_request, comm_type comm_mode)
{
	LOG_MANAGER(SYSTEM_MANAGER_TAG, "%s", buffer_request);

	char str_idp[5] = {};

	idp_type idp_request = idp_parser_get(buffer_request);
    snprintf(str_idp, sizeof(str_idp), "%d", idp_request);

	switch(idp_request)
	{
		case IDP_0:
		{
			system_manager_idp_00(buffer_request, comm_mode);
			break;
		}
		case IDP_1:
		{
			system_manager_idp_01(buffer_request, comm_mode);
			break;
		}
		case IDP_2:
		{
			system_manager_idp_02(buffer_request, comm_mode);
			break;
		}
		case IDP_3:
		{
			system_manager_idp_03(buffer_request, comm_mode);
			break;
		}
		case IDP_4:
		{
			system_manager_idp_04(buffer_request, comm_mode);
			break;
		}
		case IDP_5:
		{
			system_manager_idp_05(buffer_request, comm_mode);
			break;
		}
		case IDP_6:
		{
			system_manager_idp_06(buffer_request, comm_mode);
			break;
		}
		case IDP_7:
		{
			system_manager_idp_07(buffer_request, comm_mode);
			break;
		}
		case IDP_12:
		{
			system_manager_idp_12(buffer_request, comm_mode);
			break;
		}
		case IDP_13:
		{
			system_manager_idp_13(buffer_request, comm_mode);
			break;
		}
		case IDP_14:
		{
			system_manager_idp_14(buffer_request, comm_mode);
			break;
		}
		case IDP_15:
		{
			system_manager_idp_15(buffer_request, comm_mode);
			break;
		}
		case IDP_16:
		{
			system_manager_idp_16(buffer_request, comm_mode);
			break;
		}
		case IDP_30:
		{
			system_manager_idp_30(buffer_request, comm_mode);
			break;
		}
		case IDP_INVALID:
		{
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
	char str_date[20] = {};
	char str_time[20] = {};
	uint16_t dwp = 0;
	uint8_t idp = 0;

	actuation_app_get_actions(&actions, sizeof(actions));
	dwp = idp_parser_create_pwd(actions);

	time_t timestamp = rtc_app_get_timestamp(false);
	rtc_app_get_str_date(timestamp, str_date);
	rtc_app_get_str_time(timestamp, str_time);

	pivot_config config = {};
	data_app_load(DATA_TYPE_PIVOT_CONFIG, &config);

	arg_pair_t arg_pairs[] = {
		{ "uint8_t", &idp },
		{ "string", config.pivot_id },
		{ "uint16_t", &dwp },
		{ "uint8_t", &actions.percentimeter },
		{ "uint16_t", &initial_angle },
		{ "uint16_t", &global_angle },
		{ "string", str_date },
		{ "string", str_time },
		{ NULL, NULL }
	};

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
			{ "uint8_t", &new_actions.percentimeter },
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
			actuation_app_set_actions(new_actions, false);

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
}

static void system_manager_idp_02(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_POST || comm_mode == COMM_MQTT)
	{
		char str_out[200] = {};
		char pivot_id[50] = {};
		network_config net_config = {};
		uint8_t idp = 0;

		arg_pair_t arg_pairs[] =
		{
			{ "uint8_t", &idp },
			{ "string", pivot_id },
			{ "string", net_config.gprs_id },
			{ "string", net_config.modem_apn },
			{ "string", net_config.wifi_ssid },
			{ "string", net_config.wifi_pass },
			{ NULL, NULL }
		};

		idp_parser_get_packet_data(buffer, arg_pairs);

		data_app_save(DATA_TYPE_NETWORK_CONFIG, &net_config, sizeof(net_config));
		comm_app_wifi_config(net_config.wifi_ssid, net_config.wifi_pass);

		// send ACK
		arg_pair_t arg_pairs_2[] = {
			{ "uint8_t", &idp },
			{ "string", pivot_id },
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
		char pivot_id[50] = {};
		network_config net_config = {};
		uint8_t idp = IDP_2;

		data_app_load(DATA_TYPE_NETWORK_CONFIG, &net_config);

		arg_pair_t arg_pairs[] =
		{
			{ "uint8_t", &idp },
			{ "string", pivot_id },
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
			{ "string", new_config.pivot_id },
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
				{ "string", new_config.pivot_id },
				{ NULL, NULL }
			};

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
			{ "string", config.pivot_id },
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

		uint8_t idp = IDP_4;
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
	time_t timestamp;
	char utc[10] = {};

	// get angle
	arg_pair_t arg_pairs[] =
	{
		{ "uint8_t", &idp },
		{ "uint16_t", &global_angle },
		{ "uint32_t", &timestamp },
		{ "string", utc },
		{ NULL, NULL }
	};

	if(initial_angle == 0xFFFF)
	{
		initial_angle = global_angle;
	}

	idp_parser_get_packet_data(buffer, arg_pairs);
	rtc_app_set_timestamp(timestamp);
}

static void system_manager_idp_12(const char* buffer, comm_type comm_mode)
{
	pivot_history load_history[CONFIG_HISTORY_MAX_VALUE] = {};

	data_app_load(DATA_TYPE_HISTORY, &load_history);
	// todo fazer tratamento para enviar
}

static void system_manager_idp_13(const char* buffer, comm_type comm_mode)
{
	pivot_scheduling_date scheduling_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
	pivot_scheduling_angle scheduling_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};

	uint8_t idp = 0;
	char str_out[200] = {};
	char pivot_id[50] = {};
	char str_author[30] = {};
	char scheaduling_id[10] = {};

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

	scheduling_stop();
	scheduling_start(scheduling_date, scheduling_angle);

	// send ack
	pivot_config config = {};
	data_app_load(DATA_TYPE_PIVOT_CONFIG, &config);

	arg_pair_t arg_pairs_2[] =
	{
		{ "uint8_t", &idp },
		{ "string", config.pivot_id },
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

		arg_pair_t arg_pairs[] =
		{
			{ "uint8_t", &idp },
			{ "string", pivot_id},
			{ "string", scheduling.scheduling_id },
			{ "uint32_t", &scheduling.start_date },
			{ "uint32_t", &scheduling.end_date },
			{ "uint16_t", &dwp },
			{ "uint8_t", &scheduling.actions.percentimeter },
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

				// get_rtc
				scheduling_date[position].start_date += rtc_app_get_timestamp(false);
				scheduling_date[position].end_date += rtc_app_get_timestamp(false);

				data_app_gen_scheduling_key((char*)&scheduling_date[position].scheduling_id);
				data_app_save(DATA_TYPE_SCHEADULING_DATE, &scheduling_date, sizeof(scheduling_date));

				scheduling_stop();
				scheduling_start(scheduling_date, NULL);

				ESP_LOGI(SYSTEM_MANAGER_TAG, "Save schedule date id : %s", scheduling_date[position].scheduling_id);

				// send ack
				pivot_config config = {};
				data_app_load(DATA_TYPE_PIVOT_CONFIG, &config);

				arg_pair_t arg_pairs_2[] =
				{
					{ "uint8_t", &idp },
					{ "string", config.pivot_id },
					{ "uint32_t", &scheduling.start_date },
					{ "uint32_t", &scheduling.end_date },
					{ "uint16_t", &dwp },
					{ "uint8_t", &scheduling.actions.percentimeter },
					{ NULL, NULL }
				};

				idp_parser_create_package(str_out, arg_pairs_2);

				comm_app_send_idp_pack(str_out, COMM_HTTP_POST);
				comm_app_send_idp_pack(str_out, COMM_MQTT);

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

		for(uint8_t position = 0; position <= scheduling_size; position++)
		{
			dwp = idp_parser_create_pwd(scheduling_date[position].actions);

			pivot_config config = {};
			data_app_load(DATA_TYPE_PIVOT_CONFIG, &config);

			arg_pair_t arg_pairs[] =
			{
				{ "uint8_t", &idp },
				{ "string", config.pivot_id },
				{ "string", scheduling_date[position].scheduling_id },
				{ "uint32_t", &scheduling_date[position].start_date },
				{ "uint32_t", &scheduling_date[position].end_date },
				{ "uint16_t", &dwp },
				{ "uint8_t", &scheduling_date[position].actions.percentimeter },
				{ NULL, NULL }
			};

			idp_parser_create_package(str_out, arg_pairs);

			strcat(buffer_out, str_out);
			strcat(buffer_out, "\n");
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
			{ "string", scheduling.scheduling_id },
			{ "uint32_t", &scheduling.start_date },
			{ "uint16_t", &scheduling.end_angle },
			{ "uint16_t", &dwp },
			{ "uint8_t", &scheduling.actions.percentimeter },
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

				// get_rtc
				scheduling_angle[position].start_date += rtc_app_get_timestamp(false);
				data_app_gen_scheduling_key((char*)&scheduling_angle[position].scheduling_id);
				data_app_save(DATA_TYPE_SCHEADULING_ANGLE, &scheduling_angle, sizeof(scheduling_angle));

				scheduling_stop();
				scheduling_start(NULL, scheduling_angle);

				ESP_LOGI(SYSTEM_MANAGER_TAG, "Save schedule angle id : %s", scheduling_angle[position].scheduling_id);

				pivot_config config = {};
				data_app_load(DATA_TYPE_PIVOT_CONFIG, &config);

				arg_pair_t arg_pairs_2[] =
				{
					{ "uint8_t", &idp },
					{ "string", config.pivot_id },
					{ "string", scheduling.scheduling_id },
					{ "uint32_t", &scheduling.start_date },
					{ "uint16_t", &scheduling.end_angle },
					{ "uint16_t", &dwp },
					{ "uint8_t", &scheduling.actions.percentimeter },
					{ NULL, NULL }
				};

				idp_parser_create_package(str_out, arg_pairs_2);

				comm_app_send_idp_pack(str_out, COMM_HTTP_POST);
				comm_app_send_idp_pack(str_out, COMM_MQTT);
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

		for(uint8_t position = 0; position <= scheduling_size; position++)
		{
			dwp = idp_parser_create_pwd(scheduling_angle[position].actions);

			pivot_config config = {};
			data_app_load(DATA_TYPE_PIVOT_CONFIG, &config);

			arg_pair_t arg_pairs[] =
			{
				{ "uint8_t", &idp },
				{ "string", config.pivot_id },
				{ "string", scheduling_angle[position].scheduling_id },
				{ "uint32_t", &scheduling_angle[position].start_date },
				{ "uint16_t", &scheduling_angle[position].end_angle },
				{ "uint16_t", &dwp },
				{ "uint8_t", &scheduling_angle[position].actions.percentimeter },
				{ NULL, NULL }
			};

			idp_parser_create_package(str_out, arg_pairs);

			strcat(buffer_out, str_out);
			strcat(buffer_out, "\n");
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

		pivot_scheduling_date scheduling = {};
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

		pivot_scheduling_date scheduling_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
		data_app_load(DATA_TYPE_SCHEADULING_DATE, &scheduling_date);

		for(uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
		{
			if(strcmp(scheduling_date[position].scheduling_id, "") == 0)
			{
				memcpy(&scheduling_date[position], &scheduling, sizeof(scheduling_date[position]));

				// get_rtc
				scheduling_date[position].start_date += rtc_app_get_timestamp(false);
				scheduling_date[position].end_date += rtc_app_get_timestamp(false);

				data_app_gen_scheduling_key((char*)&scheduling_date[position].scheduling_id);
				data_app_save(DATA_TYPE_SCHEADULING_DATE, &scheduling_date, sizeof(scheduling_date));

				scheduling_stop();
				scheduling_start(scheduling_date, NULL);

				ESP_LOGI(SYSTEM_MANAGER_TAG, "Save schedule date id : %s", scheduling_date[position].scheduling_id);

				pivot_config config = {};
				data_app_load(DATA_TYPE_PIVOT_CONFIG, &config);

				arg_pair_t arg_pairs_2[] =
				{
					{ "uint8_t", &idp },
					{ "string", config.pivot_id },
					{ "string", scheduling_date[position].scheduling_id },
					{ "uint32_t", &scheduling.end_date },
					{ NULL, NULL }
				};

				idp_parser_create_package(str_out, arg_pairs_2);

				comm_app_send_idp_pack(str_out, COMM_HTTP_POST);
				comm_app_send_idp_pack(str_out, COMM_MQTT);

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

		for(uint8_t position = 0; position <= scheduling_size; position++)
		{
			if(scheduling_date[position].start_date == 0
			&& scheduling_date[position].end_date != 0)
			{
				pivot_config config = {};
				data_app_load(DATA_TYPE_PIVOT_CONFIG, &config);

				arg_pair_t arg_pairs[] =
				{
					{ "uint8_t", &idp },
					{ "string", config.pivot_id },
					{ "string", scheduling_date[position].scheduling_id },
					{ "uint32_t", &scheduling_date[position].end_date },
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

static void system_manager_idp_30(const char* buffer, comm_type comm_mode)
{
	uint8_t idp = 0;
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

		actuation_app_shutdown();
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
			{ "uint8_t", &new_actions.percentimeter },
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
			actuation_app_set_actions(new_actions, false);

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
	char str_out[200] = {};
	network_config config = {};
	data_app_load(DATA_TYPE_NETWORK_CONFIG, &config);

	arg_pair_t arg_pairs_ack[] =
	{
		{ "uint8_t", &idp },
		{ "string", config.gprs_id },
		{ NULL, NULL }
	};

	idp_parser_create_package(str_out, arg_pairs_ack);
	comm_app_send_idp_pack(str_out, COMM_MQTT);
}
