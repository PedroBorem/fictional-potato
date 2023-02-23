/*
 * http_parser.h
 *
 *  Created on: 28 de ago de 2022
 *      Author: bruno
 */

#ifndef COMPONENTS_HTTP_SERVER_INCLUDE_HTTP_CONFIG_PARSER_H_
#define COMPONENTS_HTTP_SERVER_INCLUDE_HTTP_CONFIG_PARSER_H_

#include "project_config.h"

/**
 * @brief	Parser all fields received.
 * @param	received_post[in] - string of a POST request
 * @param	field_name[in] - string of desired field from web page
 * @return
 *  - Allocated string for the field value
 *  - NULL: if there is no config with name field_name
 */
char * http_config_parser(const char* received_post, const char* field_name);

pivot_actions http_parser_action(char * request_body);


pivot_config http_parser_config(char * request_body);

#endif /* COMPONENTS_HTTP_SERVER_INCLUDE_HTTP_CONFIG_PARSER_H_ */
