/**
 * @file pluviometer_uart.c
 * @brief Pluviometer UART module implementation.
 *
 * This file contains the implementation of the Pluviometer UART module for communication over UART.
 * It uses FreeRTOS and ESP-IDF UART driver.
 */

/* Self include */
#include "pluviometer_uart.h"

/* UART include */
#include "driver/uart.h"
#include "driver/gpio.h"

/* FreeRTOS includes */
#include "FreeRTOS_defines.h"

/* Logging include */
#include "log.h"

/* Standard includes */
#include <string.h>

/* Private definitions ------------------------------------------- */
/**
 * @def PLUV_UART_TAG
 * @brief Tag used for logging in the Pluviometer UART module.
 */
#define PLUV_UART_TAG        "pluviometer"

/**
 * @def PLUV_UART_NUM
 * @brief UART number used for Pluviometer communication.
 */
#define PLUV_UART_NUM        UART_NUM_2

/**
 * @def PLUV_UART_TX_NUM
 * @brief GPIO number used for Pluviometer UART TX pin.
 */
#define PLUV_UART_TX_NUM     GPIO_NUM_11

/**
 * @def PLUV_UART_RX_NUM
 * @brief GPIO number used for Pluviometer UART RX pin.
 */
#define PLUV_UART_RX_NUM     GPIO_NUM_12

/**
 * @def PLUV_UART_BUF_SIZE
 * @brief Size of the Pluviometer UART buffer.
 */
#define PLUV_UART_BUF_SIZE   (1024)

/* Private variables --------------------------------------------- */
/**
 * @brief Callback function for Pluviometer UART events.
 */
static app_callback pluv_callback = NULL;

/**
 * @brief Queue handle for Pluviometer UART events.
 */
static QueueHandle_t pluv_uart_queue = NULL;

/* Private function prototypes ----------------------------------- */
/**
 * @brief Task to handle Pluviometer UART events.
 * @param arg User-defined argument passed to the task.
 */
static void pluv_uart_event_task(void *arg);

/* Public methods ------------------------------------------------ */
/**
 * @brief Initialize the Pluviometer UART module.
 *
 * This function initializes the Pluviometer UART module, setting up the UART communication.
 *
 * @param callback The callback function to be called when data is received.
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Fail to initialize
 */
esp_err_t pluv_uart_init(const app_callback callback)
{
    esp_err_t err = ESP_FAIL;
    BaseType_t xReturn = pdPASS;

    // Configure UART parameters
    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Install UART driver and get the queue
    err = uart_driver_install(PLUV_UART_NUM, PLUV_UART_BUF_SIZE * 6, PLUV_UART_BUF_SIZE * 2, 20, &pluv_uart_queue, 0);
    if (err == ESP_OK)
    {
        uart_param_config(PLUV_UART_NUM, &uart_config);

        // Set UART pins
        err = uart_set_pin(PLUV_UART_NUM, PLUV_UART_TX_NUM, PLUV_UART_RX_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        if (err == ESP_OK)
        {
            // Create a task to handle UART events from ISR
            xReturn = xTaskCreate(pluv_uart_event_task,
                                  PLUV_UART_TASK_NAME,
                                  PLUV_UART_STACK_SIZE,
                                  NULL,
                                  PLUV_UART_TASK_PRIORITY,
                                  NULL);

            if (callback != NULL && xReturn == pdPASS)
            {
                pluv_callback = callback;
            }
            else
            {
                err = ESP_ERR_INVALID_ARG;
                ESP_LOGE(PLUV_UART_TAG, "%s, invalid argument", __func__);
            }
        }
        else
        {
            ESP_LOGE(PLUV_UART_TAG, "%s, failed to configure UART pins", __func__);
        }
    }
    else
    {
        ESP_LOGE(PLUV_UART_TAG, "%s, failed to install UART driver", __func__);
    }

    return err;
}

/**
 * @brief Send data through the Pluviometer UART module.
 *
 * This function sends data through the Pluviometer UART module.
 *
 * @param data The buffer to be sent.
 * @param data_size The size of the buffer.
 *
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Fail to send the data
 */
esp_err_t pluv_uart_send(const char *data, size_t data_size)
{
    esp_err_t err = ESP_FAIL;

    if (uart_write_bytes(PLUV_UART_NUM, data, data_size) != -1)
    {
        LOG_COMM(PLUV_UART_TAG, "Data sent successfully");
        err = ESP_OK;
    }

    return err;
}

/* Private methods ----------------------------------------------- */
/**
 * @brief Task to handle Pluviometer UART events.
 * 
 * This task handles UART events, such as data reception, FIFO overflow, buffer full, etc.
 * 
 * @param arg User-defined argument passed to the task.
 */
static void pluv_uart_event_task(void *arg)
{
    uart_event_t event = {};
    uint8_t *dtmp = (uint8_t *)malloc(PLUV_UART_BUF_SIZE);

    while (1)
    {
        // Waiting for UART event
        if (xQueueReceive(pluv_uart_queue, (void *)&event, (TickType_t)portMAX_DELAY))
        {
            bzero(dtmp, PLUV_UART_BUF_SIZE);

            switch (event.type)
            {
            case UART_DATA:
                if (event.size > 0 && event.size < PLUV_UART_BUF_SIZE)
                {
                    uart_read_bytes(PLUV_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    LOG_COMM(PLUV_UART_TAG, "Received data: %s", (char *)dtmp);

                    if (pluv_callback != NULL)
                    {
                        pluv_callback((char *)dtmp, COMM_PLUV);
                    }
                }
                break;

            case UART_FIFO_OVF:
                ESP_LOGW(PLUV_UART_TAG, "UART FIFO overflow");
                uart_flush_input(PLUV_UART_NUM);
                xQueueReset(pluv_uart_queue);
                break;

            case UART_BUFFER_FULL:
                ESP_LOGW(PLUV_UART_TAG, "UART buffer full");
                uart_flush_input(PLUV_UART_NUM);
                xQueueReset(pluv_uart_queue);
                break;

            case UART_PARITY_ERR:
                ESP_LOGE(PLUV_UART_TAG, "%s, UART parity error", __func__);
                break;

            case UART_FRAME_ERR:
                ESP_LOGE(PLUV_UART_TAG, "%s, UART frame error", __func__);
                break;

            default:
                ESP_LOGW(PLUV_UART_TAG, "Unhandled UART event type: %d", event.type);
                break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }

    free(dtmp);
}
