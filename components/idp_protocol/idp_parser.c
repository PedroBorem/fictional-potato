/*
 * idp_parser.c
 *
 *  Created on: 12 de jul. de 2023
 *      Author: bruno
 */

/* Private inclusions -------------------------------------------- */
#include "idp_parser.h"
#include "log.h"

#include "esp_err.h"

#include <string.h>

/* Private definitions ------------------------------------------- */
#define IDP_PARSER_TAG	"idp_parser"

/* Private methods ----------------------------------------------- */
esp_err_t common_parser_check_pack(const char* string_in)
{
	esp_err_t ret = ESP_FAIL;

	size_t str_last_position = strlen(string_in) - 1;
	if(string_in[0] == '$' && string_in[str_last_position] == '#' )
	{
		LOG_COMM(IDP_PARSER_TAG, "package : %s", string_in);
		ret = ESP_OK;
	}
	else
	{
		ESP_LOGE(IDP_PARSER_TAG, "invalid package");
	}

	return ret;
}

/* Public methods ------------------------------------------------ */
idp_type common_parser_get_idp(const char* string_in)
{
	idp_type idp_ret = IDP_INVALID;

    char delimiter[] = "-";
	char* ptr = strtok(string_in, delimiter);

	if(common_parser_check_pack(string_in) == ESP_OK)
	{
	    sscanf(&ptr[1], "%d",(int*)&idp_ret);
	}
	else
	{
		ESP_LOGE(IDP_PARSER_TAG, "invalid idp");
	}

	return idp_ret;
}



