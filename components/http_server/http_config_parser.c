/*
 * http_parser.c
 *
 *  Created on: 28 de ago de 2022
 *      Author: bruno
 */

/* Self include */
#include "http_config_parser.h"

/* Base include */
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "utils.h"

/* Public methods ------------------------------------------------ */
pivot_actions http_parser_action(char * request_body)
{
	pivot_actions actions = {};

	cJSON* action_subitem = cJSON_Parse(request_body);
	cJSON* action_power = cJSON_GetObjectItem(action_subitem, "power");

	if(action_power->valueint == true)
	{
		actions.power_state = PIVOT_ON;

		cJSON* action_water = cJSON_GetObjectItem(action_subitem, "water");
		cJSON* action_direction = cJSON_GetObjectItem(action_subitem, "direction");
		cJSON* action_percentimeter = cJSON_GetObjectItem(action_subitem, "percentimeter");

		if(action_water->valueint == true)
		{
			actions.watering_state = PIVOT_WET;
		}
		else
		{
			actions.watering_state = PIVOT_DRY;
		}

		if(strcmp(action_direction->valuestring, "ANTI_CLOCKWISE") == 0)
		{
			actions.rotation = PIVOT_CCW;
		}
		else
		{
			actions.rotation = PIVOT_CW;
		}

		actions.percentimeter = action_percentimeter->valueint;
	}
	else
	{
		actions.power_state = PIVOT_OFF;
		actions.watering_state = PIVOT_DRY;
		actions.rotation = PIVOT_CW;
		actions.percentimeter = 0;
	}

	cJSON_Delete(action_subitem);
	return actions;
}

void http_parser_action_to_json(pivot_actions action, char* out_action)
{
	char int_str[20];

	// create JSON
	cJSON* action_root = cJSON_CreateObject();

	// TODO : mock angle
	cJSON_AddItemToObject(action_root, "pivot_num", cJSON_CreateString("10"));
	cJSON_AddItemToObject(action_root, "pivot_start_angle", cJSON_CreateString("20"));
	cJSON_AddItemToObject(action_root, "pivot_end_angle", cJSON_CreateString("60"));
	cJSON_AddItemToObject(action_root, "start_angle", cJSON_CreateString("25"));
	cJSON_AddItemToObject(action_root, "end_angle", cJSON_CreateString("40"));

	// power state
	if(action.power_state == PIVOT_ON)
	{
		cJSON_AddItemToObject(action_root, "power", cJSON_CreateString("true"));
	}
	else
	{
		cJSON_AddItemToObject(action_root, "power", cJSON_CreateString("false"));
	}

	// watering state
	if(action.watering_state == PIVOT_WET)
	{
		cJSON_AddItemToObject(action_root, "water", cJSON_CreateString("true"));
	}
	else
	{
		cJSON_AddItemToObject(action_root, "water", cJSON_CreateString("false"));
	}

	// rotation
	if(action.rotation == PIVOT_CCW)
	{
		cJSON_AddItemToObject(action_root, "direction", cJSON_CreateString("ANTI_CLOCKWISE"));
	}
	else
	{
		cJSON_AddItemToObject(action_root, "direction", cJSON_CreateString("CLOCKWISE"));
	}

	memset(int_str, 0x00, sizeof(int_str));
	sprintf(int_str, "%d", action.percentimeter );
	cJSON_AddItemToObject(action_root, "percentimeter", cJSON_CreateString(int_str));

	memcpy(out_action, cJSON_Print(action_root), strlen(cJSON_Print(action_root)));
	cJSON_Delete(action_root);
}

pivot_config http_parser_config(char * request_body)
{
	pivot_config config = {};

	cJSON* subitem = cJSON_Parse(request_body);

	char* pivot_id = cJSON_GetObjectItem(subitem, "pivot_id")->valuestring;
	char* gprs_id = cJSON_GetObjectItem(subitem, "gprs_id")->valuestring;

	memcpy(&config.pivot_id, pivot_id, strlen(pivot_id));
	memcpy(&config.gprs_id, gprs_id, strlen(gprs_id));

	if(strcmp(cJSON_GetObjectItem(subitem, "contactor_type")->valuestring, "NA") == 0)
	{
		config.contactor = CONTACTOR_NA;
	}
	else
	{
		config.contactor = CONTACTOR_NF;
	}

	if(strcmp(cJSON_GetObjectItem(subitem, "pressure_type")->valuestring, "NA") == 0)
	{
		config.contactor = PRESSURE_SWITCH_NA;
	}
	else
	{
		config.contactor = PRESSURE_SWITCH_NF;
	}

	config.pressurization_time = cJSON_GetObjectItem(subitem, "pressure_time")->valueint;
	config.on_off_time = cJSON_GetObjectItem(subitem, "turn_on_time")->valueint;

	cJSON_Delete(subitem);

	return config;
}

void http_parser_config_to_json(pivot_config config, char* out_config)
{
	char int_str[20];

	// create JSON
	cJSON* config_root = cJSON_CreateObject();

	cJSON_AddItemToObject(config_root, "pivot_id", cJSON_CreateString(config.pivot_id));
	cJSON_AddItemToObject(config_root, "gprs_id", cJSON_CreateString(config.gprs_id));

	if(config.contactor == CONTACTOR_NA)
	{
		cJSON_AddItemToObject(config_root, "contactor_type", cJSON_CreateString("NA"));
	}
	else
	{
		cJSON_AddItemToObject(config_root, "contactor_type", cJSON_CreateString("NF"));
	}

	if(config.pressure_switch == PRESSURE_SWITCH_NA )
	{
		cJSON_AddItemToObject(config_root, "pressure_type", cJSON_CreateString("NA"));
	}
	else
	{
		cJSON_AddItemToObject(config_root, "pressure_type", cJSON_CreateString("NF"));
	}

	sprintf(int_str, "%d", config.pressurization_time );
	cJSON_AddItemToObject(config_root, "pressure_time", cJSON_CreateString(int_str));

	memset(int_str, 0x00, sizeof(int_str));
	sprintf(int_str, "%d", config.on_off_time );
	cJSON_AddItemToObject(config_root, "turn_on_time", cJSON_CreateString(int_str));

	memcpy(out_config, cJSON_Print(config_root), strlen(cJSON_Print(config_root)));
	cJSON_Delete(config_root);
}
