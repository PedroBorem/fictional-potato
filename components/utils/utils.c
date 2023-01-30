/*
 * log.h
 *
 *  Created on: 14 de jan. de 2023
 *      Author: brunolima
 */

/* Includes ----------------------------------------------------------------- */
// Standard
#include <string.h>
#include <stdbool.h>
// Self-include
#include "utils.h"

/* Private variables -------------------------------------------------------- */
static const char *hex_digits = "0123456789ABCDEF";

/* Public functions --------------------------------------------------------- */
/**
 * @brief Generate an hex string from a byte sequence
 * 
 * @param input             Pointer to input byte stream
 * @param len               Size of byte sequence
 * @return char*            Allocated hex string with letters uppercase
 */
char *generate_hex_string_from_byte_stream(uint8_t *input, size_t len)
{
    char *output = (char *) malloc(2 * len + 1);

    if (output != NULL) {
        for (int k = 0; k < len; k++) {
            output[2 * k + 0] = hex_digits[input[k] / 16];
            output[2 * k + 1] = hex_digits[input[k] % 16];
        }
        output[2 * len] = '\0';
    }

    return output;
}

/**
 * @brief Generate byte stream from hex sequence object
 * 
 * @param bytes_input       Pointer to input hex bytes
 * @param len_hex_sequence  Length of hex bytes (0 will handle bytes_input as string)
 * @return uint8_t*         Pointer to allocated byte stream (NULL on any input errors)
 */
uint8_t *generate_byte_stream_from_hex_sequence(char *bytes_input, size_t len_hex_sequence)
{
    uint8_t *bytes_output = NULL;

    // Flexible to string or stream
    if (len_hex_sequence == 0) {
        len_hex_sequence = strlen(bytes_input);
    }

    // Input validation
    if (len_hex_sequence % 2 == 0 && len_hex_sequence > 0) {
        // Output allocation
        size_t len_output = len_hex_sequence / 2;
        bytes_output = (uint8_t *) malloc(len_output);

        if (bytes_output != NULL) {
            // Output initialization
            memset(bytes_output, 0, len_output);

            // String sweep
            for (int k = 0; k < len_hex_sequence && bytes_output != NULL; k++) {
                bytes_output[k / 2] *= 16;
                if (bytes_input[k] >= '0' && bytes_input[k] <= '9') {
                    bytes_output[k / 2] += bytes_input[k] - '0';
                } else if (bytes_input[k] >= 'a' && bytes_input[k] <= 'f') {
                    bytes_output[k / 2] +=  bytes_input[k] - 'a' + 10;
                } else if (bytes_input[k] >= 'A' && bytes_input[k] <= 'F') {
                    bytes_output[k / 2] +=  bytes_input[k] - 'A' + 10;
                } else {
                    free(bytes_output);
                    bytes_output = NULL;
                }
            }
        }
    }

    return bytes_output;
}

/**
 * @brief Concatenates two strings into a new one
 * 
 * @param first             First string
 * @param second            Second string
 * @return char*            New allocated string
 */
char *dyn_strcat(char *first, const char *second)
{
    char *swap = NULL;

    if (first != NULL && second != NULL) {
        swap = (char *) malloc(strlen(first) + strlen(second) + 1);
        if (swap != NULL) {
            strcpy(swap, first);
            strcat(swap, second);
            free(first);
        }
    }

    return swap;
}

/**
 * @brief Converts a number into respective string
 * (using letters always in upper case)
 * 
 * @param value             Value to be converted
 * @param base              Numeric base (from 2 to 16)
 * @return char*            Allocated string with the result
 */
char *dyn_itoa(int64_t value, int base)
{
    char *output = NULL;

    if (base >= 2 && base <= 16) {
        // Calculate needed buffer size
        int size_buffer = 1; // 1 byte for '\0'
        int64_t calc = value;
        if (calc < 0) {
            // Negative must have the signal
            size_buffer++;
            calc = -calc;
        }
        do {
            size_buffer++;
            calc /= base;
        } while (calc > 0);

        // Allocate and fill buffer
        output = (char *) malloc(size_buffer);
        if (output != NULL) {
            output[size_buffer - 1] = '\0';
            calc = value;
            if (calc < 0) {
                // Negative must have the signal
                output[0] = '-';
                calc = -calc;
            }
            int pointer_buffer = size_buffer - 1;
            do {
                output[--pointer_buffer] = hex_digits[calc % base];
                calc /= base;
            } while (calc > 0);
        }
    }

    return output;
}

/**
 * @brief Converts a number represented as string into respective binary value
 * (using letters always in upper case)
 * 
 * @param byte_stream       Pointer to byte stream respresenting numeric value in ASCII
 * @param len               Length of byte stream. If zero, is considered like string
 * @param base              Numeric base
 * @return int64_t          Numeric result
 */
int64_t atoi_with_base(const char *byte_stream, size_t len, int base)
{
    int64_t output = 0;

    if (base >= 2 && base <= 16) {
        int pos = 0;
        bool is_negative = false;

        if (len == 0) {
            len = strlen(byte_stream);
        }

        if (byte_stream[pos] == '-') {
            pos++;
            is_negative = true;
        }

        while (pos < len) {
            output *= base;
            for (int k = 0; k < base; k++) {
                if (byte_stream[pos] == hex_digits[k]) {
                    output += k;
                }
            }
            pos++;
        }

        if (is_negative) {
            output = -output;
        }
    }

    return output;
}
