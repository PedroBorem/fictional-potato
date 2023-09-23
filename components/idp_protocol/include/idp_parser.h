/*
 * idp_parser.h
 *
 *  Created on: 12 de jul. de 2023
 *      Author: bruno
 */

#ifndef COMPONENTS_IDP_PROTOCOL__INCLUDE_IDP_PARSER_H_
#define COMPONENTS_IDP_PROTOCOL__INCLUDE_IDP_PARSER_H_


#include "project_config.h"

typedef struct arg_pair{
    const char* type;
    void* value;
} arg_pair_t;

/**
 * @brief Parse the IDP packet and extract the IDP type.
 *
 * This function extracts the IDP type from the received IDP packet.
 *
 * @param[in] string_in The received IDP packet string.
 * @return The IDP type extracted from the packet, or IDP_INVALID if the packet is invalid.
 */
idp_type idp_parser_get(const char* string_in, char* string_out);

bool idp_parser_validate_actions(const pivot_actions actions);

uint16_t idp_parser_create_pwd(pivot_actions actions);


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

#endif /* COMPONENTS_IDP_PROTOCOL__INCLUDE_IDP_PARSER_H_ */
