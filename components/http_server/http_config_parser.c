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

/* Private methods - prototypes ---------------------------------- */
/**
 * @brief	Method fetches and makes adjustments to escape characters.
 * @param	raw_value[in] - String of raw HTTP field value
 * @return	Allocated string without HTTP escaping characters
 */
char *http_remove_escape_chr(char* raw_value);

/* Public methods ------------------------------------------------ */
char * http_config_parser(const char* received_post, const char* field_name)
{
    char *field_value_string = NULL;
    char *field_name_http_expr = strdup(field_name);
    field_name_http_expr = dyn_strcat(field_name_http_expr, "=");

    if (field_name_http_expr != NULL) {
        // Find field value if it exists
        char *start_value = strstr(received_post, field_name_http_expr);
        if (start_value != NULL) {
            start_value += strlen(field_name_http_expr);

            // Fields are separated by '&' but last does not have '&'
            char *end_value = strstr(start_value, "&");
            if (end_value == NULL) {
                end_value = start_value + strlen(start_value);
            }

            // Raw value crop
            int raw_value_strlen = end_value - start_value;
            char *field_value_raw = (char *) malloc(raw_value_strlen + 1);
            if (field_value_raw != NULL) {
                field_value_raw[raw_value_strlen] = '\0';
                memcpy(field_value_raw, start_value, raw_value_strlen);

                // Remove HTTP escape chars to final output
                field_value_string = http_remove_escape_chr(field_value_raw);
                free(field_value_raw);
            }
        }

        free(field_name_http_expr);
    }

    return field_value_string;
}

char *http_remove_escape_chr(char* raw_value)
{
    int len_content = strlen(raw_value);
    char* occurrence = NULL;
    char *output = (char *) malloc(len_content + 1);

    if (output != NULL) {
        output[len_content] = '\0';
        memcpy(output, raw_value, len_content);

        for (int i = 0; i < len_content; i++) {
            if (output[i] == '+') {
                output[i] = ' ';
            }
        }

        while ((occurrence = strchr(output, '%'))) {
            char hex_chr[3] = {'\0', '\0', '\0'};
            int index = (int)(occurrence - output);

            hex_chr[0] = output[index + 1];
            hex_chr[1] = output[index + 2];
            output[index] = strtol(hex_chr, NULL, 16);

            for(int j = index + 1; j < len_content; j++) {
                if (j + 2 < len_content) {
                    output[j] = output[j + 2];
                } else if (j < len_content) {
                    output[j] = '\0';
                }
            }
        }
    }

    return output;
}

pivot_actions http_parser_action(char * request_body)
{
	pivot_actions actions = {};

	cJSON* subitem = cJSON_Parse(request_body);
	cJSON* power = cJSON_GetObjectItem(subitem, "power");

	if(power->valueint == true)
	{
		actions.power_state = PIVOT_ON;

		cJSON* water = cJSON_GetObjectItem(subitem, "water");
		cJSON* direction = cJSON_GetObjectItem(subitem, "direction");
		cJSON* percentimeter = cJSON_GetObjectItem(subitem, "percentimeter");

		if(water->valueint == true)
		{
			actions.watering_state = PIVOT_WET;
		}
		else
		{
			actions.watering_state = PIVOT_DRY;
		}

		if(strcmp(direction->valuestring, "ANTI_CLOCKWISE") == 0)
		{
			actions.rotation = PIVOT_CW;
		}
		else
		{
			actions.rotation = PIVOT_CCW;
		}

		actions.percentimeter = percentimeter->valueint;

		cJSON_Delete(water);
		cJSON_Delete(direction);
		cJSON_Delete(percentimeter);
	}
	else
	{
		actions.power_state = PIVOT_OFF;
		actions.watering_state = PIVOT_DRY;
		actions.rotation = PIVOT_CW;
		actions.percentimeter = 0;
	}

	cJSON_Delete(power);
	cJSON_Delete(subitem);
	return actions;
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

char* http_parser_config_to_json(pivot_config config)
{
	char* string_out = "";
	char int_str[20];

	// create JSON
	cJSON *root =  cJSON_CreateObject();

	cJSON_AddItemToObject(root, "pivot_id", cJSON_CreateString(config.pivot_id));
	cJSON_AddItemToObject(root, "gprs_id", cJSON_CreateString(config.gprs_id));

	if(config.contactor == CONTACTOR_NA)
	{
		cJSON_AddItemToObject(root, "contactor_type", cJSON_CreateString("NA"));
	}
	else
	{
		cJSON_AddItemToObject(root, "contactor_type", cJSON_CreateString("NF"));
	}

	if(config.pressure_switch == PRESSURE_SWITCH_NA )
	{
		cJSON_AddItemToObject(root, "pressure_type", cJSON_CreateString("NA"));
	}
	else
	{
		cJSON_AddItemToObject(root, "pressure_type", cJSON_CreateString("NF"));
	}

	sprintf(int_str, "%d", config.pressurization_time );
	cJSON_AddItemToObject(root, "pressure_time", cJSON_CreateString(int_str));

	memset(int_str, 0x00, sizeof(int_str));
	sprintf(int_str, "%d", config.on_off_time );
	cJSON_AddItemToObject(root, "turn_on_time", cJSON_CreateString(int_str));

	memcpy(string_out, cJSON_Print(root), strlen(cJSON_Print(root)));
	cJSON_Delete(root);

	return string_out;
}

