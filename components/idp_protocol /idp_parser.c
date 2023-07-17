/*
 * idp_parser.c
 *
 *  Created on: 12 de jul. de 2023
 *      Author: bruno
 */

#include "idp_parser.h"

idp_type common_parser_get_idp(const char* string_in)
{
	idp_type idp_ret;
	char pack_delimiter = string_in[0];
	char idp_value = string_in[1];
	char idp_delimiter = string_in[2];

	if(idp_value >= '0' && idp_value <= '9'
	&& idp_delimiter == '-' && pack_delimiter == '#')
	{
		idp_ret = idp_value - '0';
	}
	else
	{
		idp_ret = IDP_INVALID;
	}

	return idp_ret;
}



