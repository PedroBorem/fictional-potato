/*
 * common_parser.c
 *
 *  Created on: 21 de jun. de 2022
 *      Author: brunolima
 */

#include "common_parser.h"

/// 451-65

#define	COMMON_PARSER_POWER_POSITION				(2)
#define	COMMON_PARSER_ROTATION_POSITION				(0)
#define	COMMON_PARSER_WATERING_POSITION				(1)
#define	COMMON_PARSER_PERCENTIMETER_POSITION		(4)

#define COMMOM_PARSER_ELEMENT_NUMBER				(4)

esp_err_t common_parser_string_to_status(const char* sting_in, pivot_config *config_out)
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

esp_err_t common_parser_status_to_string(pivot_config config_in, char* string_out)
{
	esp_err_t err = ESP_ERR_INVALID_ARG;
	char* sting_out = malloc(sizeof(int));

	return err;
}


