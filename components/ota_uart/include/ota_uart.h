/**
 * @file ota_uart.h
 * @brief OTA receiver over UART for control board firmware.
 *
 * This module receives OTA frames over UART, persists the binary in flash
 * storage incrementally, and replies with ACK/NACK frames compatible with the
 * modem sender protocol.
 */

#ifndef COMPONENTS_OTA_UART_INCLUDE_OTA_UART_H_
#define COMPONENTS_OTA_UART_INCLUDE_OTA_UART_H_

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

/**
 * @brief Base path used to mount OTA storage filesystem.
 */
#define OTA_UART_STORAGE_BASE_PATH "/ota"

/**
 * @brief Partition label used for OTA storage filesystem.
 */
#define OTA_UART_STORAGE_PARTITION_LABEL "ota_storage"

/**
 * @brief Absolute path of firmware file received over UART.
 */
#define OTA_UART_FIRMWARE_FILE_PATH "/ota/board_firmware.bin"

/**
 * @brief Maximum allowed firmware file size (in bytes) for UART reception.
 *
 * This value is runtime-configurable and can be changed before receiving a transfer.
 */
extern size_t ota_uart_rx_max_file_size_bytes;

/**
 * @brief Maximum decoded bytes accepted per OTA UART data packet.
 *
 * This value is runtime-configurable and controls the largest binary packet
 * allowed after Base64 decoding.
 */
extern size_t ota_uart_rx_max_decoded_packet_size;

/**
 * @brief Maximum Base64 payload length accepted per OTA UART data packet.
 *
 * This value is runtime-configurable and controls the largest encoded payload
 * accepted in the `#OTA-DATA-<id>-<payload>$` frame.
 */
extern size_t ota_uart_rx_max_base64_payload_size;

/**
 * @brief Callback signature to transmit ACK/NACK frames over UART.
 *
 * @param event Null-terminated frame to transmit.
 * @param event_size Frame size in bytes.
 * @return esp_err_t ESP_OK on successful write operation.
 */
typedef esp_err_t (*ota_uart_tx_callback_t)(const char *event, size_t event_size);

/**
 * @brief Initializes OTA UART receiver storage and internal state.
 *
 * Mounts SPIFFS partition labeled `ota_storage` in `OTA_UART_STORAGE_BASE_PATH`.
 *
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t ota_uart_init(void);

/**
 * @brief Checks if a frame belongs to OTA UART protocol.
 *
 * @param frame Input frame.
 * @return true if frame starts with `#OTA-`.
 * @return false otherwise.
 */
bool ota_uart_is_protocol_frame(const char *frame);

/**
 * @brief Handles one complete UART frame for OTA transfer.
 *
 * Supported frames:
 * - `#OTA-BEGIN-<file_size>-<total_packets>$`
 * - `#OTA-DATA-<packet_id>-<base64_payload>$`
 * - `#OTA-END$`
 *
 * ACK/NACK responses are sent through `tx_callback`.
 * After a valid `#OTA-END$`, the module applies the saved firmware using
 * ESP-IDF OTA APIs and requests system restart after 3 seconds.
 *
 * @param frame Null-terminated complete frame (`#...$`).
 * @param tx_callback Callback to transmit responses.
 * @return true if frame is OTA protocol and was handled.
 * @return false if frame is not OTA protocol.
 */
bool ota_uart_handle_frame(const char *frame, ota_uart_tx_callback_t tx_callback);

/**
 * @brief Indicates if the last transfer was completed successfully.
 *
 * @return true when `#OTA-END$` was validated and acknowledged.
 * @return false otherwise.
 */
bool ota_uart_has_completed_transfer(void);

/**
 * @brief Returns current number of bytes persisted in storage.
 *
 * @return size_t Number of received bytes in current/last transfer.
 */
size_t ota_uart_get_received_size(void);

/**
 * @brief Returns the absolute path of persisted firmware file.
 *
 * @return const char* File path for the received firmware image.
 */
const char *ota_uart_get_firmware_path(void);

/**
 * @brief Clears current OTA UART transfer state and deletes persisted file.
 */
void ota_uart_reset_transfer(void);

#endif /* COMPONENTS_OTA_UART_INCLUDE_OTA_UART_H_ */
