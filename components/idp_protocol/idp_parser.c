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
	if(string_in[0] == '$' && string_in[str_last_position] == '#' )
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

void idp_parser_set(char* str_out, const char* str, ...)
{
    va_list args;
    va_start(args, str);

    size_t buffer_size = strlen(str) + 1;
    const char* current_str = str;

    // Calcula o tamanho total do buffer
    while (current_str != NULL) {
        if (current_str != str) {
            buffer_size++; // Adicionar um caractere traço '-' para cada string exceto a primeira
        }

        if (strcmp(current_str, "int8_t") == 0) {
            int8_t int8_arg = va_arg(args, int);
            char arg_buffer[MAX_BUFFER_SIZE];
            snprintf(arg_buffer, MAX_BUFFER_SIZE, "%" PRId8, int8_arg);
            buffer_size += strlen(arg_buffer);
        } else if (strcmp(current_str, "uint8_t") == 0) {
            uint8_t uint8_arg = va_arg(args, int);
            char arg_buffer[MAX_BUFFER_SIZE];
            snprintf(arg_buffer, MAX_BUFFER_SIZE, "%" PRIu8, uint8_arg);
            buffer_size += strlen(arg_buffer);
        } else if (strcmp(current_str, "int16_t") == 0) {
            int16_t int16_arg = va_arg(args, int);
            char arg_buffer[MAX_BUFFER_SIZE];
            snprintf(arg_buffer, MAX_BUFFER_SIZE, "%" PRId16, int16_arg);
            buffer_size += strlen(arg_buffer);
        } else if (strcmp(current_str, "uint16_t") == 0) {
            uint16_t uint16_arg = va_arg(args, int);
            char arg_buffer[MAX_BUFFER_SIZE];
            snprintf(arg_buffer, MAX_BUFFER_SIZE, "%" PRIu16, uint16_arg);
            buffer_size += strlen(arg_buffer);
        } else {
            // Se não for nenhum dos tipos esperados, considera como uma string normal
            buffer_size += strlen(current_str);
        }

        current_str = va_arg(args, const char*);
    }

    va_end(args);

    // Concatenar as strings com o traço '-' e adicionar '$' e '#'
    str_out[0] = '\0'; // Inicializar o buffer vazio
    strcat(str_out, "$");
    strcat(str_out, str);

    va_start(args, str);

    current_str = va_arg(args, const char*);
    while (current_str != NULL) {
        strcat(str_out, "-");

        if (strcmp(current_str, "int8_t") == 0) {
            int8_t int8_arg = va_arg(args, int);
            char arg_buffer[MAX_BUFFER_SIZE];
            snprintf(arg_buffer, MAX_BUFFER_SIZE, "%" PRId8, int8_arg);
            strcat(str_out, arg_buffer);
        } else if (strcmp(current_str, "uint8_t") == 0) {
            uint8_t uint8_arg = va_arg(args, int);
            char arg_buffer[MAX_BUFFER_SIZE];
            snprintf(arg_buffer, MAX_BUFFER_SIZE, "%" PRIu8, uint8_arg);
            strcat(str_out, arg_buffer);
        } else if (strcmp(current_str, "int16_t") == 0) {
            int16_t int16_arg = va_arg(args, int);
            char arg_buffer[MAX_BUFFER_SIZE];
            snprintf(arg_buffer, MAX_BUFFER_SIZE, "%" PRId16, int16_arg);
            strcat(str_out, arg_buffer);
        } else if (strcmp(current_str, "uint16_t") == 0) {
            uint16_t uint16_arg = va_arg(args, int);
            char arg_buffer[MAX_BUFFER_SIZE];
            snprintf(arg_buffer, MAX_BUFFER_SIZE, "%" PRIu16, uint16_arg);
            strcat(str_out, arg_buffer);
        } else {
            // Se não for nenhum dos tipos esperados, considera como uma string normal
            strcat(str_out, current_str);
        }

        current_str = va_arg(args, const char*);
    }

    va_end(args);

    strcat(str_out, "#"); // Adicionar '#' no final do buffer
}


