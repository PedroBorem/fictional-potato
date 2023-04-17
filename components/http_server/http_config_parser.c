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

void http_parser_action_to_json(const pivot_actions action, const pivot_config config, uint16_t start_angle, uint16_t end_angle, char* out_action)
{
	char int_str[20];

	// create JSON
	cJSON* action_root = cJSON_CreateObject();

	// pivot ID
	cJSON_AddItemToObject(action_root, "pivot_num", cJSON_CreateString(config.pivot_id));

	// coverage Angle
	cJSON_AddItemToObject(action_root, "pivot_start_angle", cJSON_CreateString("0")); // TODO : mock angle
	cJSON_AddItemToObject(action_root, "pivot_end_angle", cJSON_CreateString("360")); // TODO : mock angle

	// current angles
	memset(int_str, 0x00, sizeof(int_str));
	sprintf(int_str, "%d", start_angle );
	cJSON_AddItemToObject(action_root, "start_angle", cJSON_CreateString(int_str));

	memset(int_str, 0x00, sizeof(int_str));
	sprintf(int_str, "%d", end_angle );
	cJSON_AddItemToObject(action_root, "end_angle", cJSON_CreateString(int_str));

	// ECO mode
	if(config.eco_mode == true)
	{
		cJSON_AddItemToObject(action_root, "eco", cJSON_CreateString("true"));
	}
	else
	{
		cJSON_AddItemToObject(action_root, "eco", cJSON_CreateString("false"));
	}

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

	// percent
	memset(int_str, 0x00, sizeof(int_str));
	sprintf(int_str, "%d", action.percentimeter );
	cJSON_AddItemToObject(action_root, "percentimeter", cJSON_CreateString(int_str));

	// output vector
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

	config.pressurization_time = (uint16_t)cJSON_GetObjectItem(subitem, "pressure_time")->valueint;
	config.on_off_time = (uint8_t)cJSON_GetObjectItem(subitem, "turn_on_time")->valueint;

	config.eco_mode = (bool)cJSON_GetObjectItem(subitem, "eco_mode")->valueint;
	if(config.eco_mode == true)
	{
		config.start_time = (time_t)cJSON_GetObjectItem(subitem, "eco_mode_start_time")->valueint;
		config.end_time = (time_t)cJSON_GetObjectItem(subitem, "eco_mode_end_time")->valueint;
	}

	config.sector_enabled = (bool)cJSON_GetObjectItem(subitem, "sector_enabled")->valueint;
	if(config.sector_enabled == true)
	{
		cJSON * sectors = cJSON_GetObjectItem(subitem,"sectors");

		for (uint8_t index = 0 ; index < cJSON_GetArraySize(sectors) ; index++)
		{
			cJSON * sectors_array = cJSON_GetArrayItem(sectors, index);

			config.sectors[(index)].id = (uint8_t)cJSON_GetObjectItem(sectors_array, "id")->valueint;
			config.sectors[(index)].start_angle = (uint16_t)cJSON_GetObjectItem(sectors_array, "start_angle")->valueint;
			config.sectors[(index)].end_angle = (uint16_t)cJSON_GetObjectItem(sectors_array, "end_angle")->valueint;
		}
	}

	cJSON_Delete(subitem);

	return config;
}

