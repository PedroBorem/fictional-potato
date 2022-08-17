/*
 * commom_parser.h
 *
 *  Created on: 21 de jun. de 2022
 *      Author: brunolima
 */

#ifndef COMPONENTS_COMMON_COMMUNICATION_INCLUDE_COMMON_PARSER_H_
#define COMPONENTS_COMMON_COMMUNICATION_INCLUDE_COMMON_PARSER_H_

/**
 * @file common_parser.h
 * @date June 21, 2022
 * @brief parse and convert incoming and outgoing messages
*/

#include "project_config.h"

#define common_parser_json_to_config(string_in,config_out) common_parser_string_to_config(string_in,config_out)

/**
 * @brief	convert a string to a configuration structure
 * @param 	sting_in[in]: input string, format(xxx-xxx)
 * @param 	config_out[out]: output configuration structure
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail
 */
esp_err_t common_parser_string_to_config(const char* string_in, pivot_config* config_out);

/**
 * @brief	convert string to angle and timestamp variables
 * @param 	sting_in[in]: input string, format(GPS...)
 * @param 	angle[out]: angle in degrees converted
 * @param 	timestamp[out]: converted timestamp
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail
 */
esp_err_t common_parser_string_to_gnss(const char* string_in, uint16_t* angle, time_t* timestamp);

/**
 * @brief	convert configuration structure to string
 * @param 	config_in[in]: input configuration structure
 * @param 	timestamp[out]: input timestamp
 * @param 	angle[out]: angle in degrees
 * @param 	string_out[out]: converted string
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail
 */
esp_err_t common_parser_status_to_string(pivot_config config_in,time_t timestamp, uint16_t angle, char* string_out);


esp_err_t common_parser_json_to_timestamp(const char* string_in, time_t* timestamp);

esp_err_t common_parser_status_to_json(pivot_config config_in,time_t timestamp, uint16_t angle, char* string_out);

#endif /* COMPONENTS_COMMON_COMMUNICATION_INCLUDE_COMMON_PARSER_H_ */
