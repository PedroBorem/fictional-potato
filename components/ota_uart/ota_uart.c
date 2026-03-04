/**
 * @file ota_uart.c
 * @brief OTA receiver over UART for control board firmware.
 */

#include "ota_uart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>

#include "mbedtls/base64.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_spiffs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @def OTA_UART_TAG
 * @brief Log tag for OTA UART module.
 */
#define OTA_UART_TAG "ota_uart"

/**
 * @def OTA_FRAME_PREFIX
 * @brief Common prefix used in OTA UART protocol.
 */
#define OTA_FRAME_PREFIX "#OTA-"

/**
 * @def OTA_FRAME_BEGIN_PREFIX
 * @brief Prefix for begin frame.
 */
#define OTA_FRAME_BEGIN_PREFIX "#OTA-BEGIN-"

/**
 * @def OTA_FRAME_DATA_PREFIX
 * @brief Prefix for data frame.
 */
#define OTA_FRAME_DATA_PREFIX "#OTA-DATA-"

/**
 * @def OTA_FRAME_END
 * @brief Exact end frame.
 */
#define OTA_FRAME_END "#OTA-END$"

/**
 * @def OTA_ACK_BEGIN
 * @brief ACK frame for begin command.
 */
#define OTA_ACK_BEGIN "#OTA-ACK-BEGIN$"

/**
 * @def OTA_ACK_END
 * @brief ACK frame for end command.
 */
#define OTA_ACK_END "#OTA-ACK-END$"

/**
 * @brief Maximum allowed packets in one transfer.
 *
 * This value avoids malformed begin frames creating unrealistic packet counters.
 */
#define OTA_UART_MAX_TOTAL_PACKETS (50000U)

/**
 * @brief Number of bytes used in ASCII previews for debug logs.
 */
#define OTA_UART_ASCII_PREVIEW_BYTES (24U)

/**
 * @brief Number of bytes used in hex previews for debug logs.
 */
#define OTA_UART_HEX_PREVIEW_BYTES (8U)

/**
 * @brief Runtime configuration for maximum firmware size.
 */
size_t ota_uart_rx_max_file_size_bytes = (4U * 1024U * 1024U);

/**
 * @brief Runtime configuration for maximum decoded packet size.
 */
size_t ota_uart_rx_max_decoded_packet_size = 2048U;

/**
 * @brief Runtime configuration for maximum Base64 payload length.
 */
size_t ota_uart_rx_max_base64_payload_size = 4096U;

/**
 * @brief OTA UART reception state.
 */
typedef struct
{
    bool storage_ready;
    bool transfer_active;
    bool transfer_complete;
    bool update_in_progress;
    size_t expected_file_size;
    size_t expected_packets;
    size_t next_packet_id;
    size_t received_size;
    FILE *firmware_file;
} ota_uart_state_t;

/**
 * @brief Module internal state instance.
 */
static ota_uart_state_t ota_state = {0};

/**
 * @brief Returns true when packet should produce detailed debug logs.
 *
 * @param packet_id Packet sequence number.
 * @return true for first packets and sparse checkpoints.
 */
static bool ota_uart_should_log_packet(size_t packet_id)
{
    return (packet_id < 3U || (packet_id % 250U) == 0U);
}

/**
 * @brief Copies head and tail previews from an ASCII buffer.
 *
 * @param input Input string.
 * @param input_len Input length.
 * @param preview_len Maximum bytes for each preview.
 * @param head_out Output head preview buffer.
 * @param head_out_size Head buffer size.
 * @param tail_out Output tail preview buffer.
 * @param tail_out_size Tail buffer size.
 */
static void ota_uart_copy_ascii_preview(const char *input, size_t input_len, size_t preview_len,
                                        char *head_out, size_t head_out_size,
                                        char *tail_out, size_t tail_out_size)
{
    if (head_out != NULL && head_out_size > 0U)
    {
        head_out[0] = '\0';
    }

    if (tail_out != NULL && tail_out_size > 0U)
    {
        tail_out[0] = '\0';
    }

    if (input == NULL || input_len == 0U)
    {
        return;
    }

    if (head_out != NULL && head_out_size > 1U)
    {
        size_t head_len = input_len;
        if (head_len > preview_len)
        {
            head_len = preview_len;
        }
        if (head_len >= head_out_size)
        {
            head_len = head_out_size - 1U;
        }

        memcpy(head_out, input, head_len);
        head_out[head_len] = '\0';
    }

    if (tail_out != NULL && tail_out_size > 1U)
    {
        size_t tail_len = input_len;
        if (tail_len > preview_len)
        {
            tail_len = preview_len;
        }
        if (tail_len >= tail_out_size)
        {
            tail_len = tail_out_size - 1U;
        }

        const char *tail_start = input + (input_len - tail_len);
        memcpy(tail_out, tail_start, tail_len);
        tail_out[tail_len] = '\0';
    }
}

/**
 * @brief Formats bytes to hexadecimal string for debug logs.
 *
 * @param data Input binary data.
 * @param data_len Number of bytes to format.
 * @param output Output string buffer.
 * @param output_size Output buffer size.
 */
