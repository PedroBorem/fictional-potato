/*
 * idp_parser.c
 *
 *  Created on: 12 de jul. de 2023
 *      Author: bruno
 */

/* Private inclusions -------------------------------------------- */
#include "idp_parser.h"
#include "log.h"
#include "esp_err.h"

/* c base include */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <inttypes.h>

/* Private definitions ------------------------------------------- */
#define IDP_PARSER_TAG	"idp_parser"

#define MAX_BUFFER_SIZE 200


static esp_err_t idp_parser_check_pack(const char* string_in);

/* Private methods ----------------------------------------------- */
static esp_err_t idp_parser_check_pack(const char* string_in)
{
	esp_err_t ret = ESP_FAIL;

	size_t str_last_position = strlen(string_in) - 1;
	if(string_in[0] == '#' && string_in[str_last_position] == '$' )
	{
		LOG_COMM(IDP_PARSER_TAG, "package : %s", string_in);
		ret = ESP_OK;
	}
	else
	{
		ESP_LOGE(IDP_PARSER_TAG, "invalid package");
	}

	return ret;
}

/* Public methods ------------------------------------------------ */
idp_type idp_parser_get(const char* string_in)
{
	idp_type idp_ret = IDP_INVALID;

    const char delimiter[] = "-";
	char* ptr = strtok(string_in, delimiter); //todo ajustar o const

	if(idp_parser_check_pack(string_in) == ESP_OK)
	{
	    sscanf(&ptr[1], "%d",(int*)&idp_ret);
	}
	else
	{
		ESP_LOGE(IDP_PARSER_TAG, "invalid idp");
	}

	return idp_ret;
}

void idp_parser_create_package(char* str_out, arg_pair_t arg_pairs[])
{
    // Inicializar o buffer vazio
    str_out[0] = '\0';

    // Concatenar as strings com o traço '-' e adicionar '#'
    strcat(str_out, "#");

    for (int i = 0; arg_pairs[i].type != NULL; i++) {
        if (i != 0) {
            strcat(str_out, "-");
        }

        if (strcmp(arg_pairs[i].type, "uint8_t") == 0) {
            uint8_t uint8_arg = * (uint8_t *) arg_pairs[i].value;
            char arg_buffer[MAX_BUFFER_SIZE];
            snprintf(arg_buffer, MAX_BUFFER_SIZE, "%02" PRIu8, uint8_arg);
            strcat(str_out, arg_buffer);
        } else if (strcmp(arg_pairs[i].type, "uint16_t") == 0) {
            uint16_t uint16_arg = * (uint16_t *) arg_pairs[i].value;
            char arg_buffer[MAX_BUFFER_SIZE];
            snprintf(arg_buffer, MAX_BUFFER_SIZE, "%02" PRIu16, uint16_arg);
            strcat(str_out, arg_buffer);
        } else if (strcmp(arg_pairs[i].type, "uint32_t") == 0) {
            uint32_t uint32_arg = * (uint32_t *) arg_pairs[i].value;
            char arg_buffer[MAX_BUFFER_SIZE];
            snprintf(arg_buffer, MAX_BUFFER_SIZE, "%04" PRIu32, uint32_arg);
            strcat(str_out, arg_buffer);
        } else if (strcmp(arg_pairs[i].type, "int8_t") == 0) {
            int8_t int8_arg = * (int8_t *) arg_pairs[i].value;
            char arg_buffer[MAX_BUFFER_SIZE];
            snprintf(arg_buffer, MAX_BUFFER_SIZE, "%02" PRId8, int8_arg);
            strcat(str_out, arg_buffer);
        } else if (strcmp(arg_pairs[i].type, "int16_t") == 0) {
            int16_t int16_arg = * (int16_t *) arg_pairs[i].value;
            char arg_buffer[MAX_BUFFER_SIZE];
            snprintf(arg_buffer, MAX_BUFFER_SIZE, "%04" PRId16, int16_arg);
            strcat(str_out, arg_buffer);
        } else if (strcmp(arg_pairs[i].type, "int32_t") == 0) {
            int32_t int32_arg = * (int32_t *) arg_pairs[i].value;
            char arg_buffer[MAX_BUFFER_SIZE];
            snprintf(arg_buffer, MAX_BUFFER_SIZE, "%08" PRId32, int32_arg);
            strcat(str_out, arg_buffer);
        } else if (strcmp(arg_pairs[i].type, "int") == 0) {
            int int_arg = * (int *) arg_pairs[i].value;
            char arg_buffer[MAX_BUFFER_SIZE];
            snprintf(arg_buffer, MAX_BUFFER_SIZE, "%08d", int_arg);
            strcat(str_out, arg_buffer);
        } else if (strcmp(arg_pairs[i].type, "string") == 0) {
            const char* str_arg = (const char*)arg_pairs[i].value;
            strcat(str_out, str_arg);
        }
    }

    strcat(str_out, "$"); // Adicionar '$' no final do buffer
}


void idp_parser_get_packet_data(const char* str_arg, arg_pair_t arg_pairs[])
{
    char* str_copy = strdup(str_arg); // Fazer uma cópia da string para evitar modificar a original

    char* token = strtok(str_copy + 1, "-"); // Ignorar o primeiro caractere '$'
    int index = 0;

    while (token != NULL)
    {
        const char* type = arg_pairs[index].type;

        if (type == NULL) {
            break;
        }

        if (strcmp(type, "uint8_t") == 0) {
            * (uint8_t *) arg_pairs[index].value = (uint8_t) strtoul(token, NULL, 10);
        } else if (strcmp(type, "uint16_t") == 0) {
            * (uint16_t *) arg_pairs[index].value = (uint16_t) strtoul(token, NULL, 10);
        } else if (strcmp(type, "uint32_t") == 0) {
            * (uint32_t *) arg_pairs[index].value = (uint32_t) strtoul(token, NULL, 10);
        } else if (strcmp(type, "int8_t") == 0) {
            * (int8_t *) arg_pairs[index].value = (int8_t) strtol(token, NULL, 10);
        } else if (strcmp(type, "int16_t") == 0) {
            * (int16_t *) arg_pairs[index].value = (int16_t) strtol(token, NULL, 10);
        } else if (strcmp(type, "int32_t") == 0) {
            * (int32_t *) arg_pairs[index].value = (int32_t) strtol(token, NULL, 10);
        } else if (strcmp(type, "int") == 0) {
            * (int *) arg_pairs[index].value = atoi(token);
        } else if (strcmp(type, "string") == 0) {
            strncpy(arg_pairs[index].value, token, 49);
            ((char *)arg_pairs[index].value)[49] = '\0';
        } else if (strcmp(type, "bool") == 0) {
            * (bool *) arg_pairs[index].value = strcmp(token, "0") == 0 ? false : true;
        }

        token = strtok(NULL, "-");
        index++;
    }

    free(str_copy);
}


