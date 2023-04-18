/*
 * http_parser.h
 *
 *  Created on: 28 de ago de 2022
 *      Author: bruno
 */

#ifndef COMPONENTS_HTTP_SERVER_INCLUDE_HTTP_CONFIG_PARSER_H_
#define COMPONENTS_HTTP_SERVER_INCLUDE_HTTP_CONFIG_PARSER_H_

#include "project_config.h"

pivot_actions http_parser_action(char * request_body, bool scheduling);
void http_parser_action_to_json(const pivot_actions action, const pivot_config config, uint16_t start_angle, uint16_t end_angle, char* out_action);

pivot_config http_parser_config(char * request_body);
void http_parser_config_to_json(pivot_config config, char* out_config);

pivot_scheduling_date http_parser_scheduling_date(char* request_body);
void http_parser_scheduling_date_to_json(pivot_scheduling_date* scheduling_date, char* out_scheduling);

pivot_scheduling_angle http_parser_scheduling_angle(char* request_body);
void http_parser_scheduling_angle_to_json(pivot_scheduling_angle* scheduling_angle, char* out_scheduling);

char* http_parser_scheduling_delete(char* request_body);


void http_parser_cycles_to_json(char* out_cycles);

#endif /* COMPONENTS_HTTP_SERVER_INCLUDE_HTTP_CONFIG_PARSER_H_ */
