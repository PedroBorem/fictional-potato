/*
 * idp_parser.h
 *
 *  Created on: 12 de jul. de 2023
 *      Author: bruno
 */

#ifndef COMPONENTS_IDP_PROTOCOL__INCLUDE_IDP_PARSER_H_
#define COMPONENTS_IDP_PROTOCOL__INCLUDE_IDP_PARSER_H_


#include "project_config.h"


idp_type idp_parser_get(const char* string_in);

void idp_parser_set(char* str_out, const char* str, ...);


#endif /* COMPONENTS_IDP_PROTOCOL__INCLUDE_IDP_PARSER_H_ */
