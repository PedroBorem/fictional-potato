
//applications include
#include "data_app.h"
#include "comm_app.h"
#include "actuation_app.h"

#include "project_config.h"
#include "idp_parser.h"
#include "log.h"

#include "rtc_app.h"

#define SYSTEM_MANAGER_TAG 	"system manager"

void system_manager_callback(char* buffer_request, comm_type communication)
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
			pivot_actions actions = {};
			uint16_t dwp = 0;

			actuation_app_get_actions(&actions, sizeof(actions));

			dwp = idp_parser_create_pwd(actions);

			arg_pair_t arg_pairs[] = {
				{ "uint8_t", &idp_request },
				{ "uint16_t", &dwp },
				{ "uint8_t", &actions.percentimeter },
				{ NULL, NULL } // TODO fazer oq falta;
			};

			idp_parser_create_package(str_out,arg_pairs);

			//TODO send comm_app;

			break;
		}
		case IDP_1:
		{
			pivot_actions new_actions = {};
			pivot_history new_history = {};

			char pivot_id[20] = {};
			uint16_t dwp = 0;

			arg_pair_t arg_pairs[] =
			{
				{ "uint8_t", &idp_request },
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
				data_app_save_old_history(rtc_app_get_timestamp(false), comm_app_get_degree());

				// act on the equipment
				actuation_app_set_actions(new_actions, false);

				// send current status
				// TODO comm_app_send_event(new_actions);

				// save new history
				if(new_actions.power_state != PIVOT_OFF)
				{
					new_history.start_date = rtc_app_get_timestamp(false);
					new_history.start_angle = comm_app_get_degree();
					memcpy(&new_history.actions, &new_actions, sizeof(new_actions));
					data_app_save_new_history(new_history);
				}
			}
			else if(new_actions.rotation == PIVOT_UNKNOWN)
			{
				// TODO comm_app_send_event(new_actions);
			}

			break;
		}
		case IDP_2:
		{
			char pivot_id[20] = {};
			network_config net_config = {};

			arg_pair_t arg_pairs[] =
			{
				{ "uint8_t", &idp_request },
				{ "string", pivot_id },
				{ "string", &net_config.gprs_id },
				{ "string", &net_config.modem_apn },
				{ "string", &net_config.wifi_ssid },
				{ "string", &net_config.wifi_pass },
				{ NULL, NULL }
			};

			idp_parser_get_packet_data(str_out, arg_pairs);
			// todo ajustar o wifi com novo id e pass.

			// todo criar classe para salvar network_config na nvs.

			arg_pair_t arg_pairs_out[] = {
				{ "uint8_t", &idp_request },
				{ "string", &net_config.gprs_id },
				{ "string", &net_config.modem_apn },
				{ NULL, NULL }
			};

			idp_parser_create_package(str_out,arg_pairs);
			// todo enviar o 06 para o edu

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
			break;
		}
		case IDP_5:
		{
			break;
		}
		case IDP_6:
		{
			break;
		}
		case IDP_7:
		{
			break;
		}
		case IDP_8:
		{
			break;
		}
		case IDP_9:
		{
			break;
		}
		case IDP_10:
		{
			break;
		}
		case IDP_11:
		{
			break;
		}
		case IDP_12:
		{
			break;
		}
		case IDP_13:
		{
			break;
		}
		case IDP_14:
		{
			break;
		}
		case IDP_15:
		{
			break;
		}
		case IDP_16:
		{
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
