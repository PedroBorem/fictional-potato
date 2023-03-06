/*
 * http_parser.h
 *
 *  Created on: 28 de ago de 2022
 *      Author: bruno
 */

#ifndef COMPONENTS_HTTP_SERVER_INCLUDE_HTTP_CONFIG_PARSER_H_
#define COMPONENTS_HTTP_SERVER_INCLUDE_HTTP_CONFIG_PARSER_H_

#include "project_config.h"

pivot_actions http_parser_action(char * request_body);

void http_parser_action_to_json(pivot_actions action, char* out_action);

pivot_config http_parser_config(char * request_body);

void http_parser_config_to_json(pivot_config config, char* out_config);

#endif /* COMPONENTS_HTTP_SERVER_INCLUDE_HTTP_CONFIG_PARSER_H_ */
