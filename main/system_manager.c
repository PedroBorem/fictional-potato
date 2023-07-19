
//applications include
#include "data_app.h"
#include "comm_app.h"
#include "actuation_app.h"

#include "project_config.h"
#include "idp_parser.h"
#include "log.h"


#define SYSTEM_MANAGER_TAG 	"system manager"

void system_manager_callback(char* buffer_request)
{
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

			dwp = ((actions.rotation * 100)
								+ (actions.watering_state)
								* 10 + actions.power_state);


			arg_pair_t arg_pairs[] = {
				{ "uint8_t", &idp_request },
				{ "uint16_t", &dwp },
				{ "uint8_t", &actions.percentimeter },
				{ NULL, NULL } // Marca o final do array arg_pairs
			};

			idp_parser_create_package(str_out,arg_pairs);

			break;
		}
		case IDP_1:
		{
			break;
		}
		case IDP_2:
		{
			break;
		}
		case IDP_3:
		{
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
