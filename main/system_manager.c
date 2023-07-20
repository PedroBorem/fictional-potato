
//applications include
#include "system_manager.h"
#include "project_config.h"
#include "idp_parser.h"
#include "log.h"

#include "rtc_app.h"

//private include
#include <string.h>

#include "actuation_app.h"
#include "comm_app.h"
#include "data_app.h"

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
static void system_manager_idp_07(const char* buffer, comm_type comm_mode);
static void system_manager_idp_12(const char* buffer, comm_type comm_mode);
static void system_manager_idp_13(const char* buffer, comm_type comm_mode);
static void system_manager_idp_14(const char* buffer, comm_type comm_mode);
static void system_manager_idp_15(const char* buffer, comm_type comm_mode);
static void system_manager_idp_16(const char* buffer, comm_type comm_mode);

void system_manager_init(void)
{
	ESP_ERROR_CHECK(rtc_app_init());
	ESP_ERROR_CHECK(actuation_app_init(&system_manager_callback));
	ESP_ERROR_CHECK(data_app_init());
	ESP_ERROR_CHECK(comm_app_init(&system_manager_callback));
}

static void system_manager_callback(const char* buffer_request, comm_type comm_mode)
{
	esp_err_t ret = ESP_FAIL;

	char str_idp[5] = {};
	char str_out[200] = {};

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
			char pivot_id[20] = {};
			pivot_config new_config = {};

			arg_pair_t arg_pairs[] =
			{
				{ "uint8_t", &idp_request },
				{ "string", pivot_id },
				{ "string", &new_config.contactor },
				{ "string", &new_config.pressure },
				{ "uint16_t", &new_config.pressurization_time },
				{ "uint8_t", &new_config.on_off_time },
				{ NULL, NULL }
			};

			idp_parser_get_packet_data(str_out, arg_pairs);

			ret = data_app_save_config(&new_config, sizeof(new_config));
			if(ret == ESP_OK)
			{
				// todo comm_app_set_config(new_config);
			}

			break;
		}
		case IDP_4:
		{
			eco_mode_config eco_mode = {};

			arg_pair_t arg_pairs[] =
			{
				{ "uint8_t", &idp_request },
				{ "uint32_t", &eco_mode.start_time }, //todo isso deve ser timestamp?
				{ "uint32_t", &eco_mode.end_time },
				{ NULL, NULL }
			};

			idp_parser_get_packet_data(str_out, arg_pairs);
			// todo criar classe para salvar o eco na nvs.

			// todo se eco_mode.start_time  e end forem 0 nao considero

			// todo aplicar na task
			break;
		}
		case IDP_5:
		{
			sector_config sector = {};

			arg_pair_t arg_pairs[] =
			{
				{ "uint8_t", &idp_request },
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

			idp_parser_get_packet_data(str_out, arg_pairs);

			// todo criar classe para salvar os setores na nvs.

			// todo se sector_number = 0 nao tem setor

			// todo aplicar na task
			break;
		}
		case IDP_7:
		{
			time_t timestamp;
			char utc[10] = {};

			// get angle
			arg_pair_t arg_pairs[] =
			{
				{ "uint8_t", &idp_request },
				{ "uint16_t", &global_angle },
				{ "uint32_t", &timestamp },
				{ "string", utc },
				{ NULL, NULL }
			};

			if(initial_angle == 0xFFFF)
			{
				initial_angle = global_angle;
			}

			idp_parser_get_packet_data(str_out, arg_pairs);
			rtc_app_set_timestamp(timestamp);

			break;
		}
		case IDP_12:
		{
			pivot_history load_history[CONFIG_HISTORY_MAX_VALUE] = {};

			data_app_load_history(load_history, sizeof(load_history));
			// todo fazer tratamento para enviar
			break;
		}
		case IDP_13:
		{
			char scheaduling_id[10] = {};
			arg_pair_t arg_pairs[] =
			{
				{ "uint8_t", &idp_request },
				{ "string", scheaduling_id },
				{ NULL, NULL }
			};

			idp_parser_get_packet_data(str_out, arg_pairs);

			// todo fazer um delete só
			data_app_delete_scheduling(data_scheduling_date, scheaduling_id);
			data_app_delete_scheduling(data_scheduling_angle, scheaduling_id);

			// todo notificar a task de agendamento
			break;
		}
		case IDP_14:
		{
			pivot_scheduling_date scheduling = {};
			char pivot_id[20] = {};
			uint16_t dwp = 0;

			arg_pair_t arg_pairs[] =
			{
				{ "uint8_t", &idp_request },
				{ "string", pivot_id },
				{ "string", scheduling.scheduling_id },
				{ "uint32_t", &scheduling.start_date },
				{ "uint32_t", &scheduling.end_date },
				{ "uint16_t", &dwp },
				{ "uint8_t", &scheduling.actions.percentimeter },
				{ NULL, NULL }
			};

			idp_parser_get_packet_data(str_out, arg_pairs);
			idp_parser_get_pwd(dwp, &scheduling.actions);

			pivot_scheduling_date scheduling_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
			data_app_load_scheduling(data_scheduling_date, scheduling_date, sizeof(scheduling_date));

			for(uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
			{
				if(strcmp(scheduling_date[position].scheduling_id, "") == 0)
				{
					memcpy(&scheduling_date[position], &scheduling, sizeof(scheduling_date[position]));

					// get_rtc
					scheduling_date[position].start_date += rtc_app_get_timestamp(false);
					scheduling_date[position].end_date += rtc_app_get_timestamp(false);

					data_app_gen_scheduling_key((char*)&scheduling_date[position].scheduling_id);
					data_app_save_scheduling(data_scheduling_date, scheduling_date, sizeof(scheduling_date));

					ESP_LOGI(SYSTEM_MANAGER_TAG, "Save schedule date id : %s", scheduling_date[position].scheduling_id);
					// todo notificar a classe de agendamento
					break;
				}
			}

			break;
		}
		case IDP_15:
		{
			pivot_scheduling_angle scheduling = {};
			char pivot_id[20] = {};
			uint16_t dwp = 0;

			arg_pair_t arg_pairs[] =
			{
				{ "uint8_t", &idp_request },
				{ "string", pivot_id },
				{ "string", scheduling.scheduling_id },
				{ "uint32_t", &scheduling.start_date },
				{ "uint16_t", &scheduling.end_angle },
				{ "uint16_t", &dwp },
				{ "uint8_t", &scheduling.actions.percentimeter },
				{ NULL, NULL }
			};

			idp_parser_get_packet_data(str_out, arg_pairs);
			idp_parser_get_pwd(dwp, &scheduling.actions);

			pivot_scheduling_angle scheduling_angle[CONFIG_SCHEDULING_MAX_VALUE] = {};
			data_app_load_scheduling(data_scheduling_angle, scheduling_angle, sizeof(scheduling_angle));

			for(uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
			{
				if(strcmp(scheduling_angle[position].scheduling_id, "") == 0)
				{
					memcpy(&scheduling_angle[position], &scheduling, sizeof(scheduling_angle[position]));

					// get_rtc
					scheduling_angle[position].start_date += rtc_app_get_timestamp(false);
					data_app_gen_scheduling_key((char*)&scheduling_angle[position].scheduling_id);
					data_app_save_scheduling(data_scheduling_angle, scheduling_angle, sizeof(scheduling_angle));

					// todo notificar a classe de agendamento
					ESP_LOGI(SYSTEM_MANAGER_TAG, "Save schedule angle id : %s", scheduling_angle[position].scheduling_id);
					break;
				}
			}
			break;
		}
		case IDP_16:
		{
			pivot_scheduling_date scheduling = {};
			arg_pair_t arg_pairs[] =
			{
				{ "uint8_t", &idp_request },
				{ "uint32_t", &scheduling.end_date },
				{ NULL, NULL }
			};

			idp_parser_get_packet_data(str_out, arg_pairs);

			pivot_scheduling_date scheduling_date[CONFIG_SCHEDULING_MAX_VALUE] = {};
			data_app_load_scheduling(data_scheduling_date, scheduling_date, sizeof(scheduling_date));

			for(uint8_t position = 0; position < CONFIG_SCHEDULING_MAX_VALUE; position++)
			{
				if(strcmp(scheduling_date[position].scheduling_id, "") == 0)
				{
					memcpy(&scheduling_date[position], &scheduling, sizeof(scheduling_date[position]));

					// get_rtc
					scheduling_date[position].start_date += rtc_app_get_timestamp(false);
					scheduling_date[position].end_date += rtc_app_get_timestamp(false);

					data_app_gen_scheduling_key((char*)&scheduling_date[position].scheduling_id);
					data_app_save_scheduling(data_scheduling_date, scheduling_date, sizeof(scheduling_date));

					ESP_LOGI(SYSTEM_MANAGER_TAG, "Save schedule date id : %s", scheduling_date[position].scheduling_id);
					// todo notificar a classe de agendamento
					break;
				}
			}

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

	LOG_MANAGER(SYSTEM_MANAGER_TAG, "%s", str_out);
}



static void system_manager_idp_00(const char* buffer, comm_type comm_mode)
{
	pivot_actions actions = {};
	char str_out[200] = {};
	uint16_t dwp = 0;
	uint8_t idp = 0;

	actuation_app_get_actions(&actions, sizeof(actions));
	dwp = idp_parser_create_pwd(actions);

	time_t timestamp = rtc_app_get_timestamp(false);

	arg_pair_t arg_pairs[] = {
		{ "uint8_t", &idp },
		{ "uint16_t", &dwp },
		{ "uint8_t", &actions.percentimeter },
		{ "uint16_t", &initial_angle },
		{ "uint16_t", &global_angle },
		{ "uint32_t", &timestamp },
		{ NULL, NULL } // TODO fazer oq falta;
	};

	idp_parser_create_package(str_out,arg_pairs);
	comm_app_send_idp_pack(str_out, comm_mode);
}

static void system_manager_idp_01(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_POST || comm_mode == COMM_MQTT)
	{
		esp_err_t ret = ESP_FAIL;
		pivot_actions new_actions = {};
		pivot_history new_history = {};

		char str_out[200] = {};
		char pivot_id[20] = {};
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

		idp_parser_get_packet_data(str_out, arg_pairs);
		idp_parser_get_pwd(dwp, &new_actions);

		ret = data_app_save_actions(&new_actions, sizeof(new_actions));
		if(ret == ESP_OK)
		{
			// save old history
			data_app_save_old_history(rtc_app_get_timestamp(false), global_angle);

			// act on the equipment
			actuation_app_set_actions(new_actions, false);

			// send current status
			arg_pair_t arg_pairs_ack[] =
			{
				{ "uint8_t", &idp },
				{ "string", pivot_id },
				{ NULL, NULL }
			};

			idp_parser_create_package(str_out, arg_pairs_ack);

			comm_app_send_idp_pack(str_out, COMM_HTTP_POST);
			comm_app_send_idp_pack(str_out, COMM_MQTT);

			// save new history
			if(new_actions.power_state != PIVOT_OFF)
			{
				new_history.start_date = rtc_app_get_timestamp(false);
				new_history.start_angle = global_angle;
				memcpy(&new_history.actions, &new_actions, sizeof(new_actions));
				data_app_save_new_history(new_history);
			}
		}
	}
}

static void system_manager_idp_02(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_POST)
	{
		char str_out[200] = {};
		char pivot_id[20] = {};
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

		// todo criar classe para salvar network_config na nvs.
		comm_app_wifi_config(net_config.wifi_ssid, net_config.wifi_pass);

		// send GPRS module
		idp = IDP_6;
		arg_pair_t arg_pairs_2[] = {
			{ "uint8_t", &idp },
			{ "string", net_config.gprs_id },
			{ "string", net_config.modem_apn },
			{ NULL, NULL }
		};

		idp_parser_create_package(str_out,arg_pairs_2);
		comm_app_send_idp_pack(str_out, COMM_MQTT);
	}
	else if(comm_mode == COMM_HTTP_GET)
	{
		char str_out[200] = {};
		char pivot_id[20] = {};
		network_config net_config = {};
		uint8_t idp = IDP_2;

		// todo criar classe para ler network_config na nvs.

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
	if(comm_mode == COMM_HTTP_POST)
	{

	}
	else if(comm_mode == COMM_HTTP_GET)
	{

	}
	else if(comm_mode == COMM_MQTT)
	{

	}
}

static void system_manager_idp_04(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_POST)
	{

	}
	else if(comm_mode == COMM_HTTP_GET)
	{

	}
	else if(comm_mode == COMM_MQTT)
	{

	}
}

static void system_manager_idp_05(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_POST)
	{

	}
	else if(comm_mode == COMM_HTTP_GET)
	{

	}
	else if(comm_mode == COMM_MQTT)
	{

	}
}

