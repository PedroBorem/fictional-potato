/**
 * @file idp_parser.h
 * @date July 12, 2023
 * @brief IDP Parser class for handling IDP protocol packets.
 *
 * This file contains the declaration of the IDP Parser class, which is responsible for parsing
 * and handling IDP (Integrated Data Protocol) protocol packets.
 */

#ifndef COMPONENTS_IDP_PROTOCOL__INCLUDE_IDP_PARSER_H_
#define COMPONENTS_IDP_PROTOCOL__INCLUDE_IDP_PARSER_H_

#include "project_config.h"

/**
 * @struct arg_pair
 * @brief Structure representing an argument pair (type and value).
 */
typedef struct arg_pair {
    const char* type; /**< Type of the argument. */
    void* value;      /**< Value of the argument. */
} arg_pair_t;

/**
 * @brief Parse the IDP packet and extract the IDP type.
 *
 * This function extracts the IDP type from the received IDP packet.
 *
 * @param[in] string_in The received IDP packet string.
 * @param[out] string_out The IDP type extracted from the packet.
 * @return The IDP type extracted from the packet, or IDP_INVALID if the packet is invalid.
 */
idp_type idp_parser_get(const char* string_in, char* string_out);

/**
 * @brief Validate the specified pivot actions.
 *
 * This function validates the specified pivot actions to ensure they conform to the IDP protocol.
 *
 * @param[in] actions The pivot actions to be validated.
 * @return true if the actions are valid, false otherwise.
 */
bool idp_parser_validate_actions(const pivot_actions actions);

/**
 * @brief Validate the specified network configuration.
 *
 * This function validates the specified network configuration to ensure it conforms to the IDP protocol.
 *
 * @param[in] net_config The network configuration to be validated.
 * @return true if the network configuration is valid, false otherwise.
 */
bool idp_parser_validate_network(const network_config net_config);

/**
 * @brief Create an IDP packet with a generated password based on the specified pivot actions.
 *
 * This function creates an IDP packet with a generated password based on the specified pivot actions.
 *
 * @param[in] actions The pivot actions used to generate the password.
 * @return The generated password.
 */
uint16_t idp_parser_create_pwd(pivot_actions actions);

/**
 * @brief Get pivot actions from the specified password.
 *
 * This function retrieves pivot actions from the specified password.
 *
 * @param[in] pwd The password from which to extract pivot actions.
 * @param[out] actions The pivot actions extracted from the password.
 */
void idp_parser_get_pwd(uint16_t pwd, pivot_actions* actions);

/**
 * @brief Create an IDP packet with the given arguments.
 *
 * This function creates an IDP packet string using the provided arguments.
 *
 * @param[out] str_out The output buffer to store the created IDP packet.
 * @param[in] arg_pairs The array of argument pairs (type-value) to be included in the packet.
 */
void idp_parser_create_package(char* str_out, arg_pair_t arg_pairs[]);

/**
 * @brief Parse the IDP packet and extract data to populate the argument pairs.
 *
 * This function parses the received IDP packet and extracts data to populate the argument pairs.
 *
 * @param[in] str_arg The received IDP packet string.
 * @param[out] arg_pairs The array of argument pairs to be populated with data from the packet.
 */
void idp_parser_get_packet_data(const char* str_arg, arg_pair_t arg_pairs[]);

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
uint8_t idp_parser_get_delimiter(const char *buffer);

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param net_config data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_02(const network_config net_config)
;

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param pivot_config data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_03(const pivot_config pivot_config);

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param eco_config data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_04(const eco_mode_config eco_config);

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param sector_config data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_05(const sector_config sector_config);

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param scheduling data to be validated.
 * @param srt_author Pointer to data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_14(const pivot_scheduling_date scheduling, const char* srt_author);

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param scheduling data to be validated.
 * @param srt_author Pointer to data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_15(const pivot_scheduling_angle scheduling, const char* srt_author);

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param scheduling data to be validated.
 * @param srt_author Pointer to data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_16(const pivot_scheduling_off_date scheduling, const char* srt_author);

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param scheduling data to be validated.
 * @param srt_author Pointer to data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_17(const pivot_scheduling_off_angle scheduling, const char* srt_author);

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param timestamp data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_21(const time_t timestamp);

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param return_config data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_22(const pivot_return_config return_config);

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param gps_config data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_23(const gps_config gps_config);

#endif /* COMPONENTS_IDP_PROTOCOL__INCLUDE_IDP_PARSER_H_ */
