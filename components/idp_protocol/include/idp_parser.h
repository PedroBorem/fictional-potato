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
    size_t size;      /**< Size of the argument. */
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
idp_type idp_parser_get(const char* string_in, char* string_out, size_t string_out_size);

/**
 * @brief Checks whether a payload belongs to a specific IDP.
 *
 * This helper is intended for lightweight routing or log filtering when the
 * caller only needs to know whether a raw payload starts with a given IDP.
 *
 * @param[in] payload Raw payload to be inspected.
 * @param[in] idp Expected IDP value.
 * @return true if the payload matches the requested IDP, false otherwise.
 */
bool idp_parser_is_payload_from_idp(const char *payload, idp_type idp);

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
 * @brief Removes specific characters ('#' and '$') from the input buffer.
 *
 * This function iterates over the input buffer and removes any occurrences 
 * of the characters '#' and '$', storing the result in the output buffer. 
 * The output buffer is null-terminated after processing.
 *
 * @param[in] buffer The input character array from which '#' and '$' will be removed.
 * @param[out] output_buffer The output character array to store the result after removal of '#' and '$'.
 * @param output_buffer_sizeOutput Output buffer size to avoid overflow.
 * @return int 0 if the operation was successful, -1 if there was an error.
 */
bool idp_parser_remove_hashtag_cipher(const char *buffer, char *output_buffer, size_t output_buffer_size);

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
bool check_valid_characters(const char *buffer, uint8_t size);

/**
 * @brief Prepares GPS configuration message.
 *
 * This function prefixes the input buffer with control bytes (0x01 and 0x00),
 * and adds a null terminator at the end, preparing the GPS configuration message.
 *
 * @param buffer Input character array representing GPS configurations.
 * @param buffer_gps_config Output character array to store the prepared configuration message.
 */
void prepare_gps_config_message(const char *buffer, char *buffer_gps_config);

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
 * @param rush_config data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_04(const rush_mode_config rush_config);

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
 * @param virtual_config data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_22(const pivot_physical_config physical_config);

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param gps_config data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_23(const gps_config gps_config);

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param gps_config data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_24(const reboot_config reboot_config);

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param virtual_config data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_26(const pivot_virtual_config virtual_config);

/**
 * @brief Validate the specified configuration paramters.
 *
 * This function validates the specified configuration paramters to ensure they conform to the IDP protocol.
 *
 * @param comm_config data to be validated.
 * @return true if the data are valid, false otherwise.
 */
bool idp_parser_validate_idp_31(const pivot_comm_main_mode_config comm_config);

/**
 * @brief Build argument pairs for IDP 28 packet.
 *
 * This function builds argument pairs for the IDP 28 packet based on the specified parameters.
 *
 * @param arg_pair The argument pair to be built.
 * @param reason_str The reason string to be included in the packet.
 * @param idp_28 The IDP 28 packet to be built.
 * @param system_id The system ID to be included in the packet.
 * @param origin The optional origin to be included in the packet.
 * @param idp The IDP type to be included in the packet.
 * @param scheduling_id The optional scheduling ID to be included in the packet.
 * @param pivot_is_on_barrier The pivot barrier status to be included in the packet.
 * @param global_angle The global angle to be included in the packet.
 * @param str_date_time The date and time to be included in the packet.
 * @param is_external_agent The external agent status to be included in the packet.
 */
void idp_parser_build_arg_pairs_pivot_shutdown(
    arg_pair_t *arg_pair,
    char *reason_str,
    uint8_t *idp_28,
    char *system_id,
    char *origin,
    idp_type *idp,
    char *scheduling_id,
    bool *pivot_is_on_barrier,
    uint16_t *global_angle,
    char *str_date_time,
    bool is_external_agent
);

#endif /* COMPONENTS_IDP_PROTOCOL__INCLUDE_IDP_PARSER_H_ */
