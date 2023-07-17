
//applications include
#include "data_app.h"
#include "comm_app.h"
#include "actuation_app.h"

#include "project_config.h"
#include "idp_parser.h"
#include "log.h"


#include "FreeRTOS_defines.h"

void system_manager_callback(char* buffer_request)
{
	idp_type idp_request = idp_parser_get(buffer_request);

	switch(idp_request)
	{
		case IDP_0:
		{
			pivot_actions actions = {};
			char str_out[200] = {};

			const char* mock_gprs_id = "teste_inatel";

			actuation_app_get_actions(&actions, sizeof(actions));

			idp_parser_set(str_out,
			 	 	 mock_gprs_id,
					"uint8_t", idp_request,
					"uint8_t", actions.rotation,
					"uint8_t", actions.watering_state,
					"uint8_t", actions.power_state,
					"uint8_t", actions.percentimeter,
					NULL);


			//idp_parser_set(str_out, mock_gprs_id, "int8_t", idp_request, "str2", "uint16_t", (uint16_t)123, "str3", NULL);

			LOG_MANAGER("TESTE", "%s", str_out);

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
}
