/**
 * @file gprs_uart.c
 * @brief GPRS UART control
 */

/* Self include */
#include "gprs_uart.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#include "FreeRTOS_defines.h"
#include "log.h"
#include "ota_uart.h"

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

/* Private definitions ------------------------------------------- */
/**
 * @def GPRS_UART_TAG
 * @brief Tag used for logging in the GPRS module
 */
#define GPRS_UART_TAG		"gprs"

/**
 * @def GPRS_UART_NUM
 * @brief UART number used for GPRS communication
 */
#define GPRS_UART_NUM 		UART_NUM_1

/**
 * @def GPRS_UART_TX_NUM
 * @brief GPIO number used for GPRS UART TX pin
 */
#define GPRS_UART_TX_NUM 	GPIO_NUM_17

/**
 * @def GPRS_UART_RX_NUM
 * @brief GPIO number used for GPRS UART RX pin
 */
#define GPRS_UART_RX_NUM	GPIO_NUM_18

/**
 * @def GPRS_UART_BUF_SIZE
 * @brief Size of the GPRS UART buffer
 */
#define GPRS_UART_BUF_SIZE 	(1024)

/**
 * @def GPRS_UART_FRAME_SIZE_MAX
 * @brief Maximum complete frame size handled by stream parser.
 */
#define GPRS_UART_FRAME_SIZE_MAX (2048)

/**
 * @def GPRS_UART_ASCII_PREVIEW_BYTES
 * @brief Number of bytes captured in frame previews for diagnostics.
 */
#define GPRS_UART_ASCII_PREVIEW_BYTES (24U)

/**
 * @def GPRS_UART_CONTROL_DROP_LOG_LIMIT
 * @brief Maximum number of immediate control-byte drop warnings.
 */
#define GPRS_UART_CONTROL_DROP_LOG_LIMIT (20U)

/**
 * @def GPRS_POST_UPDATE_NOTICE_FRAME
 * @brief One-shot frame sent after reboot from successful OTA apply.
 */
#define GPRS_POST_UPDATE_NOTICE_FRAME "#OTA-BOARD-UPDATE-REBOOTED$"

/**
 * @def GPRS_POST_UPDATE_NOTICE_RETRY_COUNT
 * @brief Number of post-update notice retries sent on boot.
 */
#define GPRS_POST_UPDATE_NOTICE_RETRY_COUNT (3U)

/**
 * @def GPRS_POST_UPDATE_NOTICE_RETRY_DELAY_MS
 * @brief Delay between post-update notice retries.
 */
#define GPRS_POST_UPDATE_NOTICE_RETRY_DELAY_MS (300U)

/* Private variables  -------------------------------------------- */
/**
 * @brief Callback function for GPRS UART events
 */
static app_callback gprs_callback = NULL;

/**
 * @brief Queue handle for GPRS UART events
 */
static QueueHandle_t gprs_uart_queue = NULL;

/**
 * @brief Temporary buffer for assembling complete '#...$' frames.
 */
static char gprs_frame_buffer[GPRS_UART_FRAME_SIZE_MAX] = {0};

/**
 * @brief Current number of bytes in @ref gprs_frame_buffer.
 */
static size_t gprs_frame_length = 0;

/**
 * @brief Indicates if parser is currently inside a frame.
 */
static bool gprs_receiving_frame = false;

/**
 * @brief OTA DATA frame counter for sparse debug logging.
 */
static uint32_t gprs_ota_data_frame_counter = 0U;

/**
 * @brief Counter for dropped control bytes while frame assembly is active.
 */
static uint32_t gprs_ota_control_drop_counter = 0U;

/**
 * @brief Counter for parser restarts triggered by a new frame prefix.
 */
static uint32_t gprs_ota_frame_restart_counter = 0U;

/* Private function prototype ------------------------------------ */
/**
 * @brief Task to handle GPRS UART events
 * @param arg User-defined argument passed to the task
 */
static void gprs_uart_event_task(void* arg);

/**
 * @brief Consumes one received character and extracts complete frames.
 *
 * @param received_char Character from UART stream.
 */
static void gprs_uart_consume_char(char received_char);

/**
 * @brief Dispatches a complete frame to OTA UART handler or application callback.
 *
 * @param frame Null-terminated complete frame (`#...$`).
 */
static void gprs_uart_dispatch_frame(const char *frame);

/**
 * @brief Copies head and tail previews from frame buffer.
 *
 * @param input Input string.
 * @param input_len Input length.
 * @param preview_len Preview size for head/tail.
 * @param head_out Output head preview.
 * @param head_out_size Head buffer size.
 * @param tail_out Output tail preview.
 * @param tail_out_size Tail buffer size.
 */
