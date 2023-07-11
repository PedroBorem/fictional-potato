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
#include "cJSON.h"

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
esp_err_t common_parser_string_to_action(const char* string_in, pivot_actions* action_out)
{
	esp_err_t err = ESP_ERR_INVALID_ARG;
	pivot_actions config = {};
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
		memcpy(action_out, &config, sizeof(pivot_actions));
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
	char search_timestamp[] = "-";
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

	sscanf(angle_out, "%d", (int*)angle);

	// time analyzer
	if(ptr != NULL)
	{
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

		sscanf(timestamp_out, "%d", (int*)timestamp);
	}
	else
	{
		*timestamp=0;
	}

	return err;
}

esp_err_t common_parser_status_to_string(pivot_actions config_in,time_t timestamp, uint16_t angle, char* string_out)
{
	esp_err_t err = ESP_OK;
	char string_converted[25] = "";

        string_converted[0] = '#';
	sprintf(&string_converted[1], "%d", config_in.rotation);
	sprintf(&string_converted[2], "%d", config_in.watering_state);
	sprintf(&string_converted[3], "%d", config_in.power_state);

	string_converted[4] = '-';
	sprintf(&string_converted[5], "%03d", config_in.percentimeter);

	string_converted[8] = '-';
	sprintf(&string_converted[9], "%03d", angle);

	string_converted[12] = '-';
	sprintf(&string_converted[13], "%010lld", (long long int)timestamp);

	string_converted[23] = '$';
	memcpy(string_out, string_converted, (sizeof(string_converted) - 1));

	return err;
}

esp_err_t common_parser_json_to_timestamp(const char* string_in, time_t* timestamp)
{
	esp_err_t err = ESP_OK;

	// assistant buffers
	char timestamp_out[COMMON_PARSER_TIMESTAMP_SIZE] = "";

	// assistant accountant
	uint8_t string_pos = 0;

	// angle analyzer
	for(string_pos = 0; string_pos < (sizeof(timestamp_out) - 1); string_pos++)
	{
		if(string_in[(string_pos)] == '}')
		{
			break;
		}
		else
		{
			timestamp_out[string_pos] = string_in[(string_pos)];
		}
	}

	sscanf(timestamp_out, "%d", (int*)timestamp);

	return err;
}

esp_err_t common_parser_status_to_json(pivot_actions config_in,time_t timestamp, uint16_t angle, const char* id, char* string_out, size_t string_size)
{
	// convert status
	char string_status[25] = "";
	common_parser_status_to_string(config_in, timestamp, angle, string_status);

	// create JSON
	cJSON *root =  cJSON_CreateObject();
	cJSON_AddItemToObject(root, "type", cJSON_CreateString("status"));
	cJSON_AddItemToObject(root, "id", cJSON_CreateString(id));
	cJSON_AddItemToObject(root, "payload", cJSON_CreateString(string_status));

	memcpy(string_out, cJSON_Print(root), string_size);
	cJSON_Delete(root);

	return ESP_OK;
}

esp_err_t common_parser_register_to_json(const char* id, char* string_out, size_t string_size)
{
	// create JSON
	cJSON *root =  cJSON_CreateObject();
	cJSON_AddItemToObject(root, "register", cJSON_CreateString("True"));
	cJSON_AddItemToObject(root, "id", cJSON_CreateString(id));

	memcpy(string_out, cJSON_Print(root), string_size);
	cJSON_Delete(root);

	return ESP_OK;
}

idp_type common_parser_get_idp(const char* string_in)
{
	idp_type idp_ret;
	char pack_delimiter = string_in[0];
	char idp_value = string_in[1];
	char idp_delimiter = string_in[2];

	if(idp_value >= '0' && idp_value <= '9'
	&& idp_delimiter == '-' && pack_delimiter == '#')
	{
		idp_ret = idp_value - '0';
	}
	else
	{
		idp_ret = IDP_INVALID;
	}

	return idp_ret;
}

