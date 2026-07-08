/**
 * @file idp_parser.h
 * @brief Generic parser and serializer for IDP text packets.
 */

#ifndef COMPONENTS_IDP_PROTOCOL_INCLUDE_IDP_PARSER_H_
#define COMPONENTS_IDP_PROTOCOL_INCLUDE_IDP_PARSER_H_

#include "project_config.h"

/**
 * @brief Type/value pair used by the generic packet parser and serializer.
 */
typedef struct arg_pair
{
    const char *type;
    void *value;
} arg_pair_t;

idp_type idp_parser_get(const char *string_in, char *string_out);
bool idp_parser_is_payload_from_idp(const char *payload, idp_type idp);
bool idp_parser_remove_hashtag_cipher(const char *buffer, char *output_buffer, size_t output_buffer_size);
void idp_parser_create_package(char *str_out, arg_pair_t arg_pairs[]);
void idp_parser_get_packet_data(const char *str_arg, arg_pair_t arg_pairs[]);
uint8_t idp_parser_get_delimiter(const char *buffer);
bool check_valid_characters(const char *buffer, uint8_t size);

#endif /* COMPONENTS_IDP_PROTOCOL_INCLUDE_IDP_PARSER_H_ */
