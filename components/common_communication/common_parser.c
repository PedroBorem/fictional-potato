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
/**
 *	Power attribute position in string (in/out)
 *
 */
#define	COMMON_PARSER_POWER_POSITION				(2)

/**
 *	Rotation attribute position in string (in/out)
 *
 */
#define	COMMON_PARSER_ROTATION_POSITION				(0)

/**
 *	Watering attribute position in string (in/out)
 *
 */
#define	COMMON_PARSER_WATERING_POSITION				(1)

/**
 *	Percentimeter attribute position in string (in/out)
 *
 */
#define	COMMON_PARSER_PERCENTIMETER_POSITION		(4)

/**
 *	Angle attribute output buffer size
 *
 */
#define COMMON_PARSER_ANGLE_SIZE					(4)

/**
 *	Angle attribute position in string (in/out)
 *
 */
#define	COMMON_PARSER_ANGLE_POSITION				(3)

/**
 *	Angle attribute position out string (in/out)
 *
 */
#define	COMMON_PARSER_ANGLE_OUT_POSITION			(8)

/**
 *	Timestamp attribute output buffer size
 *
 */
#define COMMON_PARSER_TIMESTAMP_SIZE				(11)

/**
 *	Timestamp attribute position in string (in/out)
 *
 */
#define COMMON_PARSER_TIMESTAMP_POSITION			(1)

/**
 *	Timestamp attribute position out string (in/out)
 *
 */
#define COMMON_PARSER_TIMESTAMP_OUT_POSITION		(12)

/**
 *	Number of elements in a configuration string
 *
 */
#define COMMOM_PARSER_ELEMENT_NUMBER				(4)

/* Public methods ------------------------------------------------ */
esp_err_t common_parser_string_to_config(const char* string_in, pivot_config* config_out)
{
	esp_err_t err = ESP_ERR_INVALID_ARG;
	pivot_config config = {};
	char status;
	int validate_ret = 0;

	status = string_in[COMMON_PARSER_POWER_POSITION];
	validate_ret += sscanf(&status, "%d",(int*)&config.power_state);

	status = string_in[COMMON_PARSER_ROTATION_POSITION];
	validate_ret += sscanf(&status, "%d",(int*)&config.rotation);

	status = string_in[COMMON_PARSER_WATERING_POSITION];
	validate_ret += sscanf(&status, "%d",(int*)&config.watering_state);

	validate_ret += sscanf(&string_in[COMMON_PARSER_PERCENTIMETER_POSITION], "%d", (int*)&config.percentimeter);

	if(validate_ret >= COMMOM_PARSER_ELEMENT_NUMBER)
	{
		memcpy(config_out, &config, sizeof(pivot_config));
		err = ESP_OK;
	}

	return err;
}

esp_err_t common_parser_string_to_gnss(const char* string_in, uint16_t* angle, time_t* timestamp)
{
	esp_err_t err = ESP_OK;

	// assistant buffers
	char angle_out[COMMON_PARSER_ANGLE_SIZE] = "";
	char timestamp_out[COMMON_PARSER_TIMESTAMP_SIZE] = "";
	char search_timestamp[] = "#";
	char* ptr = strstr(string_in, search_timestamp);

	// assistant accountant
	uint8_t string_pos = 0;
	uint8_t string_off_set = COMMON_PARSER_ANGLE_POSITION;

	// angle analyzer
	for(string_pos = 0; string_pos < (sizeof(angle_out) - 1); string_pos++)
	{
		if(string_in[(string_pos + string_off_set)] == '.')
		{
			break;
		}
		else
		{
			angle_out[string_pos] = string_in[(string_pos + string_off_set)];
		}
	}

	// time analyzer
	string_off_set = COMMON_PARSER_TIMESTAMP_POSITION;
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
	char string_converted[25] = "";

	sprintf(&string_converted[COMMON_PARSER_ROTATION_POSITION], "%d", config_in.rotation);
	sprintf(&string_converted[COMMON_PARSER_WATERING_POSITION], "%d", config_in.watering_state);
	sprintf(&string_converted[COMMON_PARSER_POWER_POSITION], "%d", config_in.power_state);

	string_converted[3] = '-';
	sprintf(&string_converted[COMMON_PARSER_PERCENTIMETER_POSITION], "%03d", config_in.percentimeter);

	string_converted[7] = '-';
	sprintf(&string_converted[COMMON_PARSER_ANGLE_OUT_POSITION], "%03d", angle);

	string_converted[11] = '-';
	sprintf(&string_converted[COMMON_PARSER_TIMESTAMP_OUT_POSITION], "%lld", (long long int)timestamp);

	memcpy(string_out, string_converted, (sizeof(string_converted) - 1));


	ESP_LOGW("aaaa", "%s", string_out);

	return err;
}

/**@}*/ 	//common_parser
/** @}*/	//components