esp_err_t common_parser_string_to_scheaduling_date(char* string_in, pivot_scheduling_date* scheduling_out)
{
	char status;
	char delim[] = "-";

	idp_type idp = common_parser_get_idp(string_in);
	char *ptr = strtok(string_in, delim);

	if(idp == IDP_2)
	{
		// TS1
		ptr = strtok(NULL, delim);
		sscanf(ptr, "%lld",(time_t*)&scheduling_out->start_date);

		// TS2
		ptr = strtok(NULL, delim);
		sscanf(ptr, "%lld",(time_t*)&scheduling_out->end_date);

		//DWP
		ptr = strtok(NULL, delim);

		status = ptr[0];
		scheduling_out->acionts.rotation = status - '0';

		status = ptr[1];
		scheduling_out->acionts.watering_state = status - '0';

		status = ptr[2];
		scheduling_out->acionts.power_state = status - '0';

		//Percent
		ptr = strtok(NULL, delim);
		ptr[(strlen(ptr) - 1)] = 0x00;
		sscanf(ptr, "%d", (int*)&scheduling_out->acionts.percentimeter);
	}
	else if(idp == IDP_4 )
	{
		// TS1
		ptr = strtok(NULL, delim);
		ptr[(strlen(ptr) - 1)] = 0x00;
		sscanf(ptr, "%lld",(time_t*)&scheduling_out->start_date);

		// TS2
		scheduling_out->end_date = scheduling_out->start_date + 30;

		//DWP
		scheduling_out->acionts.rotation = PIVOT_CCW;
		scheduling_out->acionts.watering_state = PIVOT_DRY;
		scheduling_out->acionts.power_state = PIVOT_OFF;

		scheduling_out->acionts.percentimeter = 0;
	}
	else if(idp == IDP_6)
	{
		// SCHEADULING ID
		ptr = strtok(NULL, delim);
		ptr[(strlen(ptr) - 1)] = 0x00;

		memcpy(scheduling_out->scheduling_id, ptr, strlen(ptr));
	}

	return ESP_OK;
}

esp_err_t common_parser_string_to_scheaduling_angle(char* string_in, pivot_scheduling_angle* scheduling_out)
{
	char status;
	char delim[] = "-";

	idp_type idp = common_parser_get_idp(string_in);
	char *ptr = strtok(string_in, delim);

	if(idp == IDP_3)
	{
		// TS1
		ptr = strtok(NULL, delim);
		sscanf(ptr, "%lld",(time_t*)&scheduling_out->start_date);

		// TS2
		ptr = strtok(NULL, delim);
		sscanf(ptr, "%d",(int*)&scheduling_out->end_angle);

		//DWP
		ptr = strtok(NULL, delim);

		status = ptr[0];
		scheduling_out->acionts.rotation = status - '0';

		status = ptr[1];
		scheduling_out->acionts.watering_state = status - '0';

		status = ptr[2];
		scheduling_out->acionts.power_state = status - '0';

		//Percent
		ptr = strtok(NULL, delim);
		ptr[(strlen(ptr) - 1)] = 0x00;
		sscanf(ptr, "%d", (int*)&scheduling_out->acionts.percentimeter);
	}
	else if(idp == IDP_5)
	{
		// não implementado
	}

	return ESP_OK;
}

esp_err_t common_parser_ipm_resp(idp_type idp, char* pivo_id, char* scheaduling_id, char* resp_out)
{
	esp_err_t err = ESP_OK;
	char string_converted[50] = {};
	size_t size_str = strlen(pivo_id);

    string_converted[0] = '#';
	sprintf(&string_converted[1], "%d", idp);
	string_converted[2] = '-';

	memcpy(&string_converted[3], pivo_id, size_str);
	string_converted[(size_str + 3)] = '-';

	memcpy(&string_converted[(size_str + 4)], scheaduling_id, strlen(scheaduling_id));
	string_converted[(size_str + 4 + strlen(scheaduling_id))] = '$';

	memcpy(resp_out, string_converted, strlen(string_converted));
	return err;
}