static void gprs_uart_copy_ascii_preview(const char *input, size_t input_len, size_t preview_len,
                                         char *head_out, size_t head_out_size,
                                         char *tail_out, size_t tail_out_size);

/* Public methods ------------------------------------------------ */
/**
 * @brief Initialize the GPRS UART module.
 *
 * This function initializes the GPRS UART module, setting up the UART communication for GPRS.
 *
 * @param callback The callback function to be called when data is received.
 *
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Fail to initialize
 *     - ESP_ERR_INVALID_ARG: Invalid callback function
 */
esp_err_t gprs_uart_init(const app_callback callback)
{
    esp_err_t err = ESP_FAIL;
    BaseType_t xReturn = pdPASS;

    // Configure parameters of a UART driver, communication pins and install the driver
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Install UART driver, and get the queue.
    err = uart_driver_install(GPRS_UART_NUM, GPRS_UART_BUF_SIZE * 6, GPRS_UART_BUF_SIZE * 2, 20, &gprs_uart_queue, 0);
    if (err == ESP_OK)
    {
        uart_param_config(GPRS_UART_NUM, &uart_config);

        // Set UART pins (using UART0 default pins, i.e., no changes.)
        err = uart_set_pin(GPRS_UART_NUM, GPRS_UART_TX_NUM, GPRS_UART_RX_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        if (err == ESP_OK)
        {
            // Create a task to handle UART event from ISR
            xReturn = xTaskCreate(gprs_uart_event_task,
                GPRS_UART_TASK_NAME,
                GPRS_UART_STACK_SIZE,
                NULL,
                GPRS_UART_TASK_PRIORITY,
                NULL);

            if (callback != NULL && xReturn == pdPASS)
            {
                gprs_callback = callback;

                esp_err_t ota_err = ota_uart_init();
                if (ota_err != ESP_OK)
                {
                    ESP_LOGW(GPRS_UART_TAG, "%s, OTA UART storage not available (%s)",
                             __func__, esp_err_to_name(ota_err));
                }
                else if (ota_uart_consume_post_update_notice())
                {
                    ESP_LOGI(GPRS_UART_TAG, "Post-update reboot notice detected. Sending frame to modem.");

                    for (uint8_t attempt = 1U; attempt <= GPRS_POST_UPDATE_NOTICE_RETRY_COUNT; attempt++)
                    {
                        if (gprs_uart_send_event(GPRS_POST_UPDATE_NOTICE_FRAME,
                                                 strlen(GPRS_POST_UPDATE_NOTICE_FRAME)) == ESP_OK)
                        {
                            ESP_LOGI(GPRS_UART_TAG,
                                     "Post-update notice sent (%u/%u)",
                                     (unsigned int)attempt,
                                     (unsigned int)GPRS_POST_UPDATE_NOTICE_RETRY_COUNT);
                        }
                        else
                        {
                            ESP_LOGW(GPRS_UART_TAG,
                                     "Failed to send post-update notice (%u/%u)",
                                     (unsigned int)attempt,
                                     (unsigned int)GPRS_POST_UPDATE_NOTICE_RETRY_COUNT);
                        }

                        if (attempt < GPRS_POST_UPDATE_NOTICE_RETRY_COUNT)
                        {
                            vTaskDelay(pdMS_TO_TICKS(GPRS_POST_UPDATE_NOTICE_RETRY_DELAY_MS));
                        }
                    }
                }
            }
            else
            {
                err = ESP_ERR_INVALID_ARG;
                ESP_LOGE(GPRS_UART_TAG, "%s, invalid argument", __func__);
            }
        }
        else
        {
            ESP_LOGE(GPRS_UART_TAG, "%s, failed to configure UART pin", __func__);
        }
    }
    else
    {
        ESP_LOGE(GPRS_UART_TAG, "%s, failed to install UART driver", __func__);
    }

    return err;
}

/**
 * @brief Send an event through the GPRS UART module.
 *
 * This function sends an event buffer through the GPRS UART module.
 *
 * @param event The event buffer to be sent.
 * @param event_size The size of the event buffer.
 *
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Fail to send the event
 */
esp_err_t gprs_uart_send_event(const char* event, size_t event_size)
{
    esp_err_t err = ESP_FAIL;

    if (uart_write_bytes(GPRS_UART_NUM, event, event_size) != -1)
    {
        LOG_COMM(GPRS_UART_TAG, "OK");
        err = ESP_OK;
    }

    return err;
}

/* Private methods ----------------------------------------------- */
/**
 * @brief UART reception task
 * @param arg[in] : task argument (default NULL)
 */
static void gprs_uart_event_task(void* arg)
{
    uart_event_t event = {};
    uint8_t* dtmp = (uint8_t*)malloc(GPRS_UART_BUF_SIZE);

    if (dtmp == NULL)
    {
        ESP_LOGE(GPRS_UART_TAG, "%s, failed to allocate RX buffer", __func__);
        vTaskDelete(NULL);
        return;
    }

    while (1)
    {
        // Waiting for UART event.
        if (xQueueReceive(gprs_uart_queue, (void*)&event, (TickType_t)portMAX_DELAY))
        {
            bzero(dtmp, GPRS_UART_BUF_SIZE);

            switch (event.type)
            {
            case UART_DATA:
            {
                if (event.size > 0)
                {
                    LOG_COMM(GPRS_UART_TAG, "event size : %d", event.size);

                    int remaining = event.size;
                    while (remaining > 0)
                    {
                        int read_request = remaining;
                        if (read_request > GPRS_UART_BUF_SIZE)
                        {
                            read_request = GPRS_UART_BUF_SIZE;
                        }

                        int read_len = uart_read_bytes(GPRS_UART_NUM, dtmp, read_request, portMAX_DELAY);
                        if (read_len <= 0)
                        {
                            break;
                        }

                        for (int char_position = 0; char_position < read_len; char_position++)
                        {
                            const uint8_t current_byte = dtmp[char_position];

                            // 0x7F = ASCII DEL
                            // 0x1A <= ASCII C^ values
                            if (current_byte == 0x7F || current_byte <= 0x1A)
                            {
                                if (gprs_receiving_frame)
                                {
                                    gprs_ota_control_drop_counter++;
                                    if (gprs_ota_control_drop_counter <= GPRS_UART_CONTROL_DROP_LOG_LIMIT ||
                                        (gprs_ota_control_drop_counter % 100U) == 0U)
                                    {
                                        ESP_LOGW(GPRS_UART_TAG,
                                                 "Dropped control byte 0x%02X while assembling frame "
                                                 "(frame_len=%u, drop_count=%" PRIu32 ")",
                                                 (unsigned int)current_byte,
                                                 (unsigned int)gprs_frame_length,
                                                 gprs_ota_control_drop_counter);
                                    }
                                }

                                continue;
                            }

                            gprs_uart_consume_char((char)current_byte);
                        }

                        remaining -= read_len;
                    }
                }
                break;
            }
            case UART_FIFO_OVF:
            {
                // Event of HW FIFO overflow detected
                ESP_LOGW(GPRS_UART_TAG, "hw fifo overflow");
                uart_flush_input(GPRS_UART_NUM);
                xQueueReset(gprs_uart_queue);

                break;
            }
            case UART_BUFFER_FULL:
            {
                // Event of UART ring buffer full
                ESP_LOGW(GPRS_UART_TAG, "ring buffer full");
                uart_flush_input(GPRS_UART_NUM);
                xQueueReset(gprs_uart_queue);

                break;
            }
            case UART_BREAK:
            {
                // Event of UART RX break detected
                LOG_COMM(GPRS_UART_TAG, "UART RX break");

                break;
            }
            case UART_PARITY_ERR:
            {
                // Event of UART parity check error
                ESP_LOGE(GPRS_UART_TAG, "%s, UART parity error", __func__);

                break;
            }
            case UART_FRAME_ERR:
            {
                // Event of UART frame error
                ESP_LOGE(GPRS_UART_TAG, "%s, UART frame error", __func__);

                break;
            }
            case UART_PATTERN_DET:
            {
                // Event of UART frame error
                ESP_LOGW(GPRS_UART_TAG, "%s, UART pattern", __func__);

                break;
            }
            default:
            {
                // Others
                ESP_LOGW(GPRS_UART_TAG, "UART event type: %d", event.type);

                break;
            }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
    free(dtmp);
}

/* Private methods ----------------------------------------------- */
/**
 * @brief Consumes one UART character and extracts complete '#...$' frames.
 *
 * @param received_char Character read from UART.
 */
static void gprs_uart_consume_char(char received_char)
{
    if (received_char == '#')
    {
        if (gprs_receiving_frame && gprs_frame_length > 0U)
        {
            char abandoned_head[GPRS_UART_ASCII_PREVIEW_BYTES + 1U] = {0};
            char abandoned_tail[GPRS_UART_ASCII_PREVIEW_BYTES + 1U] = {0};

            gprs_ota_frame_restart_counter++;
            gprs_uart_copy_ascii_preview(gprs_frame_buffer, gprs_frame_length, GPRS_UART_ASCII_PREVIEW_BYTES,
                                         abandoned_head, sizeof(abandoned_head),
                                         abandoned_tail, sizeof(abandoned_tail));

            ESP_LOGW(GPRS_UART_TAG,
                     "Parser restarted on new '#' before '$' "
                     "(prev_len=%u, restart_count=%" PRIu32 ", head='%s', tail='%s')",
                     (unsigned int)gprs_frame_length,
                     gprs_ota_frame_restart_counter,
                     abandoned_head,
                     abandoned_tail);
        }

        gprs_receiving_frame = true;
        gprs_frame_length = 0;
        gprs_frame_buffer[gprs_frame_length++] = received_char;
        return;
    }

    if (!gprs_receiving_frame)
    {
        return;
    }

    if (gprs_frame_length >= (GPRS_UART_FRAME_SIZE_MAX - 1))
    {
        char oversized_head[GPRS_UART_ASCII_PREVIEW_BYTES + 1U] = {0};
        char oversized_tail[GPRS_UART_ASCII_PREVIEW_BYTES + 1U] = {0};

        gprs_uart_copy_ascii_preview(gprs_frame_buffer, gprs_frame_length, GPRS_UART_ASCII_PREVIEW_BYTES,
                                     oversized_head, sizeof(oversized_head),
                                     oversized_tail, sizeof(oversized_tail));

        gprs_receiving_frame = false;
        gprs_frame_length = 0;
        ESP_LOGW(GPRS_UART_TAG, "Discarded oversized frame (head='%s', tail='%s')",
                 oversized_head, oversized_tail);
        return;
    }

    gprs_frame_buffer[gprs_frame_length++] = received_char;

    if (received_char == '$')
    {
        gprs_frame_buffer[gprs_frame_length] = '\0';
        gprs_uart_dispatch_frame(gprs_frame_buffer);
        gprs_receiving_frame = false;
        gprs_frame_length = 0;
    }
}

/**
 * @brief Dispatches complete frame.
 *
 * OTA frames are handled locally by OTA UART module and acknowledged over UART.
 * Other frames follow the existing callback path to the system manager.
 *
 * @param frame Complete frame (`#...$`).
 */
static void gprs_uart_dispatch_frame(const char *frame)
{
    if (frame == NULL)
    {
        return;
    }

    if (strncmp(frame, "#OTA-DATA-", 10) == 0)
    {
        gprs_ota_data_frame_counter++;
        if (gprs_ota_data_frame_counter <= 3U || (gprs_ota_data_frame_counter % 250U) == 0U)
        {
            size_t frame_len = strlen(frame);
            char frame_head[GPRS_UART_ASCII_PREVIEW_BYTES + 1U] = {0};
            char frame_tail[GPRS_UART_ASCII_PREVIEW_BYTES + 1U] = {0};
            size_t hyphen_count = 0U;

            for (size_t i = 0; i < frame_len; i++)
            {
                if (frame[i] == '-')
                {
                    hyphen_count++;
                }
            }

            gprs_uart_copy_ascii_preview(frame, frame_len, GPRS_UART_ASCII_PREVIEW_BYTES,
                                         frame_head, sizeof(frame_head),
                                         frame_tail, sizeof(frame_tail));

            ESP_LOGI(GPRS_UART_TAG,
                     "OTA DATA frame rx_count=%" PRIu32 " len=%u hyphens=%u head='%s' tail='%s'",
                     gprs_ota_data_frame_counter,
                     (unsigned int)frame_len,
                     (unsigned int)hyphen_count,
                     frame_head,
                     frame_tail);
        }
    }

    if (ota_uart_handle_frame(frame, gprs_uart_send_event))
    {
        return;
    }

    if (gprs_callback != NULL)
    {
        char *frame_copy = strdup(frame);
        if (frame_copy != NULL)
        {
            gprs_callback(frame_copy, COMM_MQTT);
            free(frame_copy);
        }
        else
        {
            ESP_LOGE(GPRS_UART_TAG, "Failed to duplicate frame for callback");
        }
    }
}

/* Private methods ----------------------------------------------- */
/**
 * @brief Copies head and tail previews from frame buffer.
 *
 * @param input Input string.
 * @param input_len Input length.
 * @param preview_len Preview size for head/tail.
 * @param head_out Output head preview.
 * @param head_out_size Head buffer size.
 * @param tail_out Output tail preview.
 * @param tail_out_size Tail buffer size.
 */
static void gprs_uart_copy_ascii_preview(const char *input, size_t input_len, size_t preview_len,
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