void http_parser_config_to_json(pivot_config config, char* out_config)
{
	char int_str[20];
	uint8_t sectors_indice = 0;

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

	if(config.eco_mode == true)
	{
		cJSON_AddItemToObject(config_root, "eco_mode", cJSON_CreateString("true"));

		memset(int_str, 0x00, sizeof(int_str));
		sprintf(int_str, "%lld", config.start_time );
		cJSON_AddItemToObject(config_root, "eco_mode_start_time", cJSON_CreateString(int_str));

		memset(int_str, 0x00, sizeof(int_str));
		sprintf(int_str, "%lld", config.end_time );
		cJSON_AddItemToObject(config_root, "eco_mode_end_time", cJSON_CreateString(int_str));
	}
	else
	{
		cJSON_AddItemToObject(config_root, "eco_mode", cJSON_CreateString("false"));
		cJSON_AddItemToObject(config_root, "eco_mode_start_time", cJSON_CreateString("0"));
		cJSON_AddItemToObject(config_root, "eco_mode_end_time", cJSON_CreateString("0"));
	}

	if(config.sector_enabled == true)
	{
		cJSON_AddItemToObject(config_root, "sector_enabled", cJSON_CreateString("true"));

		cJSON* array_sectors =  cJSON_CreateArray();
		cJSON* sectors;

		cJSON_AddItemToObject(config_root, "sectors", array_sectors);

		uint8_t size_sectors = 4; //TODO: tamanho maximo de setores
		for(sectors_indice = 0; sectors_indice < size_sectors; sectors_indice++)
		{
			if(config.sectors[sectors_indice].start_angle != 0 && config.sectors[sectors_indice].end_angle != 0)
			{
				cJSON_AddItemToArray(array_sectors, sectors = cJSON_CreateObject());

				memset(int_str, 0x00, sizeof(int_str));
				sprintf(int_str, "%d",config.sectors[sectors_indice].id );
				cJSON_AddItemToObject(sectors, "id", cJSON_CreateString(int_str));

				memset(int_str, 0x00, sizeof(int_str));
				sprintf(int_str, "%d",config.sectors[sectors_indice].start_angle );
				cJSON_AddItemToObject(sectors, "start_angle", cJSON_CreateString(int_str));

				memset(int_str, 0x00, sizeof(int_str));
				sprintf(int_str, "%d",config.sectors[sectors_indice].end_angle );
				cJSON_AddItemToObject(sectors, "end_angle", cJSON_CreateString(int_str));
			}
		}
	}
	else
	{
		cJSON_AddItemToObject(config_root, "sector_enabled", cJSON_CreateString("false"));
	}

	memcpy(out_config, cJSON_Print(config_root), strlen(cJSON_Print(config_root)));
	cJSON_Delete(config_root);
}

pivot_scheduling_date http_parser_scheduling_date(char* request_body)
{
	pivot_scheduling_date scheduling_date = {};

	cJSON* subitem = cJSON_Parse(request_body);

	char* scheduling_id = "7"; // TODO receber esse valor
	memcpy(&scheduling_date.scheduling_id, scheduling_id, strlen(scheduling_id));

	// get scheduling
	scheduling_date.is_stop = (bool)cJSON_GetObjectItem(subitem, "is_stop")->valueint;
	scheduling_date.start_date = (time_t)cJSON_GetObjectItem(subitem, "start_date")->valueint;
	scheduling_date.end_date = (time_t)cJSON_GetObjectItem(subitem, "end_date")->valueint;

	printf("scheduling_date[position].start_date %lld \n", scheduling_date.start_date);
	printf("scheduling_date[position].start_date %lld \n", scheduling_date.end_date);

	cJSON_Delete(subitem);

	// get actions
	scheduling_date.acionts = http_parser_action(request_body);

	return scheduling_date;
}

