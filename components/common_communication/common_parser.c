/*
 * common_parser.c
 *
 *  Created on: 21 de jun. de 2022
 *      Author: brunolima
 */

/**
 * @file common_parser.c
 * @date June 21, 2022
 * @brief parse and convert incoming and outgoing messages
*/

/* Self include */
#include "common_parser.h"

/**\addtogroup components
 * @{
 *
 */

/**\addtogroup common_parser
 * @{
 *
 */

/* Private definitions ------------------------------------------- */
#define	COMMON_PARSER_POWER_POSITION				(2)
#define	COMMON_PARSER_ROTATION_POSITION				(0)
#define	COMMON_PARSER_WATERING_POSITION				(1)
#define	COMMON_PARSER_PERCENTIMETER_POSITION		(4)

#define COMMOM_PARSER_ELEMENT_NUMBER				(4)


/* Public methods ------------------------------------------------ */
esp_err_t common_parser_string_to_config(const char* sting_in, pivot_config* config_out)
{
	esp_err_t err = ESP_ERR_INVALID_ARG;
	pivot_config config = {};
	char status;
	int validate_ret = 0;

	status = sting_in[COMMON_PARSER_POWER_POSITION];
	validate_ret += sscanf(&status, "%d",(int*)&config.power_state);

	status = sting_in[COMMON_PARSER_ROTATION_POSITION];
	validate_ret += sscanf(&status, "%d",(int*)&config.rotation);

	status = sting_in[COMMON_PARSER_WATERING_POSITION];
	validate_ret += sscanf(&status, "%d",(int*)&config.watering_state);

	validate_ret += sscanf(&sting_in[COMMON_PARSER_PERCENTIMETER_POSITION], "%d", (int*)&config.percentimeter);

	if(validate_ret >= COMMOM_PARSER_ELEMENT_NUMBER)
	{
		memcpy(config_out, &config, sizeof(pivot_config));
		err = ESP_OK;
	}

	return err;
}

esp_err_t common_parser_string_to_gnss(const char* sting_in, uint16_t* angle, time_t* timestamp)
{
	esp_err_t err = ESP_OK;

	// assistant buffers
	char angle_out[6] = "";
	char timestamp_out[11] = "";
	char search_timestamp[] = "#";
	char* ptr = strstr(sting_in, search_timestamp);

	// assistant accountant
	uint8_t string_pos = 0;
	uint8_t string_off_set = 3;

	// angle analyzer
	for(string_pos = 0; string_pos < (sizeof(angle_out) - 1); string_pos++)
	{
		if(sting_in[(string_pos + string_off_set)] == '.')
		{
			string_pos = (string_pos - 1);
			string_off_set = (string_off_set + 1);
		}
		else
		{
			angle_out[string_pos] = sting_in[(string_pos + string_off_set)];
		}
	}

	// time analyzer
	string_off_set = 1;
	for(string_pos = 0; string_pos < (sizeof(timestamp_out) - 1); string_pos++)
	{
		if(ptr[(string_pos + string_off_set)] == '$')
		{
			break;
		}
		else
		{
			timestamp_out[string_pos] = ptr[(string_pos + string_off_set)];
		}
	}

	sscanf(angle_out, "%d", (int*)angle);
	sscanf(timestamp_out, "%d", (int*)timestamp);

	return err;
}

esp_err_t common_parser_status_to_string(pivot_config config_in,time_t timestamp, uint16_t angle, char* string_out)
{
	esp_err_t err = ESP_OK;
	char string_converted[8] = "";

	sprintf(&string_converted[COMMON_PARSER_ROTATION_POSITION], "%d", config_in.rotation);
	sprintf(&string_converted[COMMON_PARSER_WATERING_POSITION], "%d", config_in.watering_state);
	sprintf(&string_converted[COMMON_PARSER_POWER_POSITION], "%d", config_in.power_state);

	string_converted[3] = '-';
	sprintf(&string_converted[COMMON_PARSER_PERCENTIMETER_POSITION], "%d", config_in.percentimeter);
	memcpy(string_out, string_converted, (sizeof(string_converted) - 1));

	return err;
}

