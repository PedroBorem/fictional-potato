/**
 * @file idp_parser.c
 * @brief Implementation of the IDP (Intelligent Device Protocol) parser.
 *
 * This file provides functions to parse IDP packets and create IDP packets
 * with various data types.
 *
 */

/* Private inclusions -------------------------------------------- */
#include "idp_parser.h"
#include "log.h"

/* c base include */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <inttypes.h>

/* Private definitions ------------------------------------------- */
/**
 * @brief The tag used for logging IDP parser-related messages.
 */
#define IDP_PARSER_TAG  "idp_parser"

/**
 * @brief The maximum size of the buffer used for creating IDP packets.
 */
#define IDP_MAX_PKG_SIZE 200

/* Private function prototype ------------------------------------ */
/**
 * @brief Check if the received IDP packet is valid.
 *
 * This function checks if the received IDP packet has the correct format.
 *
 * @param[in] string_in The received IDP packet string.
 * @return ESP_OK if the packet is valid, ESP_FAIL otherwise.
 */
static bool idp_parser_check_pack(const char* string_in, char* string_out);

/* Private methods ----------------------------------------------- */

/**
 * @brief Check if the received IDP packet is valid.
 *
 * This function checks if the received IDP packet has the correct format.
 *
 * @param[in] string_in The received IDP packet string.
 * @param[out] string_out The valid IDP packet string.
 * @return true if the packet is valid, false otherwise.
 */
static bool idp_parser_check_pack(const char* string_in, char* string_out)
{
    bool ret = false;
    int ch_1 = '#';
    int ch_2 = '$';

    char* ptr_1 = strchr(string_in, ch_1);
    char* ptr_2 = strchr(string_in, ch_2);

    if (ptr_1 != NULL && ptr_2 != NULL
    && (strlen(ptr_1) > strlen(ptr_2)))
    {
        size_t sub_len = ptr_2 - ptr_1 + 1;

        strncpy(string_out, ptr_1, sub_len);
        string_out[sub_len] = '\0';

        ret = true;
    }
    else
    {
        LOG_ERROR(IDP_PARSER_TAG, "IDP", "invalid package");
    }

    return ret;
}

/* Public methods ------------------------------------------------ */
/**
 * @brief Get the IDP type from the given string.
 *
 * This function extracts the IDP type from the received IDP packet string.
 *
 * @param[in] string_in The received IDP packet string.
 * @param[out] string_out The extracted IDP type as a string.
 * @return The IDP type as an `idp_type` enum.
 */
idp_type idp_parser_get(const char* string_in, char* string_out)
{
	idp_type idp_ret = IDP_INVALID;
	char* str_copy = strdup(string_in);
	char str_sub[100] = {};

	if(idp_parser_check_pack(str_copy, str_sub) == true)
	{
		strcpy(string_out, str_sub);

	    const char delimiter[] = "-";
		char* ptr = strtok(str_sub, delimiter);

	    sscanf(&ptr[1], "%d",(int*)&idp_ret);
	}

	free(str_copy);
	return idp_ret;
}

/**
 * @brief Checks whether a payload belongs to a specific IDP.
 *
 * The function performs a lightweight prefix validation, accepting both
 * `#IDP-...` and `#IDP$` payload formats.
 *
 * @param[in] payload Raw payload to be inspected.
 * @param[in] idp Expected IDP value.
 * @return true if the payload matches the requested IDP, false otherwise.
 */
bool idp_parser_is_payload_from_idp(const char *payload, idp_type idp)
{
    char expected_prefix[8] = {};

    if (payload == NULL)
    {
        return false;
    }

    snprintf(expected_prefix, sizeof(expected_prefix), "#%u", (unsigned int)idp);

    if (strncmp(payload, expected_prefix, strlen(expected_prefix)) != 0)
    {
        return false;
    }

    return (payload[strlen(expected_prefix)] == '-' ||
            payload[strlen(expected_prefix)] == '$');
}