static void system_manager_idp_07(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_POST)
	{

	}
	else if(comm_mode == COMM_HTTP_GET)
	{

	}
	else if(comm_mode == COMM_MQTT)
	{

	}
}

static void system_manager_idp_12(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_POST)
	{

	}
	else if(comm_mode == COMM_HTTP_GET)
	{

	}
	else if(comm_mode == COMM_MQTT)
	{

	}
}

static void system_manager_idp_13(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_POST)
	{

	}
	else if(comm_mode == COMM_HTTP_GET)
	{

	}
	else if(comm_mode == COMM_MQTT)
	{

	}
}

static void system_manager_idp_14(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_POST)
	{

	}
	else if(comm_mode == COMM_HTTP_GET)
	{

	}
	else if(comm_mode == COMM_MQTT)
	{

	}
}

static void system_manager_idp_15(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_POST)
	{

	}
	else if(comm_mode == COMM_HTTP_GET)
	{

	}
	else if(comm_mode == COMM_MQTT)
	{

	}
}

static void system_manager_idp_16(const char* buffer, comm_type comm_mode)
{
	if(comm_mode == COMM_HTTP_POST)
	{

	}
	else if(comm_mode == COMM_HTTP_GET)
	{

	}
	else if(comm_mode == COMM_MQTT)
	{

	}
}
