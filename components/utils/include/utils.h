/*
 * log.h
 *
 *  Created on: 14 de jan. de 2023
 *      Author: brunolima
 */

#ifndef _UTILS_INCLUDE_H_
#define _UTILS_INCLUDE_H_

/* Includes ----------------------------------------------------------------- */
#include <stddef.h>
#include <stdint.h>

/* Public definitions ------------------------------------------------------- */
// Inline operations
#define MSB_OF_SHORT_INT(arg)                   (((arg) & 0xFF00) >> 8)
#define LSB_OF_SHORT_INT(arg)                   ((arg) & 0x00FF)
#define BIT_SET(arg,bit)                        ((arg) |= (1<<(bit)))
#define BIT_CLR(arg,bit)                        ((arg) &= ~(1<<(bit)))
#define BIT_FLP(arg,bit)                        ((arg) ^= (1<<(bit)))
#define BIT_TST(arg,bit)                        ((arg) & (1<<(bit)))
#define COMPOSE_SHORTINT_WITH_CHARS(MsB,LsB)    ((((uint16_t) (MsB)) << 8) | (LsB))

// Values
#define BYTE_SIZE                               8

/* Public methods ----------------------------------------------------------- */
char *generate_hex_string_from_byte_stream(uint8_t *input, size_t len);
uint8_t *generate_byte_stream_from_hex_sequence(char *bytes_input, size_t len_hex_sequence);
char *dyn_strcat(char *first, const char *second);
char *dyn_itoa(int64_t value, int base);
int64_t atoi_with_base(const char *byte_stream, size_t len, int base);

#endif /* _UTILS_INCLUDE_H_ */
