/*
 * commom_parser.h
 *
 *  Created on: 21 de jun. de 2022
 *      Author: brunolima
 */

#ifndef COMPONENTS_COMMON_COMMUNICATION_INCLUDE_COMMON_PARSER_H_
#define COMPONENTS_COMMON_COMMUNICATION_INCLUDE_COMMON_PARSER_H_

#include "project_config.h"

esp_err_t common_parser_string_to_status(const char* sting_in, pivot_config* config_out);

esp_err_t common_parser_status_to_string(const pivot_config config_in, char* string_out);

#endif /* COMPONENTS_COMMON_COMMUNICATION_INCLUDE_COMMON_PARSER_H_ */
