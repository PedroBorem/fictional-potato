/*
 * idp_parser.h
 *
 *  Created on: 12 de jul. de 2023
 *      Author: bruno
 */

#ifndef COMPONENTS_IDP_PROTOCOL__INCLUDE_IDP_PARSER_H_
#define COMPONENTS_IDP_PROTOCOL__INCLUDE_IDP_PARSER_H_


#include "project_config.h"

typedef struct arg_pair{
    const char* type;
    void* value;
} arg_pair_t;

idp_type idp_parser_get(const char* string_in);

void idp_parser_create_package(char* str_out, arg_pair_t arg_pairs[]);

void idp_parser_get_packet_data(const char* str_arg, arg_pair_t arg_pairs[]);


#endif /* COMPONENTS_IDP_PROTOCOL__INCLUDE_IDP_PARSER_H_ */