esp_err_t common_parser_scheaduling_date_http_to_mqtt(idp_type idp, char * gprs_id, pivot_scheduling_date scheduling_in, char* string_out)
{
	esp_err_t err = ESP_OK;
	char string_converted[100] = {};
	char string_timestamp[30] = {};
	size_t size_str = strlen(gprs_id);

    string_converted[0] = '#';
	string_converted[1] = idp + '0';
	string_converted[2] = '-';

	memcpy(&string_converted[3], gprs_id, size_str);
	size_str = size_str + 3;
	string_converted[(size_str)] = '-';

	if(idp == IDP_2)
	{
		size_str = size_str + 1;
		memcpy(&string_converted[size_str], scheduling_in.scheduling_id, strlen(scheduling_in.scheduling_id));
		size_str = size_str + strlen(scheduling_in.scheduling_id);
		string_converted[(size_str)] = '-';

		size_str = size_str + 1;
		sprintf(string_timestamp, "%lld", scheduling_in.start_date);
		memcpy(&string_converted[size_str], string_timestamp, strlen(string_timestamp));

		size_str = size_str + strlen(string_timestamp);
		string_converted[(size_str)] = '-';
		size_str = size_str + 1;

		sprintf(string_timestamp, "%lld", scheduling_in.end_date);
		memcpy(&string_converted[size_str], string_timestamp, strlen(string_timestamp));

		size_str = size_str + strlen(string_timestamp);
		string_converted[(size_str)] = '-';
		size_str = size_str + 1;

		string_converted[size_str] = scheduling_in.acionts.rotation + '0';
		size_str = size_str + 1;

		string_converted[size_str] = scheduling_in.acionts.watering_state + '0';
		size_str = size_str + 1;

		string_converted[size_str] = scheduling_in.acionts.power_state + '0';
		size_str = size_str + 1;

		string_converted[(size_str)] = '-';
		size_str = size_str + 1;

		sprintf(string_timestamp, "%03d", scheduling_in.acionts.percentimeter);
		memcpy(&string_converted[size_str], string_timestamp, strlen(string_timestamp));

		size_str = size_str + strlen(string_timestamp);
		string_converted[(size_str)] = '$';
	}
	else if(idp == IDP_4)
	{
		size_str = size_str + 1;
		memcpy(&string_converted[size_str], scheduling_in.scheduling_id, strlen(scheduling_in.scheduling_id));
		size_str = size_str + strlen(scheduling_in.scheduling_id);
		string_converted[(size_str)] = '-';

		size_str = size_str + 1;
		sprintf(string_timestamp, "%lld", scheduling_in.start_date);
		memcpy(&string_converted[size_str], string_timestamp, strlen(string_timestamp));

		size_str = size_str + strlen(string_timestamp);
		string_converted[(size_str)] = '$';
	}
	else
	{
		size_str = size_str + 1;
		memcpy(&string_converted[size_str], scheduling_in.scheduling_id, strlen(scheduling_in.scheduling_id));
		size_str = size_str + strlen(scheduling_in.scheduling_id);
		string_converted[(size_str)] = '$';
	}

	memcpy(string_out, string_converted, strlen(string_converted));

	return err;
}

esp_err_t common_parser_scheaduling_angle_http_to_mqtt(idp_type idp, char * gprs_id, pivot_scheduling_angle scheduling_in, char* string_out)
{
	esp_err_t err = ESP_OK;
	char string_converted[100] = {};
	char string_timestamp[30] = {};
	size_t size_str = strlen(gprs_id);

    string_converted[0] = '#';
    string_converted[1] = idp + '0';
	string_converted[2] = '-';

	memcpy(&string_converted[3], gprs_id, size_str);
	size_str = size_str + 3;
	string_converted[(size_str)] = '-';

	if(idp == IDP_3)
	{

		size_str = size_str + 1;
		memcpy(&string_converted[size_str], scheduling_in.scheduling_id, strlen(scheduling_in.scheduling_id));
		size_str = size_str + strlen(scheduling_in.scheduling_id);
		string_converted[(size_str)] = '-';

		size_str = size_str + 1;
		sprintf(string_timestamp, "%lld", scheduling_in.start_date);
		memcpy(&string_converted[size_str], string_timestamp, strlen(string_timestamp));

		size_str = size_str + strlen(string_timestamp);
		string_converted[(size_str)] = '-';
		size_str = size_str + 1;

		sprintf(string_timestamp, "%d", scheduling_in.end_angle);
		memcpy(&string_converted[size_str], string_timestamp, strlen(string_timestamp));

		size_str = size_str + strlen(string_timestamp);
		string_converted[(size_str)] = '-';
		size_str = size_str + 1;

		string_converted[size_str] = scheduling_in.acionts.rotation + '0';
		size_str = size_str + 1;

		string_converted[size_str] = scheduling_in.acionts.watering_state + '0';
		size_str = size_str + 1;

		string_converted[size_str] = scheduling_in.acionts.power_state + '0';
		size_str = size_str + 1;

		string_converted[(size_str)] = '-';
		size_str = size_str + 1;

		sprintf(string_timestamp, "%03d", scheduling_in.acionts.percentimeter);
		memcpy(&string_converted[size_str], string_timestamp, strlen(string_timestamp));

		size_str = size_str + strlen(string_timestamp);
		string_converted[(size_str)] = '$';
	}
	else
	{
		size_str = size_str + 1;
		memcpy(&string_converted[size_str], scheduling_in.scheduling_id, strlen(scheduling_in.scheduling_id));
		size_str = size_str + strlen(scheduling_in.scheduling_id);
		string_converted[(size_str)] = '$';
	}

	memcpy(string_out, string_converted, strlen(string_converted));

	return err;
}

/**@}*/ 	//common_parser
/** @}*/	//components
