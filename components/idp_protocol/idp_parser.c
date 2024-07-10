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

/**
 * @brief The minimum time value accepted for pressurization in seconds.
 */
#define PRESSURE_TIME_MIN_SEC 30

/**
 * @brief The maximum time value accepted for pressurization in seconds.
 */
#define PRESSURE_TIME_MAX_SEC 1800

/**
 * @brief The minimum time value accepted for On_Off_relay activation in seconds.
 */
#define RELAY_TIME_MIN_SEC 1

/**
 * @brief The maximum time value accepted for On_Off_relay activation in seconds.
 */
#define RELAY_TIME_MAX_SEC 30

/**
 * @brief The minimum time value accepted for periodically read status (IDP_0) in minutes.
 */
#define READ_TIME_MIN_MINUTES 5

/**
 * @brief The maximum time value accepted for periodically read status (IDP_0) in minutes.
 */
#define READ_TIME_MAX_MINUTES 60

/**
 * @brief The minimum time value accepted for latitude.
 */
#define LATITUDE_MIN 0

/**
 * @brief The maximum time value accepted for latitude.
 */
#define LATITUDE_MAX 90

/**
 * @brief The minimum time value accepted for longitude.
 */
#define LONGITUDE_MIN 0

/**
 * @brief The maximum time value accepted for longitude.
 */
#define LONGITUDE_MAX 180

/**
 * @brief The minimum time value accepted for periodically payload from GPS (IDP_7) in seconds.
 */
#define PAYLOAD_TIME_MIN_SEC 1

/**
 * @brief The maximum time value accepted for periodically payload from GPS (IDP_7) in seconds.
 */
#define PAYLOAD_TIME_MAX_SEC 120


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
    	ESP_LOGE(IDP_PARSER_TAG, "invalid package");
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
 * @brief Validate the actions in a pivot_actions structure.
 *
 * This function validates the actions in a `pivot_actions` structure to ensure they are within valid ranges.
 *
 * @param[in] actions The `pivot_actions` structure to validate.
 * @return true if the actions are valid, false otherwise.
 */
bool idp_parser_validate_actions(const pivot_actions actions)
{
	bool ret = false;

	if(actions.percentimeter <= 100)
	{
		if(actions.power_state == PIVOT_ON
		|| actions.power_state == PIVOT_OFF)
		{
			if(actions.rotation == PIVOT_CW
			|| actions.rotation == PIVOT_CCW)
			{
				if(actions.watering_state == PIVOT_DRY
				|| actions.watering_state == PIVOT_WET)
				{
					ret = true;
				}
			}
			else if(actions.power_state == PIVOT_OFF
					&& actions.rotation == 0
					&& actions.watering_state == 0)
			{
				ret = true;
			}
		}
	}

	return ret;
}

/**
 * @brief Validate the network configuration.
 *
 * This function validates the network configuration to ensure all necessary fields are filled.
 *
 * @param[in] net_config The network configuration to validate.
 * @return true if the configuration is valid, false otherwise.
 */
bool idp_parser_validate_network(const network_config net_config)
{
	bool ret = false;

	if(strlen(net_config.gprs_id) > 0 && strlen(net_config.modem_apn) > 0
			&& strlen(net_config.wifi_ssid) > 0 && strlen(net_config.wifi_pass) > 0)
	{
		ret = true;
	}

	return ret;
}

/**
 * @brief Create a password from the actions in a `pivot_actions` structure.
 *
 * This function creates a password from the actions in a `pivot_actions` structure.
 *
 * @param[in] actions The `pivot_actions` structure.
 * @return The generated password.
 */
uint16_t idp_parser_create_pwd(pivot_actions actions)
{
	uint16_t dwp = ((actions.rotation * 100)
	+ (actions.watering_state)
	* 10 + actions.power_state);

	return dwp;
}

/**
 * @brief Get the `pivot_actions` structure from a password.
 *
 * This function extracts the `pivot_actions` structure from a password.
 *
 * @param[in] pwd The password to extract from.
 * @param[out] actions The extracted `pivot_actions` structure.
 */
