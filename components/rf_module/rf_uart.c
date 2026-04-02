/**
 * @file rf_uart.c
 * @brief RF UART module implementation.
 *
 * This file contains the implementation of the RF UART module for communication over UART.
 * It uses FreeRTOS and ESP-IDF UART driver.
 */

/* Self include */
#include "rf_uart.h"

/* UART include */
#include "driver/uart.h"
#include "driver/gpio.h"

/* FreeRTOS includes */
#include "FreeRTOS_defines.h"

/* C base */
#include <strings.h>

/* include components */
#include "log.h"

/**
 * @def RF_UART_TAG
 * @brief The log tag for RF UART module.
 */
#define RF_UART_TAG     "rf_uart"

/**
 * @def RF_UART_NUM
 * @brief The UART number to use.
 */
#define RF_UART_NUM     UART_NUM_2

/**
 * @def RF_UART_TX_NUM
 * @brief The GPIO number for UART TX.
 */
#define RF_UART_TX_NUM  GPIO_NUM_2

/**
 * @def RF_UART_RX_NUM
 * @brief The GPIO number for UART RX.
 */
#define RF_UART_RX_NUM  GPIO_NUM_1

/**
 * @def RF_UART_BUF_SIZE
 * @brief The buffer size for UART communication.
 */
#define RF_UART_BUF_SIZE    (1024)

/* Private variables  -------------------------------------------- */
/**
 * @brief Callback function for RF UART events.
 */
static app_callback rf_callback = NULL;

/**
 * @brief Queue handle for RF UART events.
 */
static QueueHandle_t rf_uart_queue = NULL;

/* Private function prototype ------------------------------------ */
/**
 * @brief RF UART event task.
 * @param arg Task argument (default NULL).
 */
static void rf_uart_event_task(void* arg);

/* Public methods ------------------------------------------------ */
/**
 * @brief Initialize the RF UART module.
 *
 * This function configures the UART parameters, installs the UART driver, and creates a task to handle UART events.
 * @param callback Callback function for RF UART events.
 * @return esp_err_t Returns ESP_OK on success, ESP_FAIL on failure.
 */
