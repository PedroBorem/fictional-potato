/*
 * rf_uart.c
 *
 *  Created on: 21 de jun. de 2022
 *      Author: brunolima
 */

/**
 * @file rf_uart.c
 * @date June 21, 2022
 * @brief RF module uart control
*/

/* Self include */
#include "rf_uart.h"

/* UART include */
#include "driver/uart.h"

/**\addtogroup components
 * @{
 *
 */

/**\addtogroup rf_uart
 * @{
 *
 */


/* Private definitions ------------------------------------------- */
#define RF_UART_TAG		"rf_uart"

/**
 * This example shows how to use the UART driver to handle special UART events.
 *
 * It also reads data from UART0 directly, and echoes it to console.
 *
 * - Port: UART0
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: on
 * - Pin assignment: TxD (default), RxD (default)
 */

#define RF_UART_NUM 		UART_NUM_0
#define PATTERN_CHR_NUM    	(3)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define RF_UART_BUF_SIZE 	(1024)

/* Private variables  -------------------------------------------- */
QueueHandle_t rf_uart_queue = NULL;

/* Private function prototype ------------------------------------ */
static void rf_uart_event_task(void* arg);

/* Public methods ------------------------------------------------ */
esp_err_t rf_uart_init(void)
{
	esp_err_t err = ESP_FAIL;

	// Configure parameters of an UART driver, communication pins and install the driver
	uart_config_t uart_config = {
		.baud_rate =	115200,
		.data_bits =	UART_DATA_8_BITS,
		.parity =		UART_PARITY_DISABLE,
		.stop_bits =	UART_STOP_BITS_1,
		.flow_ctrl =	UART_HW_FLOWCTRL_DISABLE,
		.source_clk =	UART_SCLK_APB,
	};

	// Install UART driver, and get the queue.
	uart_driver_install(RF_UART_NUM, RF_UART_BUF_SIZE * 2, RF_UART_BUF_SIZE * 2, 20, &rf_uart_queue, 0);
	uart_param_config(RF_UART_NUM, &uart_config);

    //Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(RF_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    //Set uart pattern detect function.
    uart_enable_pattern_det_baud_intr(RF_UART_NUM, '+', PATTERN_CHR_NUM, 9, 0, 0);

    //Reset the pattern queue length to record at most 20 pattern positions.
    uart_pattern_queue_reset(RF_UART_NUM, 20);

    //Create a task to handler UART event from ISR
    xTaskCreate(rf_uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);

	return err;
}


esp_err_t rf_uart_send_event(char* event, size_t event_size)
{
	esp_err_t err = ESP_FAIL;

	if(uart_write_bytes(RF_UART_NUM, event, event_size) != -1)
	{
		err = ESP_OK;
	}

	return err;
}

/* Private methods ----------------------------------------------- */
static void rf_uart_event_task(void* arg)
{
	uart_event_t event = {};
	size_t buffered_size = 0;
	uint8_t* dtmp = (uint8_t*) malloc(RF_UART_BUF_SIZE);

	while(1)
	{
		//Waiting for UART event.
		if(xQueueReceive(rf_uart_queue, (void*)&event, (portTickType)portMAX_DELAY))
		{
			bzero(dtmp, RF_UART_BUF_SIZE);
			LOG_COMM(RF_UART_TAG, "uart[%d] event:", RF_UART_NUM);

			switch(event.type)
			{
				case UART_DATA:
				{
					//Event of UART receving data
					/*We'd better handler data event fast, there would be much more data events than
					other types of events. If we take too much time on data event, the queue might
					be full.*/
					LOG_COMM(RF_UART_TAG, "[UART DATA]: %d", event.size);
					uart_read_bytes(RF_UART_NUM, dtmp, event.size, portMAX_DELAY);
					LOG_COMM(RF_UART_TAG, "[DATA EVT]:");
					uart_write_bytes(RF_UART_NUM, (const char*) dtmp, event.size);
					break;
				}
				case UART_FIFO_OVF:
				{
					//Event of HW FIFO overflow detected
					ESP_LOGW(RF_UART_TAG, "hw fifo overflow");
					// If fifo overflow happened, you should consider adding flow control for your application.
					// The ISR has already reset the rx FIFO,
					// As an example, we directly flush the rx buffer here in order to read more data.
					uart_flush_input(RF_UART_NUM);
					xQueueReset(rf_uart_queue);
					break;
				}
				case UART_BUFFER_FULL:
				{
					//Event of UART ring buffer full
					ESP_LOGW(RF_UART_TAG, "ring buffer full");
					// If buffer full happened, you should consider encreasing your buffer size
					// As an example, we directly flush the rx buffer here in order to read more data.
					uart_flush_input(RF_UART_NUM);
					xQueueReset(rf_uart_queue);
					break;
				}
				case UART_BREAK:
				{
					//Event of UART RX break detected
					LOG_COMM(RF_UART_TAG, "uart rx break");
					break;
				}
				case UART_PARITY_ERR:
				{
					//Event of UART parity check error
					ESP_LOGE(RF_UART_TAG, "%s, uart parity error", __func__);
					break;
				}
				case UART_FRAME_ERR:
				{
					//Event of UART frame error
					ESP_LOGE(RF_UART_TAG, "%s, uart frame error", __func__);
					break;
				}
				case UART_PATTERN_DET:
				{
					//UART_PATTERN_DET
					uart_get_buffered_data_len(RF_UART_NUM, &buffered_size);
					int pos = uart_pattern_pop_pos(RF_UART_NUM);
					LOG_COMM(RF_UART_TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
					if (pos == -1)
					{
						// There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
						// record the position. We should set a larger queue size.
						// As an example, we directly flush the rx buffer here.
						uart_flush_input(RF_UART_NUM);
					}
					else
					{
						uart_read_bytes(RF_UART_NUM, dtmp, pos, 100 / portTICK_PERIOD_MS);
						uint8_t pat[PATTERN_CHR_NUM + 1];
						memset(pat, 0, sizeof(pat));
						uart_read_bytes(RF_UART_NUM, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
						LOG_COMM(RF_UART_TAG, "read data: %s", dtmp);
						LOG_COMM(RF_UART_TAG, "read pat : %s", pat);
					}
					break;
				}
				default:
				{
					//Others
					ESP_LOGW(RF_UART_TAG, "uart event type: %d", event.type);
					break;
				}
			}
		}

		vTaskDelay(pdMS_TO_TICKS(20));
	}
}

/**@}*/ 	//rf_uart
/** @}*/	//components