void idp_parser_get_pwd(uint16_t pwd, pivot_actions* actions)
{
	actions->power_state = pwd % 10;
	pwd /= 10;

	actions->watering_state = pwd % 10;
	pwd /= 10;

	actions->rotation = pwd;
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
            strcat(str_out, bool_arg ? "1" : "0");
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
            * (bool *) arg_pairs[index].value = strcmp(token, "0") == 0 ? false : true;
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
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param net_config data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_02(const network_config net_config)
{
	bool ret = false;

	if(strlen(net_config.gprs_id) > 0 && strlen(net_config.modem_apn) > 0
			&& strlen(net_config.wifi_ssid) > 0 && strlen(net_config.wifi_pass) > 0)
	{
		ret = true;
	}

	return ret;
}

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param pivot_config data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_03(const pivot_config pivot_config)
{
    bool ret = false;
    
    if(strcmp(pivot_config.contactor, "NA") != 0){
        if(strcmp(pivot_config.contactor, "NF") != 0){
            return ret;
        }
    }

    if(strcmp(pivot_config.pressure, "NA") != 0){
        if(strcmp(pivot_config.pressure, "NF") != 0){
            return ret;
        }
    }

    if(!(pivot_config.pressurization_time >= PRESSURE_TIME_MIN_SEC && pivot_config.pressurization_time <= PRESSURE_TIME_MAX_SEC)){
        return ret;
    }

    if(!(pivot_config.on_time >= RELAY_TIME_MIN_SEC && pivot_config.on_time <= RELAY_TIME_MAX_SEC)){
        return ret;
    }

    if(!(pivot_config.off_time >= RELAY_TIME_MIN_SEC && pivot_config.off_time <= RELAY_TIME_MAX_SEC)){
        return ret;
    }

    if(!(pivot_config.read_time >= READ_TIME_MIN_MINUTES && pivot_config.read_time <= READ_TIME_MAX_MINUTES)){
        return ret;
    }

    return true;
}

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param eco_config data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_04(const eco_mode_config eco_config)
{
    bool ret = false;

    if(eco_config.start_time >= 0 && eco_config.end_time >= 0)
    {
    	ret = true;
    }

    return ret;
}

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param sector_config data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_05(const sector_config sector_config)
{
    bool ret = false;

    if(!(sector_config.sector_number <= CONFIG_SECTORS_MAX_VALUE))
    {
        return ret;
    }

    for (size_t i = 1; i < sector_config.sector_number; i++)
    {
        if(!(sector_config.sectors[i].start_angle <= 360))
        {
            return ret;
        }

        if(!(sector_config.sectors[i].end_angle <= 360))
        {
            return ret;
        }
    }
    
    return true;
}

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param scheduling data to be validated.
 * @param srt_author Pointer to data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_14(const pivot_scheduling_date scheduling, const char* srt_author)
{
    bool ret = false;

    if(!(scheduling.start_date > 0))
    {
        return ret;
    }

    if(!(scheduling.end_date > 0))
    {
        return ret;
    }

    if(!(idp_parser_validate_actions(scheduling.actions))){
        return ret;
    }

    if(!((scheduling.actions.percentimeter > 0) && (scheduling.actions.percentimeter <= 100)))
    {
        return ret;
    }

    if(!(strlen(srt_author) > 0))
    {
        return ret;
    }
    
    return true;
}

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param scheduling data to be validated.
 * @param srt_author Pointer to data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_15(const pivot_scheduling_angle scheduling, const char* srt_author)
{
    bool ret = false;

    if(!(scheduling.start_date > 0))
    {
        return ret;
    }

    if(!(scheduling.end_angle <= 360))
    {
        return ret;
    }

    if(!(idp_parser_validate_actions(scheduling.actions))){
        return ret;
    }

    if(!((scheduling.actions.percentimeter > 0) && (scheduling.actions.percentimeter <= 100)))
    {
        return ret;
    }

    if(!(strlen(srt_author) > 0))
    {
        return ret;
    }
    
    return true;
}

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param scheduling data to be validated.
 * @param srt_author Pointer to data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_16(const pivot_scheduling_off_date scheduling, const char* srt_author)
{
    bool ret = false;

    if(!(scheduling.end_date > 0))
    {
        return ret;
    }

    if(!(strlen(srt_author) > 0))
    {
        return ret;
    }
    
    return true;
}

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param scheduling data to be validated.
 * @param srt_author Pointer to data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_17(const pivot_scheduling_off_angle scheduling, const char* srt_author)
{
    bool ret = false;

    if(!(scheduling.end_angle <= 360))
    {
        return ret;
    }

    if(!(strlen(srt_author) > 0))
    {
        return ret;
    }
    
    return true;
}

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param timestamp data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_21(const time_t timestamp)
{
    bool ret = false;

    if(!(timestamp > 1720106718))
    {
        return ret;
    }

    return true;
}

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param return_config data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_22(const pivot_return_config return_config)
{
    bool ret = false;

    if(!(return_config.start_angle <= 360))
    {
        return ret;
    }

    if(!(return_config.end_angle <= 360))
    {
        return ret;
    }

    if(!(return_config.automatic_return == 0 || return_config.automatic_return == 1))
    {
        return ret;
    }

    if(!(return_config.water_return == 0 || return_config.water_return == 1))
    {
        return ret;
    }

    return true;
}

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param gps_config data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_23(const gps_config gps_config)
{
    bool ret = false;
    double latitude = atof(gps_config.latitude);
    double longitude = atof(gps_config.longitude);

    if(!(gps_config.sinal_lat == 0 || gps_config.sinal_lat == 1))
    {
        return ret;
    }

    if(!(latitude > LATITUDE_MIN && latitude < LATITUDE_MAX ))
    {
        return ret;
    }    
    
    if(!(gps_config.sinal_lon == 0 || gps_config.sinal_lon == 1))
    {
        return ret;
    }

    if(!(longitude > LONGITUDE_MIN && longitude < LONGITUDE_MAX ))
    {
        return ret;
    }

    if(!(gps_config.time_payload >= PAYLOAD_TIME_MIN_SEC && gps_config.time_payload <= PAYLOAD_TIME_MAX_SEC ))
    {
        return ret;
    }

    if(!(gps_config.offset > 0))
    {
        return ret;
    }

    return true;
}