static void ota_uart_format_hex_bytes(const uint8_t *data, size_t data_len, char *output, size_t output_size)
{
    if (output == NULL || output_size == 0U)
    {
        return;
    }

    output[0] = '\0';
    if (data == NULL || data_len == 0U || output_size < 3U)
    {
        return;
    }

    size_t write_index = 0U;
    for (size_t i = 0; i < data_len; i++)
    {
        const char *format = (i == 0U) ? "%02X" : " %02X";
        int written = snprintf(output + write_index, output_size - write_index, format, data[i]);
        if (written <= 0)
        {
            break;
        }

        size_t written_size = (size_t)written;
        if (written_size >= (output_size - write_index))
        {
            output[output_size - 1U] = '\0';
            break;
        }

        write_index += written_size;
    }
}

/**
 * @brief Returns true when character belongs to Base64 alphabet.
 *
 * Accepted set: A-Z, a-z, 0-9, '+', '/', '='.
 *
 * @param c Character to validate.
 * @return true for valid Base64 character.
 */
static bool ota_uart_is_base64_char(char c)
{
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
    {
        return true;
    }

    return (c == '+' || c == '/' || c == '=');
}

/**
 * @brief Safely closes current firmware file handle.
 */
static void ota_uart_close_file(void)
{
    if (ota_state.firmware_file != NULL)
    {
        fflush(ota_state.firmware_file);
        fclose(ota_state.firmware_file);
        ota_state.firmware_file = NULL;
    }
}

/**
 * @brief Resets transfer counters and runtime flags.
 *
 * @param clear_completion true to clear transfer_complete flag.
 */
static void ota_uart_reset_runtime_state(bool clear_completion)
{
    ota_state.transfer_active = false;
    if (clear_completion)
    {
        ota_state.transfer_complete = false;
    }
    ota_state.update_in_progress = false;

    ota_state.expected_file_size = 0;
    ota_state.expected_packets = 0;
    ota_state.next_packet_id = 0;
    ota_state.received_size = 0;
}

/**
 * @brief Applies firmware image saved in OTA UART storage file.
 *
 * @return esp_err_t ESP_OK when image was written and boot partition updated.
 */