void http_parser_scheduling_date_to_json(pivot_scheduling_date* scheduling_date, char* out_scheduling)
{
	// create JSON
	cJSON* scheduling_date_array = cJSON_CreateArray();
	cJSON* scheduling_date_obj;
	char int_str[20] = {};

	for(uint8_t position = 0; position < SCHEDULING_MAX_VALUE; position ++)
	{
		if(strcmp(scheduling_date[position].scheduling_id,"") > 0)
		{
			cJSON_AddItemToArray(scheduling_date_array, scheduling_date_obj = cJSON_CreateObject());
			cJSON_AddItemToObject(scheduling_date_obj, "scheduling_id", cJSON_CreateString(scheduling_date[position].scheduling_id));

			if(scheduling_date[position].is_stop == true)
			{
				cJSON_AddItemToObject(scheduling_date_obj, "is_stop", cJSON_CreateString("true"));
			}
			else
			{
				cJSON_AddItemToObject(scheduling_date_obj, "is_stop", cJSON_CreateString("false"));
			}

			if(scheduling_date[position].is_running == true)
			{
				cJSON_AddItemToObject(scheduling_date_obj, "is_running", cJSON_CreateString("true"));
			}
			else
			{
				cJSON_AddItemToObject(scheduling_date_obj, "is_running", cJSON_CreateString("false"));
			}

			memset(int_str, 0x00, sizeof(int_str));
			sprintf(int_str, "%lld",scheduling_date[position].start_date);
			cJSON_AddItemToObject(scheduling_date_obj, "start_date", cJSON_CreateString(int_str));

			memset(int_str, 0x00, sizeof(int_str));
			sprintf(int_str, "%lld",scheduling_date[position].end_date);
			cJSON_AddItemToObject(scheduling_date_obj, "end_date", cJSON_CreateString(int_str));

			// power state
			if(scheduling_date[position].acionts.power_state == PIVOT_ON)
			{
				cJSON_AddItemToObject(scheduling_date_obj, "power", cJSON_CreateString("true"));
			}
			else
			{
				cJSON_AddItemToObject(scheduling_date_obj, "power", cJSON_CreateString("false"));
			}

			// watering state
			if(scheduling_date[position].acionts.watering_state == PIVOT_WET)
			{
				cJSON_AddItemToObject(scheduling_date_obj, "water", cJSON_CreateString("true"));
			}
			else
			{
				cJSON_AddItemToObject(scheduling_date_obj, "water", cJSON_CreateString("false"));
			}

			// rotation
			if(scheduling_date[position].acionts.rotation == PIVOT_CCW)
			{
				cJSON_AddItemToObject(scheduling_date_obj, "direction", cJSON_CreateString("ANTI_CLOCKWISE"));
			}
			else
			{
				cJSON_AddItemToObject(scheduling_date_obj, "direction", cJSON_CreateString("CLOCKWISE"));
			}

			// percent
			memset(int_str, 0x00, sizeof(int_str));
			sprintf(int_str, "%d", scheduling_date[position].acionts.percentimeter );
			cJSON_AddItemToObject(scheduling_date_obj, "percentimeter", cJSON_CreateString(int_str));
		}
	}

	memcpy(out_scheduling, cJSON_Print(scheduling_date_array), strlen(cJSON_Print(scheduling_date_array)));
	cJSON_Delete(scheduling_date_array);
}

pivot_scheduling_angle http_parser_scheduling_angle(char* request_body)
{
	pivot_scheduling_angle scheduling_angle = {};

	cJSON* subitem = cJSON_Parse(request_body);

	char* scheduling_id = "7"; // TODO receber esse valor
	memcpy(&scheduling_angle.scheduling_id, scheduling_id, strlen(scheduling_id));

	// get scheduling
	scheduling_angle.is_return = (bool)cJSON_GetObjectItem(subitem, "is_return")->valueint;
	scheduling_angle.start_angle = (uint16_t)cJSON_GetObjectItem(subitem, "start_angle")->valueint;
	scheduling_angle.end_angle = (uint16_t)cJSON_GetObjectItem(subitem, "end_angle")->valueint;
	scheduling_angle.start_date = (time_t)cJSON_GetObjectItem(subitem, "start_date")->valueint;
	cJSON_Delete(subitem);

	// get actions
	scheduling_angle.acionts = http_parser_action(request_body);

	return scheduling_angle;
}

