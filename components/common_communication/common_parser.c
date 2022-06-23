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
	esp_err_t err = ESP_ERR_INVALID_ARG;

	//GPS179.31-17:51:12#1655920272$‹/



	return err;
}

esp_err_t common_parser_status_to_string(pivot_config config_in,time_t timestamp, uint16_t angle, char* string_out)
{
	esp_err_t err = ESP_ERR_INVALID_ARG;
	char string_converted[8] = "";

	sprintf(&string_converted[COMMON_PARSER_ROTATION_POSITION], "%d", config_in.rotation);
	sprintf(&string_converted[COMMON_PARSER_WATERING_POSITION], "%d", config_in.watering_state);
	sprintf(&string_converted[COMMON_PARSER_POWER_POSITION], "%d", config_in.power_state);

	string_converted[3] = '-';
	sprintf(&string_converted[COMMON_PARSER_PERCENTIMETER_POSITION], "%d", config_in.percentimeter);
	memcpy(string_out, string_converted, sizeof(string_converted) - 1);

	//TODO: added angle and timestamp
	return err;
}