/**
 * @brief Create an IDP package from an array of argument pairs.
 *
 * This function creates an IDP package from an array of argument pairs.
 *
 * @param[out] str_out The created IDP package string.
 * @param[in] arg_pairs The array of argument pairs.
 */
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
            char arg_buffer[IDP_MAX_PKG_SIZE];
            snprintf(arg_buffer, IDP_MAX_PKG_SIZE, "%02" PRIu8, uint8_arg);
            strcat(str_out, arg_buffer);
        } else if (strcmp(arg_pairs[i].type, "uint16_t") == 0) {
            uint16_t uint16_arg = * (uint16_t *) arg_pairs[i].value;
            char arg_buffer[IDP_MAX_PKG_SIZE];
            snprintf(arg_buffer, IDP_MAX_PKG_SIZE, "%02" PRIu16, uint16_arg);
            strcat(str_out, arg_buffer);
        } else if (strcmp(arg_pairs[i].type, "uint32_t") == 0) {
            uint32_t uint32_arg = * (uint32_t *) arg_pairs[i].value;
            char arg_buffer[IDP_MAX_PKG_SIZE];
            snprintf(arg_buffer, IDP_MAX_PKG_SIZE, "%04" PRIu32, uint32_arg);
            strcat(str_out, arg_buffer);
        } else if (strcmp(arg_pairs[i].type, "int8_t") == 0) {
            int8_t int8_arg = * (int8_t *) arg_pairs[i].value;
            char arg_buffer[IDP_MAX_PKG_SIZE];
            snprintf(arg_buffer, IDP_MAX_PKG_SIZE, "%02" PRId8, int8_arg);
            strcat(str_out, arg_buffer);
        } else if (strcmp(arg_pairs[i].type, "int16_t") == 0) {
            int16_t int16_arg = * (int16_t *) arg_pairs[i].value;
            char arg_buffer[IDP_MAX_PKG_SIZE];
            snprintf(arg_buffer, IDP_MAX_PKG_SIZE, "%04" PRId16, int16_arg);
            strcat(str_out, arg_buffer);
        } else if (strcmp(arg_pairs[i].type, "int32_t") == 0) {
            int32_t int32_arg = * (int32_t *) arg_pairs[i].value;
            char arg_buffer[IDP_MAX_PKG_SIZE];
            snprintf(arg_buffer, IDP_MAX_PKG_SIZE, "%08" PRId32, int32_arg);
            strcat(str_out, arg_buffer);
        } else if (strcmp(arg_pairs[i].type, "int") == 0) {
            int int_arg = * (int *) arg_pairs[i].value;
            char arg_buffer[IDP_MAX_PKG_SIZE];
            snprintf(arg_buffer, IDP_MAX_PKG_SIZE, "%08d", int_arg);
            strcat(str_out, arg_buffer);
        } else if (strcmp(arg_pairs[i].type, "string") == 0) {
            const char* str_arg = (const char*)arg_pairs[i].value;
            strcat(str_out, str_arg);
        } else if (strcmp(arg_pairs[i].type, "time_t") == 0) {
						time_t time_arg = * (time_t *) arg_pairs[i].value;
						char arg_buffer[IDP_MAX_PKG_SIZE];
						snprintf(arg_buffer, IDP_MAX_PKG_SIZE, "%ld", (long)time_arg);
						strcat(str_out, arg_buffer);
        } else if (strcmp(arg_pairs[i].type, "bool") == 0) {
            bool bool_arg = * (bool *) arg_pairs[i].value;
            if (bool_arg == true)
            {
                strcat(str_out, "1");
            }
            else
            {
                strcat(str_out, "0");
            }
        }
    }

    strcat(str_out, "$"); // Adicionar '$' no final do buffer
}

/**
 * @brief Get the data from an IDP package string.
 *
 * This function extracts data from an IDP package string and fills an array of argument pairs.
 *
 * @param[in] str_arg The IDP package string.
 * @param[out] arg_pairs The array of argument pairs to fill.
 */
void idp_parser_get_packet_data(const char* str_arg, arg_pair_t arg_pairs[])
{
    char* str_copy = strdup(str_arg); // Fazer uma cópia da string para evitar modificar a original

    char* token = strtok(str_copy + 1, "-$"); // Ignorar o primeiro caractere '#' e o último caractere '$'
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
        	uint8_t str_size = strlen(token);
        	strncpy(arg_pairs[index].value, token, str_size);
            ((char *)arg_pairs[index].value)[str_size] = '\0';
        } else if (strcmp(type, "bool") == 0) {
            if (strcmp(token, "0") == 0)
            {
                * (bool *) arg_pairs[index].value = false;
            }
            else
            {
                * (bool *) arg_pairs[index].value = true;
            }
        } else if (strcmp(type, "time_t") == 0) {
        	* (time_t *) arg_pairs[index].value = (time_t) strtol(token, NULL, 10);
        }

        token = strtok(NULL, "-$");
        index++;
    }

    free(str_copy);
}

/**
 * @brief Returns the number of delimiters '-' present in the buffer.
 *
 * This function counts the number of delimiters '-' present in the provided buffer.
 *
 * @param buffer The buffer containing the data to be analyzed.
 * @return The number of '-' delimiters in the buffer.
 * @note The provided buffer must be a valid null-terminated string.
 * @warning This function does not check whether the provided buffer is valid or null-terminated.
 * Ensure that the buffer is valid and null-terminated to avoid undefined behavior.
 */
uint8_t idp_parser_get_delimiter(const char *buffer)
{
    int count = 0;
    int buffer_size = strlen(buffer);

    for (int i = 0; i < buffer_size; i++) {
        if (buffer[i] == '-') {
        	count++;
        }
    }

    return count;
}

/**
 * @brief Validates if characters in a buffer are within the printable ASCII range.
 *
 * Iterates over each character in the buffer to ensure they are within the ASCII printable 
 * range (32 to 125). This validation helps prevent processing issues related to non-printable 
 * characters.
 *
 * @param buffer Array of characters to be validated.
 * @param size Number of characters in the buffer.
 * @return true if all characters are valid, otherwise false.
 */
bool check_valid_characters(const char *buffer, uint8_t size)
{
	for(uint8_t i = 0; i < size; i++)
	{
		if(buffer[i] <= 32 || buffer[i] >= 125)
		{
			return false;
		}
	}

	return true;
}

/**
 * @brief Removes specific characters ('#' and '$') from the input buffer.
 *
 * This function iterates over the input buffer and removes any occurrences 
 * of the characters '#' and '$', storing the result in the output buffer. 
 * The output buffer is null-terminated after processing.
 *
 * @param buffer Input character array from which '#' and '$' will be removed.
 * @param output_buffer Output character array to store the result after removal of '#' and '$'.
 * @param output_buffer_sizeOutput Output buffer size to avoid overflow.
 * @return int 0 if the operation was successful, -1 if there was an error.
 */
bool idp_parser_remove_hashtag_cipher(const char *buffer, char *output_buffer, size_t output_buffer_size) 
{
    if (buffer == NULL || output_buffer == NULL) 
    {
        return false; 
    }

    int j = 0;
    for (int i = 0; buffer[i] != '\0'; i++) 
    {
        if (buffer[i] != '#' && buffer[i] != '$') 
        {
            if (j >= (int)(output_buffer_size - 1)) 
            {
                return false;
            }
            output_buffer[j++] = buffer[i];
        }
    }

    output_buffer[j] = '\0';

    return true;
}