void http_parser_scheduling_angle_to_json(pivot_scheduling_angle* scheduling_angle, char* out_scheduling)
{
	// create JSON
	cJSON* scheduling_angle_array = cJSON_CreateArray();
	cJSON* scheduling_angle_obj;
	char int_str[20] = {};

	for(uint8_t position = 0; position < SCHEDULING_MAX_VALUE; position ++)
	{
		if(strcmp(scheduling_angle[position].scheduling_id,"") > 0)
		{
			cJSON_AddItemToArray(scheduling_angle_array, scheduling_angle_obj = cJSON_CreateObject());
			cJSON_AddItemToObject(scheduling_angle_obj, "scheduling_id", cJSON_CreateString(scheduling_angle[position].scheduling_id));

			if(scheduling_angle[position].is_return == true)
			{
				cJSON_AddItemToObject(scheduling_angle_obj, "is_stop", cJSON_CreateString("true"));
			}
			else
			{
				cJSON_AddItemToObject(scheduling_angle_obj, "is_stop", cJSON_CreateString("false"));
			}

			if(scheduling_angle[position].is_running == true)
			{
				cJSON_AddItemToObject(scheduling_angle_obj, "is_running", cJSON_CreateString("true"));
			}
			else
			{
				cJSON_AddItemToObject(scheduling_angle_obj, "is_running", cJSON_CreateString("false"));
			}

			memset(int_str, 0x00, sizeof(int_str));
			sprintf(int_str, "%lld",scheduling_angle[position].start_date);
			cJSON_AddItemToObject(scheduling_angle_obj, "start_date", cJSON_CreateString(int_str));

			memset(int_str, 0x00, sizeof(int_str));
			sprintf(int_str, "%d",scheduling_angle[position].start_angle);
			cJSON_AddItemToObject(scheduling_angle_obj, "start_angle", cJSON_CreateString(int_str));

			memset(int_str, 0x00, sizeof(int_str));
			sprintf(int_str, "%d",scheduling_angle[position].end_angle);
			cJSON_AddItemToObject(scheduling_angle_obj, "end_angle", cJSON_CreateString(int_str));

			// power state
			if(scheduling_angle[position].acionts.power_state == PIVOT_ON)
			{
				cJSON_AddItemToObject(scheduling_angle_obj, "power", cJSON_CreateString("true"));
			}
			else
			{
				cJSON_AddItemToObject(scheduling_angle_obj, "power", cJSON_CreateString("false"));
			}

			// watering state
			if(scheduling_angle[position].acionts.watering_state == PIVOT_WET)
			{
				cJSON_AddItemToObject(scheduling_angle_obj, "water", cJSON_CreateString("true"));
			}
			else
			{
				cJSON_AddItemToObject(scheduling_angle_obj, "water", cJSON_CreateString("false"));
			}

			// rotation
			if(scheduling_angle[position].acionts.rotation == PIVOT_CCW)
			{
				cJSON_AddItemToObject(scheduling_angle_obj, "direction", cJSON_CreateString("ANTI_CLOCKWISE"));
			}
			else
			{
				cJSON_AddItemToObject(scheduling_angle_obj, "direction", cJSON_CreateString("CLOCKWISE"));
			}

			// percent
			memset(int_str, 0x00, sizeof(int_str));
			sprintf(int_str, "%d", scheduling_angle[position].acionts.percentimeter );
			cJSON_AddItemToObject(scheduling_angle_obj, "percentimeter", cJSON_CreateString(int_str));
		}
	}

	memcpy(out_scheduling, cJSON_Print(scheduling_angle_array), strlen(cJSON_Print(scheduling_angle_array)));
	cJSON_Delete(scheduling_angle_array);
}

void http_parser_cycles_to_json(char* out_cycles)
{
	// create JSON
	cJSON* cycles_root = cJSON_CreateObject();
	cJSON* cycles_array = cJSON_CreateArray();

	cJSON_AddItemToObject(cycles_root, "is_running", cJSON_CreateString("false"));
	cJSON_AddItemToObject(cycles_root, "start_date", cJSON_CreateString("1678935600"));
	cJSON_AddItemToObject(cycles_root, "end_date", cJSON_CreateString("1678935600"));
	cJSON_AddItemToObject(cycles_root, "start_angle", cJSON_CreateString("30"));
	cJSON_AddItemToObject(cycles_root, "end_angle", cJSON_CreateString("60"));
	cJSON_AddItemToObject(cycles_root, "power", cJSON_CreateString("true"));
	cJSON_AddItemToObject(cycles_root, "water", cJSON_CreateString("true"));
	cJSON_AddItemToObject(cycles_root, "direction", cJSON_CreateString("CLOCKWISE"));
	cJSON_AddItemToObject(cycles_root, "percentimeter", cJSON_CreateString("50"));

	cJSON_AddItemToArray(cycles_array, cycles_root);

	memcpy(out_cycles, cJSON_Print(cycles_array), strlen(cJSON_Print(cycles_array)));
	cJSON_Delete(cycles_array);
}