static esp_err_t ota_uart_apply_saved_firmware(void)
{
    esp_err_t err = ESP_FAIL;
    FILE *firmware = fopen(OTA_UART_FIRMWARE_FILE_PATH, "rb");
    if (firmware == NULL)
    {
        ESP_LOGE(OTA_UART_TAG, "Failed to open firmware file for OTA apply");
        return ESP_ERR_NOT_FOUND;
    }

    const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
    if (partition == NULL)
    {
        ESP_LOGE(OTA_UART_TAG, "No OTA partition available in partition table");
        fclose(firmware);
        return ESP_ERR_NOT_FOUND;
    }

    esp_ota_handle_t ota_handle = 0;
    err = esp_ota_begin(partition, ota_state.expected_file_size, &ota_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(OTA_UART_TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        fclose(firmware);
        return err;
    }

    uint8_t buffer[2048] = {0};
    size_t total_written = 0;

    while (total_written < ota_state.expected_file_size)
    {
        size_t bytes_to_read = ota_state.expected_file_size - total_written;
        if (bytes_to_read > sizeof(buffer))
        {
            bytes_to_read = sizeof(buffer);
        }

        size_t read_len = fread(buffer, 1, bytes_to_read, firmware);
        if (read_len != bytes_to_read)
        {
            ESP_LOGE(OTA_UART_TAG, "Failed reading saved firmware while applying OTA");
            esp_ota_end(ota_handle);
            fclose(firmware);
            return ESP_FAIL;
        }

        err = esp_ota_write(ota_handle, buffer, read_len);
        if (err != ESP_OK)
        {
            ESP_LOGE(OTA_UART_TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
            esp_ota_end(ota_handle);
            fclose(firmware);
            return err;
        }

        total_written += read_len;
    }

    fclose(firmware);

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(OTA_UART_TAG, "esp_ota_end failed (%s)", esp_err_to_name(err));
        return err;
    }

    err = esp_ota_set_boot_partition(partition);
    if (err != ESP_OK)
    {
        ESP_LOGE(OTA_UART_TAG, "esp_ota_set_boot_partition failed (%s)", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

/**
 * @brief Sends one frame through registered UART TX callback.
 *
 * @param tx_callback Callback function.
 * @param frame Null-terminated frame.
 */
static void ota_uart_send_frame(ota_uart_tx_callback_t tx_callback, const char *frame)
{
    if (tx_callback == NULL || frame == NULL)
    {
        return;
    }

    if (tx_callback(frame, strlen(frame)) != ESP_OK)
    {
        ESP_LOGE(OTA_UART_TAG, "Failed to send frame: %s", frame);
    }
}

/**
 * @brief Sends ACK frame for packet id.
 *
 * @param tx_callback Callback function.
 * @param packet_id Packet id to acknowledge.
 */
static void ota_uart_send_ack_packet(ota_uart_tx_callback_t tx_callback, size_t packet_id)
{
    char ack_frame[40] = {0};
    snprintf(ack_frame, sizeof(ack_frame), "#OTA-ACK-%" PRIu32 "$", (uint32_t)packet_id);
    ota_uart_send_frame(tx_callback, ack_frame);
}

/**
 * @brief Sends generic NACK frame.
 *
 * @param tx_callback Callback function.
 * @param reason Reason string used in frame.
 */
static void ota_uart_send_nack(ota_uart_tx_callback_t tx_callback, const char *reason)
{
    char nack_frame[96] = {0};
    snprintf(nack_frame, sizeof(nack_frame), "#OTA-NACK-%s$", reason);
    ota_uart_send_frame(tx_callback, nack_frame);
}

/**
 * @brief Computes reflected CRC32 with configurable seed and final XOR.
 *
 * @param data Input buffer.
 * @param length Buffer length.
 * @param initial_crc Initial CRC accumulator.
 * @param apply_final_xor true to apply final bitwise NOT.
 * @return uint32_t CRC result.
 */
static uint32_t ota_uart_crc32_variant(const uint8_t *data, size_t length, uint32_t initial_crc, bool apply_final_xor)
{
    uint32_t crc = initial_crc;

    for (size_t i = 0; i < length; i++)
    {
        crc ^= data[i];

        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if ((crc & 1U) != 0U)
            {
                crc = (crc >> 1U) ^ 0xEDB88320U;
            }
            else
            {
                crc >>= 1U;
            }
        }
    }

    if (apply_final_xor)
    {
        return ~crc;
    }

    return crc;
}

/**
 * @brief Swaps byte order for 32-bit values.
 *
 * @param value Input value.
 * @return uint32_t Byte-swapped value.
 */
static uint32_t ota_uart_bswap32(uint32_t value)
{
    return ((value & 0x000000FFU) << 24U) |
           ((value & 0x0000FF00U) << 8U) |
           ((value & 0x00FF0000U) >> 8U) |
           ((value & 0xFF000000U) >> 24U);
}

static uint32_t ota_uart_crc32(const uint8_t *data, size_t length)
{
    return ota_uart_crc32_variant(data, length, 0xFFFFFFFFU, true);
}

/**
 * @brief Logs parse failure details for OTA DATA frames.
 *
 * @param reason Human-readable parse failure reason.
 * @param frame Input frame snapshot.
 */
static void ota_uart_log_data_frame_parse_error(const char *reason, const char *frame)
{
    const char *safe_reason = (reason != NULL) ? reason : "unknown";
    size_t frame_len = 0U;
    char frame_head[OTA_UART_ASCII_PREVIEW_BYTES + 1U] = {0};
    char frame_tail[OTA_UART_ASCII_PREVIEW_BYTES + 1U] = {0};

    if (frame != NULL)
    {
        frame_len = strlen(frame);
        ota_uart_copy_ascii_preview(frame, frame_len, OTA_UART_ASCII_PREVIEW_BYTES,
                                    frame_head, sizeof(frame_head),
                                    frame_tail, sizeof(frame_tail));
    }

    ESP_LOGW(OTA_UART_TAG,
             "Invalid OTA DATA frame (%s). frame_len=%u head='%s' tail='%s'",
             safe_reason,
             (unsigned int)frame_len,
             frame_head,
             frame_tail);
}

/**
 * @brief Parses begin frame and extracts metadata.
 *
 * @param frame Input frame.
 * @param file_size Output expected file size.
 * @param total_packets Output expected packet count.
 * @return true on successful parse.
 */
static bool ota_uart_parse_begin_frame(const char *frame, size_t *file_size, size_t *total_packets)
{
    if (frame == NULL || file_size == NULL || total_packets == NULL)
    {
        return false;
    }

    if (strncmp(frame, OTA_FRAME_BEGIN_PREFIX, strlen(OTA_FRAME_BEGIN_PREFIX)) != 0)
    {
        return false;
    }

    const char *cursor = frame + strlen(OTA_FRAME_BEGIN_PREFIX);
    char *end_ptr = NULL;

    unsigned long parsed_file_size = strtoul(cursor, &end_ptr, 10);
    if (end_ptr == cursor || *end_ptr != '-')
    {
        return false;
    }

    cursor = end_ptr + 1;
    unsigned long parsed_total_packets = strtoul(cursor, &end_ptr, 10);
    if (end_ptr == cursor || *end_ptr != '$' || *(end_ptr + 1) != '\0')
    {
        return false;
    }

    if (parsed_file_size == 0 || parsed_total_packets == 0)
    {
        return false;
    }

    *file_size = (size_t)parsed_file_size;
    *total_packets = (size_t)parsed_total_packets;
    return true;
}

/**
 * @brief Parses data frame header and payload boundaries.
 *
 * @param frame Input frame.
 * @param packet_id Output packet id.
 * @param expected_crc32 Output expected CRC32 for decoded payload.
 * @param payload Output pointer to payload start.
 * @param payload_len Output payload length.
 * @return true on successful parse.
 */
static bool ota_uart_parse_data_frame(const char *frame, size_t *packet_id, uint32_t *expected_crc32,
                                     const char **payload, size_t *payload_len)
{
    if (frame == NULL || packet_id == NULL || expected_crc32 == NULL || payload == NULL || payload_len == NULL)
    {
        ota_uart_log_data_frame_parse_error("null_argument", frame);
        return false;
    }

    if (strncmp(frame, OTA_FRAME_DATA_PREFIX, strlen(OTA_FRAME_DATA_PREFIX)) != 0)
    {
        ota_uart_log_data_frame_parse_error("invalid_prefix", frame);
        return false;
    }

    const char *cursor = frame + strlen(OTA_FRAME_DATA_PREFIX);
    char *end_ptr = NULL;

    unsigned long parsed_packet_id = strtoul(cursor, &end_ptr, 10);
    if (end_ptr == cursor || *end_ptr != '-')
    {
        ota_uart_log_data_frame_parse_error("invalid_packet_id", frame);
        return false;
    }

    cursor = end_ptr + 1;
    unsigned long parsed_crc32 = strtoul(cursor, &end_ptr, 10);
    if (end_ptr == cursor || *end_ptr != '-')
    {
        ota_uart_log_data_frame_parse_error("invalid_crc_field", frame);
        return false;
    }

    const char *payload_start = end_ptr + 1;
    const char *payload_end = strrchr(payload_start, '$');
    if (payload_end == NULL || *(payload_end + 1) != '\0')
    {
        ota_uart_log_data_frame_parse_error("missing_frame_terminator", frame);
        return false;
    }

    if (payload_end <= payload_start)
    {
        ota_uart_log_data_frame_parse_error("empty_payload", frame);
        return false;
    }

    *packet_id = (size_t)parsed_packet_id;
    *expected_crc32 = (uint32_t)parsed_crc32;
    *payload = payload_start;
    *payload_len = (size_t)(payload_end - payload_start);
    return true;
}

/**
 * @brief Decodes Base64 payload to binary buffer.
 *
 * @param packet_id Packet id related to payload.
 * @param payload Base64 payload pointer.
 * @param payload_len Payload length.
 * @param output_decoded Allocated output binary buffer.
 * @param output_decoded_len Output binary size.
 * @return true on successful decode.
 */
static bool ota_uart_decode_payload(size_t packet_id, const char *payload, size_t payload_len,
                                    uint8_t **output_decoded, size_t *output_decoded_len)
{
    if (payload == NULL || output_decoded == NULL || output_decoded_len == NULL)
    {
        ESP_LOGW(OTA_UART_TAG, "Base64 decode skipped due to null argument (packet id=%u)", (unsigned int)packet_id);
        return false;
    }

    if (payload_len == 0 || payload_len > ota_uart_rx_max_base64_payload_size)
    {
        ESP_LOGW(OTA_UART_TAG,
                 "Base64 decode rejected by payload length: packet id=%u payload_len=%u max_allowed=%u",
                 (unsigned int)packet_id,
                 (unsigned int)payload_len,
                 (unsigned int)ota_uart_rx_max_base64_payload_size);
        return false;
    }

    /* Normalize payload to support URL-safe variants and ignore accidental whitespaces. */
    size_t normalized_capacity = payload_len + 4U;
    char *normalized_payload = (char *)malloc(normalized_capacity + 1U);
    if (normalized_payload == NULL)
    {
        return false;
    }

    size_t normalized_len = 0U;
    size_t ignored_whitespace = 0U;
    size_t replaced_dash_count = 0U;
    size_t replaced_underscore_count = 0U;
    for (size_t i = 0; i < payload_len; i++)
    {
        char current = payload[i];

        if (current == '\r' || current == '\n' || current == '\t' || current == ' ')
        {
            ignored_whitespace++;
            continue;
        }

        if (current == '-')
        {
            current = '+';
            replaced_dash_count++;
        }
        else if (current == '_')
        {
            current = '/';
            replaced_underscore_count++;
        }

        normalized_payload[normalized_len++] = current;
    }

    size_t added_padding_count = 0U;
    while ((normalized_len % 4U) != 0U && normalized_len < normalized_capacity)
    {
        normalized_payload[normalized_len++] = '=';
        added_padding_count++;
    }
    normalized_payload[normalized_len] = '\0';

    char normalized_head[OTA_UART_ASCII_PREVIEW_BYTES + 1U] = {0};
    char normalized_tail[OTA_UART_ASCII_PREVIEW_BYTES + 1U] = {0};
    ota_uart_copy_ascii_preview(normalized_payload, normalized_len, OTA_UART_ASCII_PREVIEW_BYTES,
                                normalized_head, sizeof(normalized_head),
                                normalized_tail, sizeof(normalized_tail));

    if (ota_uart_should_log_packet(packet_id))
    {
        ESP_LOGI(OTA_UART_TAG,
                 "Base64 normalize packet id=%u raw_len=%u normalized_len=%u ignored_ws=%u replaced_dash=%u "
                 "replaced_underscore=%u added_padding=%u head='%s' tail='%s'",
                 (unsigned int)packet_id,
                 (unsigned int)payload_len,
                 (unsigned int)normalized_len,
                 (unsigned int)ignored_whitespace,
                 (unsigned int)replaced_dash_count,
                 (unsigned int)replaced_underscore_count,
                 (unsigned int)added_padding_count,
                 normalized_head,
                 normalized_tail);
    }

    size_t output_capacity = ((normalized_len + 3U) / 4U) * 3U;
    if (output_capacity == 0 || output_capacity > ota_uart_rx_max_decoded_packet_size)
    {
        ESP_LOGW(OTA_UART_TAG,
                 "Base64 decode rejected by output capacity: packet id=%u normalized_len=%u output_capacity=%u max_decoded=%u "
                 "ignored_ws=%u replaced_dash=%u replaced_underscore=%u added_padding=%u head='%s' tail='%s'",
                 (unsigned int)packet_id,
                 (unsigned int)normalized_len,
                 (unsigned int)output_capacity,
                 (unsigned int)ota_uart_rx_max_decoded_packet_size,
                 (unsigned int)ignored_whitespace,
                 (unsigned int)replaced_dash_count,
                 (unsigned int)replaced_underscore_count,
                 (unsigned int)added_padding_count,
                 normalized_head,
                 normalized_tail);
        free(normalized_payload);
        return false;
    }

    uint8_t *decoded_buffer = (uint8_t *)malloc(output_capacity);
    if (decoded_buffer == NULL)
    {
        free(normalized_payload);
        return false;
    }

    int invalid_base64_index = -1;
    for (size_t i = 0; i < normalized_len; i++)
    {
        if (!ota_uart_is_base64_char(normalized_payload[i]))
        {
            invalid_base64_index = (int)i;
            break;
        }
    }

    size_t decoded_len = 0;
    int decode_status = mbedtls_base64_decode(decoded_buffer, output_capacity, &decoded_len,
                                              (const unsigned char *)normalized_payload, normalized_len);
    if (decode_status != 0 || decoded_len == 0)
    {
        char invalid_context[OTA_UART_ASCII_PREVIEW_BYTES + 1U] = {0};
        if (invalid_base64_index >= 0)
        {
            size_t context_start = 0U;
            if ((size_t)invalid_base64_index > (OTA_UART_ASCII_PREVIEW_BYTES / 2U))
            {
                context_start = (size_t)invalid_base64_index - (OTA_UART_ASCII_PREVIEW_BYTES / 2U);
            }

            size_t context_len = normalized_len - context_start;
            if (context_len > OTA_UART_ASCII_PREVIEW_BYTES)
            {
                context_len = OTA_UART_ASCII_PREVIEW_BYTES;
            }

            if (context_len > 0U)
            {
                memcpy(invalid_context, normalized_payload + context_start, context_len);
                invalid_context[context_len] = '\0';
            }
        }

        if (invalid_base64_index >= 0)
        {
            ESP_LOGW(OTA_UART_TAG,
                     "Base64 decode failed: packet id=%u status=%d payload_len=%u normalized_len=%u invalid_char_index=%d invalid_char=0x%02X "
                     "ignored_ws=%u replaced_dash=%u replaced_underscore=%u added_padding=%u head='%s' tail='%s' invalid_context='%s'",
                     (unsigned int)packet_id,
                     decode_status,
                     (unsigned int)payload_len,
                     (unsigned int)normalized_len,
                     invalid_base64_index,
                     (unsigned int)(uint8_t)normalized_payload[invalid_base64_index],
                     (unsigned int)ignored_whitespace,
                     (unsigned int)replaced_dash_count,
                     (unsigned int)replaced_underscore_count,
                     (unsigned int)added_padding_count,
                     normalized_head,
                     normalized_tail,
                     invalid_context);
        }
        else
        {
            ESP_LOGW(OTA_UART_TAG,
                     "Base64 decode failed: packet id=%u status=%d payload_len=%u normalized_len=%u ignored_ws=%u replaced_dash=%u "
                     "replaced_underscore=%u added_padding=%u head='%s' tail='%s'",
                     (unsigned int)packet_id,
                     decode_status,
                     (unsigned int)payload_len,
                     (unsigned int)normalized_len,
                     (unsigned int)ignored_whitespace,
                     (unsigned int)replaced_dash_count,
                     (unsigned int)replaced_underscore_count,
                     (unsigned int)added_padding_count,
                     normalized_head,
                     normalized_tail);
        }

        free(decoded_buffer);
        free(normalized_payload);
        return false;
    }

    free(normalized_payload);
    *output_decoded = decoded_buffer;
    *output_decoded_len = decoded_len;
    return true;
}

/**
 * @brief Handles begin frame processing.
 *
 * @param frame Input frame.
 * @param tx_callback Callback for ACK/NACK.
 */
static void ota_uart_handle_begin_frame(const char *frame, ota_uart_tx_callback_t tx_callback)
{
    size_t file_size = 0;
    size_t total_packets = 0;

    if (!ota_state.storage_ready)
    {
        ota_uart_send_nack(tx_callback, "storage_not_ready");
        return;
    }

    if (!ota_uart_parse_begin_frame(frame, &file_size, &total_packets))
    {
        ota_uart_send_nack(tx_callback, "invalid_begin");
        return;
    }

    if (file_size > ota_uart_rx_max_file_size_bytes)
    {
        ota_uart_send_nack(tx_callback, "file_too_large");
        return;
    }

    if (total_packets > OTA_UART_MAX_TOTAL_PACKETS)
    {
        ota_uart_send_nack(tx_callback, "invalid_total_packets");
        return;
    }

    if (ota_state.transfer_active &&
        ota_state.next_packet_id == 0 &&
        ota_state.expected_file_size == file_size &&
        ota_state.expected_packets == total_packets)
    {
        ota_uart_send_frame(tx_callback, OTA_ACK_BEGIN);
        return;
    }

    ota_uart_close_file();
    ota_uart_reset_runtime_state(true);

    remove(OTA_UART_FIRMWARE_FILE_PATH);

    ota_state.firmware_file = fopen(OTA_UART_FIRMWARE_FILE_PATH, "wb");
    if (ota_state.firmware_file == NULL)
    {
        ota_uart_send_nack(tx_callback, "file_open_error");
        return;
    }

    ota_state.expected_file_size = file_size;
    ota_state.expected_packets = total_packets;
    ota_state.next_packet_id = 0;
    ota_state.received_size = 0;
    ota_state.transfer_active = true;

    ESP_LOGI(OTA_UART_TAG, "Begin OTA UART transfer. file_size=%u bytes, packets=%u",
             (unsigned int)file_size, (unsigned int)total_packets);
    ota_uart_send_frame(tx_callback, OTA_ACK_BEGIN);
}

/**
 * @brief Handles data frame processing.
 *
 * @param frame Input frame.
 * @param tx_callback Callback for ACK/NACK.
 */
static void ota_uart_handle_data_frame(const char *frame, ota_uart_tx_callback_t tx_callback)
{
    size_t packet_id = 0;
    uint32_t expected_crc32 = 0U;
    const char *payload = NULL;
    size_t payload_len = 0;

    if (!ota_state.transfer_active || ota_state.firmware_file == NULL)
    {
        ota_uart_send_nack(tx_callback, "no_active_transfer");
        return;
    }

    if (!ota_uart_parse_data_frame(frame, &packet_id, &expected_crc32, &payload, &payload_len))
    {
        ota_uart_send_nack(tx_callback, "invalid_data");
        return;
    }

    if (ota_state.next_packet_id > 0 && packet_id == (ota_state.next_packet_id - 1U))
    {
        ota_uart_send_ack_packet(tx_callback, packet_id);
        return;
    }

    if (packet_id != ota_state.next_packet_id)
    {
        ota_uart_send_nack(tx_callback, "out_of_order");
        return;
    }

    size_t frame_len = strlen(frame);
    char payload_head[OTA_UART_ASCII_PREVIEW_BYTES + 1U] = {0};
    char payload_tail[OTA_UART_ASCII_PREVIEW_BYTES + 1U] = {0};
    ota_uart_copy_ascii_preview(payload, payload_len, OTA_UART_ASCII_PREVIEW_BYTES,
                                payload_head, sizeof(payload_head),
                                payload_tail, sizeof(payload_tail));

    if (ota_uart_should_log_packet(packet_id))
    {
        ESP_LOGI(OTA_UART_TAG,
                 "RX data packet id=%u expected_next=%u frame_len=%u payload_len=%u crc_dec=%" PRIu32 " crc_hex=0x%08" PRIX32
                 " payload_head='%s' payload_tail='%s'",
                 (unsigned int)packet_id,
                 (unsigned int)ota_state.next_packet_id,
                 (unsigned int)frame_len,
                 (unsigned int)payload_len,
                 expected_crc32,
                 expected_crc32,
                 payload_head,
                 payload_tail);
    }

    uint8_t *decoded_payload = NULL;
    size_t decoded_payload_len = 0;

    if (!ota_uart_decode_payload(packet_id, payload, payload_len, &decoded_payload, &decoded_payload_len))
    {
        ESP_LOGW(OTA_UART_TAG,
                 "Invalid Base64 payload on packet id=%u frame_len=%u payload_len=%u payload_head='%s' payload_tail='%s'",
                 (unsigned int)packet_id,
                 (unsigned int)frame_len,
                 (unsigned int)payload_len,
                 payload_head,
                 payload_tail);
        ota_uart_send_nack(tx_callback, "invalid_base64_payload");
        return;
    }

    if (ota_uart_should_log_packet(packet_id))
    {
        char decoded_head_hex[(OTA_UART_HEX_PREVIEW_BYTES * 3U) + 1U] = {0};
        char decoded_tail_hex[(OTA_UART_HEX_PREVIEW_BYTES * 3U) + 1U] = {0};

        size_t head_len = decoded_payload_len;
        if (head_len > OTA_UART_HEX_PREVIEW_BYTES)
        {
            head_len = OTA_UART_HEX_PREVIEW_BYTES;
        }

        size_t tail_len = decoded_payload_len;
        if (tail_len > OTA_UART_HEX_PREVIEW_BYTES)
        {
            tail_len = OTA_UART_HEX_PREVIEW_BYTES;
        }

        const uint8_t *tail_ptr = decoded_payload + (decoded_payload_len - tail_len);
        ota_uart_format_hex_bytes(decoded_payload, head_len, decoded_head_hex, sizeof(decoded_head_hex));
        ota_uart_format_hex_bytes(tail_ptr, tail_len, decoded_tail_hex, sizeof(decoded_tail_hex));

        ESP_LOGI(OTA_UART_TAG,
                 "Decoded packet id=%u decoded_len=%u decoded_head_hex=%s decoded_tail_hex=%s",
                 (unsigned int)packet_id,
                 (unsigned int)decoded_payload_len,
                 decoded_head_hex,
                 decoded_tail_hex);
    }

    uint32_t calculated_crc32 = ota_uart_crc32(decoded_payload, decoded_payload_len);
    if (calculated_crc32 != expected_crc32)
    {
        char decoded_head_hex[(OTA_UART_HEX_PREVIEW_BYTES * 3U) + 1U] = {0};
        char decoded_tail_hex[(OTA_UART_HEX_PREVIEW_BYTES * 3U) + 1U] = {0};

        size_t head_len = decoded_payload_len;
        if (head_len > OTA_UART_HEX_PREVIEW_BYTES)
        {
            head_len = OTA_UART_HEX_PREVIEW_BYTES;
        }

        size_t tail_len = decoded_payload_len;
        if (tail_len > OTA_UART_HEX_PREVIEW_BYTES)
        {
            tail_len = OTA_UART_HEX_PREVIEW_BYTES;
        }

        const uint8_t *tail_ptr = decoded_payload + (decoded_payload_len - tail_len);
        ota_uart_format_hex_bytes(decoded_payload, head_len, decoded_head_hex, sizeof(decoded_head_hex));
        ota_uart_format_hex_bytes(tail_ptr, tail_len, decoded_tail_hex, sizeof(decoded_tail_hex));

        uint32_t no_final_xor_crc = ota_uart_crc32_variant(decoded_payload, decoded_payload_len, 0xFFFFFFFFU, false);
        uint32_t init_zero_crc = ota_uart_crc32_variant(decoded_payload, decoded_payload_len, 0x00000000U, true);
        uint32_t init_zero_no_final_crc = ota_uart_crc32_variant(decoded_payload, decoded_payload_len, 0x00000000U, false);
        uint32_t swapped_crc = ota_uart_bswap32(calculated_crc32);

        ESP_LOGW(OTA_UART_TAG,
                 "CRC mismatch packet=%u decoded_len=%u expected=%" PRIu32 " (0x%08" PRIX32 ") "
                 "got=%" PRIu32 " (0x%08" PRIX32 ") bswap=0x%08" PRIX32
                 " no_final_xor=0x%08" PRIX32 " init0=0x%08" PRIX32 " init0_no_final=0x%08" PRIX32
                 " decoded_head_hex=%s decoded_tail_hex=%s",
                 (unsigned int)packet_id,
                 (unsigned int)decoded_payload_len,
                 expected_crc32,
                 expected_crc32,
                 calculated_crc32,
                 calculated_crc32,
                 swapped_crc,
                 no_final_xor_crc,
                 init_zero_crc,
                 init_zero_no_final_crc,
                 decoded_head_hex,
                 decoded_tail_hex);
        free(decoded_payload);
        ota_uart_send_nack(tx_callback, "crc_mismatch");
        return;
    }

    if ((ota_state.received_size + decoded_payload_len) > ota_state.expected_file_size)
    {
        free(decoded_payload);
        ota_uart_send_nack(tx_callback, "size_overflow");
        return;
    }

    size_t written = fwrite(decoded_payload, 1, decoded_payload_len, ota_state.firmware_file);
    free(decoded_payload);

    if (written != decoded_payload_len)
    {
        ota_uart_close_file();
        ota_uart_reset_runtime_state(false);
        ota_uart_send_nack(tx_callback, "storage_write_error");
        return;
    }

    fflush(ota_state.firmware_file);

    ota_state.received_size += written;
    ota_state.next_packet_id++;

    ota_uart_send_ack_packet(tx_callback, packet_id);
}

/**
 * @brief Handles end frame processing.
 *
 * @param tx_callback Callback for ACK/NACK.
 */
static void ota_uart_handle_end_frame(ota_uart_tx_callback_t tx_callback)
{
    if (!ota_state.transfer_active || ota_state.firmware_file == NULL)
    {
        if (ota_state.transfer_complete)
        {
            ota_uart_send_frame(tx_callback, OTA_ACK_END);
            return;
        }

        ota_uart_send_nack(tx_callback, "no_active_transfer");
        return;
    }

    if (ota_state.next_packet_id != ota_state.expected_packets ||
        ota_state.received_size != ota_state.expected_file_size)
    {
        ota_uart_send_nack(tx_callback, "transfer_incomplete");
        return;
    }

    ota_uart_close_file();
    ota_state.transfer_active = false;
    ota_state.transfer_complete = true;

    ESP_LOGI(OTA_UART_TAG, "OTA UART transfer completed. saved_size=%u",
             (unsigned int)ota_state.received_size);
    ota_uart_send_frame(tx_callback, OTA_ACK_END);

    if (ota_state.update_in_progress)
    {
        return;
    }

    ota_state.update_in_progress = true;
    esp_err_t ota_err = ota_uart_apply_saved_firmware();
    if (ota_err != ESP_OK)
    {
        ESP_LOGE(OTA_UART_TAG, "Failed to apply saved firmware (%s)", esp_err_to_name(ota_err));
        ota_uart_send_nack(tx_callback, "update_apply_failed");
        ota_state.update_in_progress = false;
        ota_state.transfer_complete = false;
        return;
    }

    remove(OTA_UART_FIRMWARE_FILE_PATH);

    ESP_LOGW(OTA_UART_TAG, "Firmware applied. Rebooting in 3 seconds...");
    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_restart();
}

/**
 * @brief Initializes OTA UART storage.
 *
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t ota_uart_init(void)
{
    if (ota_state.storage_ready)
    {
        return ESP_OK;
    }

    esp_vfs_spiffs_conf_t conf = {
        .base_path = OTA_UART_STORAGE_BASE_PATH,
        .partition_label = OTA_UART_STORAGE_PARTITION_LABEL,
        .max_files = 3,
        .format_if_mount_failed = true};

    esp_err_t err = esp_vfs_spiffs_register(&conf);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(OTA_UART_TAG, "Failed to mount OTA UART storage (err=%s)", esp_err_to_name(err));
        return err;
    }

    ota_state.storage_ready = true;
    ota_uart_reset_runtime_state(true);
    ota_uart_close_file();
    remove(OTA_UART_FIRMWARE_FILE_PATH);

    size_t total_bytes = 0;
    size_t used_bytes = 0;
    if (esp_spiffs_info(OTA_UART_STORAGE_PARTITION_LABEL, &total_bytes, &used_bytes) == ESP_OK)
    {
        ESP_LOGI(OTA_UART_TAG, "Storage mounted. total=%u used=%u",
                 (unsigned int)total_bytes, (unsigned int)used_bytes);
    }

    return ESP_OK;
}

/**
 * @brief Checks if frame belongs to OTA UART protocol.
 *
 * @param frame Input frame.
 * @return true for OTA frames.
 */
bool ota_uart_is_protocol_frame(const char *frame)
{
    if (frame == NULL)
    {
        return false;
    }

    return (strncmp(frame, OTA_FRAME_PREFIX, strlen(OTA_FRAME_PREFIX)) == 0);
}

/**
 * @brief Handles one OTA UART frame.
 *
 * @param frame Input frame.
 * @param tx_callback Callback for ACK/NACK.
 * @return true if frame is OTA protocol.
 */
bool ota_uart_handle_frame(const char *frame, ota_uart_tx_callback_t tx_callback)
{
    if (!ota_uart_is_protocol_frame(frame))
    {
        return false;
    }

    if (strcmp(frame, OTA_FRAME_END) == 0)
    {
        ota_uart_handle_end_frame(tx_callback);
        return true;
    }

    if (strncmp(frame, OTA_FRAME_BEGIN_PREFIX, strlen(OTA_FRAME_BEGIN_PREFIX)) == 0)
    {
        ota_uart_handle_begin_frame(frame, tx_callback);
        return true;
    }

    if (strncmp(frame, OTA_FRAME_DATA_PREFIX, strlen(OTA_FRAME_DATA_PREFIX)) == 0)
    {
        ota_uart_handle_data_frame(frame, tx_callback);
        return true;
    }

    ota_uart_send_nack(tx_callback, "invalid_command");
    return true;
}

/**
 * @brief Returns completed transfer state.
 *
 * @return true if transfer completed successfully.
 */
bool ota_uart_has_completed_transfer(void)
{
    return ota_state.transfer_complete;
}

/**
 * @brief Returns currently received size.
 *
 * @return size_t Number of bytes written in current or last transfer.
 */
size_t ota_uart_get_received_size(void)
{
    return ota_state.received_size;
}

/**
 * @brief Returns firmware storage file path.
 *
 * @return const char* Firmware file absolute path.
 */
const char *ota_uart_get_firmware_path(void)
{
    return OTA_UART_FIRMWARE_FILE_PATH;
}

/**
 * @brief Clears transfer state and removes stored firmware file.
 */
void ota_uart_reset_transfer(void)
{
    ota_uart_close_file();
    ota_uart_reset_runtime_state(true);
    remove(OTA_UART_FIRMWARE_FILE_PATH);
}