esp_err_t rf_uart_init(const app_callback callback)
{
    esp_err_t err = ESP_FAIL;
    BaseType_t xReturn = pdPASS;

    // Configure parameters of a UART driver, communication pins, and install the driver
    const uart_config_t uart_config = {
        .baud_rate =    9600,
        .data_bits =    UART_DATA_8_BITS,
        .parity =       UART_PARITY_DISABLE,
        .stop_bits =    UART_STOP_BITS_1,
        .flow_ctrl =    UART_HW_FLOWCTRL_DISABLE,
        .source_clk =   UART_SCLK_DEFAULT,
    };

    // Install UART driver and get the queue.
    err = uart_driver_install(RF_UART_NUM, RF_UART_BUF_SIZE * 6, RF_UART_BUF_SIZE * 2, 20, &rf_uart_queue, 0);
    if (err == ESP_OK) {
        uart_param_config(RF_UART_NUM, &uart_config);

        // Set UART pins (using UART0 default pins, i.e., no changes).
        err = uart_set_pin(RF_UART_NUM, RF_UART_TX_NUM, RF_UART_RX_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        if (err == ESP_OK) {
            // Create a task to handle UART events from ISR
            xReturn = xTaskCreate(rf_uart_event_task,
                                  RF_UART_TASK_NAME,
                                  RF_UART_STACK_SIZE,
                                  NULL,
                                  RF_UART_TASK_PRIORITY,
                                  NULL);

            if (callback != NULL && xReturn == pdPASS) {
                rf_callback = callback;
            } else {
                err = ESP_ERR_INVALID_ARG;
                ESP_LOGE(RF_UART_TAG, "%s, invalid argument", __func__);
            }
        } else {
            ESP_LOGE(RF_UART_TAG, "%s, failed to configure UART pin", __func__);
        }
    } else {
        ESP_LOGE(RF_UART_TAG, "%s, failed to install UART driver", __func__);
    }

    return err;
}

/**
 * @brief Send an event over RF UART.
 *
 * @param event Pointer to the event data.
 * @param event_size Size of the event data.
 * @return esp_err_t Returns ESP_OK on success, ESP_FAIL on failure.
 */
esp_err_t rf_uart_send_event(const char* event, size_t event_size)
{
    esp_err_t err = ESP_FAIL;

    if (uart_write_bytes(RF_UART_NUM, event, event_size) != -1) {
        LOG_COMM(RF_UART_TAG, "OK");
        err = ESP_OK;
    }

    return err;
}

/* Private methods ----------------------------------------------- */
/**
 * @brief 	UART reception task.
 *
 * This task handles UART events, such as data reception, FIFO overflow, buffer full, etc.
 * @param arg Task argument (default NULL).
 */
static void rf_uart_event_task(void* arg)
{
    uart_event_t event = {};
    uint8_t* dtmp = (uint8_t*)malloc(RF_UART_BUF_SIZE);

    while (1) {
        // Waiting for UART event.
        if (xQueueReceive(rf_uart_queue, (void*)&event, (TickType_t)portMAX_DELAY)) {
            bzero(dtmp, RF_UART_BUF_SIZE);

            switch (event.type) {
                case UART_DATA: {
                    if (event.size > 0 && event.size < 3000) { // 3 KB
                        char* buff_in = (char*)malloc(event.size + 1);
                        int aux = 0;

                        if (buff_in == NULL) {
                            ESP_LOGE(RF_UART_TAG, "%s, failed to allocate UART input buffer", __func__);
                            uart_flush_input(RF_UART_NUM);
                            xQueueReset(rf_uart_queue);
                            break;
                        }

                        // Event of UART receiving data.
                        uart_read_bytes(RF_UART_NUM, dtmp, event.size, portMAX_DELAY);
                        LOG_COMM(RF_UART_TAG, "event size : %d", event.size);

                        for (int char_position = 0; char_position < event.size; char_position++) {
                            // 0x7F = ASCII space
                            // 0x1A <= ASCII C^ values
                            if (dtmp[char_position] != 0x7F && dtmp[char_position] > 0x1A) {
                                buff_in[aux] = dtmp[char_position];
                                aux++;
                            }
                        }

                        buff_in[aux] = '\0';

                        // LOG_COMM(RF_UART_TAG, "data : %s", (char*)buff_in);

                        rf_callback(buff_in, COMM_RF);
                        free(buff_in);
                    }
                    break;
                }
                case UART_FIFO_OVF: {
                    // Event of HW FIFO overflow detected.
                    ESP_LOGW(RF_UART_TAG, "hw fifo overflow");
                    uart_flush_input(RF_UART_NUM);
                    xQueueReset(rf_uart_queue);

                    break;
                }
                case UART_BUFFER_FULL: {
                    // Event of UART ring buffer full.
                    ESP_LOGW(RF_UART_TAG, "ring buffer full");
                    uart_flush_input(RF_UART_NUM);
                    xQueueReset(rf_uart_queue);

                    break;
                }
                case UART_BREAK: {
                    // Event of UART RX break detected.
                    ESP_LOGD(RF_UART_TAG, "UART RX break");

                    break;
                }
                case UART_PARITY_ERR: {
                    // Event of UART parity check error.
                    ESP_LOGE(RF_UART_TAG, "%s, UART parity error", __func__);

                    break;
                }
                case UART_FRAME_ERR: {
                    // Event of UART frame error.
                    ESP_LOGE(RF_UART_TAG, "%s, UART frame error", __func__);

                    break;
                }
                case UART_PATTERN_DET: {
                    // Event of UART frame error.
                    ESP_LOGW(RF_UART_TAG, "%s, UART pattern", __func__);

                    break;
                }
                default: {
                    // Others
                    ESP_LOGW(RF_UART_TAG, "UART event type: %d", event.type);

                    break;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
