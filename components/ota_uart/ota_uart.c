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
 * @param payload Output pointer to payload start.
 * @param payload_len Output payload length.
 * @return true on successful parse.
 */
static bool ota_uart_parse_data_frame(const char *frame, size_t *packet_id, const char **payload, size_t *payload_len)
{
    if (frame == NULL || packet_id == NULL || payload == NULL || payload_len == NULL)
    {
        return false;
    }

    if (strncmp(frame, OTA_FRAME_DATA_PREFIX, strlen(OTA_FRAME_DATA_PREFIX)) != 0)
    {
        return false;
    }

    const char *cursor = frame + strlen(OTA_FRAME_DATA_PREFIX);
    char *end_ptr = NULL;

    unsigned long parsed_packet_id = strtoul(cursor, &end_ptr, 10);
    if (end_ptr == cursor || *end_ptr != '-')
    {
        return false;
    }

    const char *payload_start = end_ptr + 1;
    const char *payload_end = strrchr(payload_start, '$');
    if (payload_end == NULL || *(payload_end + 1) != '\0')
    {
        return false;
    }

    if (payload_end <= payload_start)
    {
        return false;
    }

    *packet_id = (size_t)parsed_packet_id;
    *payload = payload_start;
    *payload_len = (size_t)(payload_end - payload_start);
    return true;
}

/**
 * @brief Decodes Base64 payload to binary buffer.
 *
 * @param payload Base64 payload pointer.
 * @param payload_len Payload length.
 * @param output_decoded Allocated output binary buffer.
 * @param output_decoded_len Output binary size.
 * @return true on successful decode.
 */
static bool ota_uart_decode_payload(const char *payload, size_t payload_len, uint8_t **output_decoded, size_t *output_decoded_len)
{
    if (payload == NULL || output_decoded == NULL || output_decoded_len == NULL)
    {
        return false;
    }

    if (payload_len == 0 || payload_len > ota_uart_rx_max_base64_payload_size)
    {
        return false;
    }

    size_t output_capacity = ((payload_len + 3U) / 4U) * 3U;
    if (output_capacity == 0 || output_capacity > ota_uart_rx_max_decoded_packet_size)
    {
        return false;
    }

    uint8_t *decoded_buffer = (uint8_t *)malloc(output_capacity);
    if (decoded_buffer == NULL)
    {
        return false;
    }

    size_t decoded_len = 0;
    int decode_status = mbedtls_base64_decode(decoded_buffer, output_capacity, &decoded_len,
                                              (const unsigned char *)payload, payload_len);
    if (decode_status != 0 || decoded_len == 0)
    {
        free(decoded_buffer);
        return false;
    }

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
    const char *payload = NULL;
    size_t payload_len = 0;

    if (!ota_state.transfer_active || ota_state.firmware_file == NULL)
    {
        ota_uart_send_nack(tx_callback, "no_active_transfer");
        return;
    }

    if (!ota_uart_parse_data_frame(frame, &packet_id, &payload, &payload_len))
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

    uint8_t *decoded_payload = NULL;
    size_t decoded_payload_len = 0;

    if (!ota_uart_decode_payload(payload, payload_len, &decoded_payload, &decoded_payload_len))
    {
        ota_uart_send_nack(tx_callback, "invalid_base64_payload");
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
