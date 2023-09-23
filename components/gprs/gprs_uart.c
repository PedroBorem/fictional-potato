/**
 * @file gprs.c
 * @date June 21, 2022
 * @brief GPRS UART control
 */

/* Self include */
#include "gprs_uart.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#include "FreeRTOS_defines.h"
#include "log.h"

#include <string.h>

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

/* Private variables  -------------------------------------------- */
/**
 * @brief Callback function for GPRS UART events
 */
static app_callback gprs_callback = NULL;

/**
 * @brief Queue handle for GPRS UART events
 */
static QueueHandle_t gprs_uart_queue = NULL;

/* Private function prototype ------------------------------------ */
/**
 * @brief Task to handle GPRS UART events
 * @param arg User-defined argument passed to the task
 */
static void gprs_uart_event_task(void* arg);

/* Public methods ------------------------------------------------ */
esp_err_t gprs_uart_init(const app_callback callback)
{
	esp_err_t err = ESP_FAIL;
	BaseType_t xReturn = pdPASS;

	// Configure parameters of an UART driver, communication pins and install the driver
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
	if(err == ESP_OK)
	{
		uart_param_config(GPRS_UART_NUM, &uart_config);

		//Set UART pins (using UART0 default pins ie no changes.)
		err = uart_set_pin(GPRS_UART_NUM, GPRS_UART_TX_NUM, GPRS_UART_RX_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
		if(err == ESP_OK)
		{
			//Create a task to handler UART event from ISR
			xReturn = xTaskCreate(gprs_uart_event_task,
								GPRS_UART_TASK_NAME,
								GPRS_UART_STACK_SIZE,
								NULL,
								GPRS_UART_TASK_PRIORITY,
								NULL);

			if(callback != NULL && xReturn == pdPASS)
			{
				gprs_callback = callback;
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


esp_err_t gprs_uart_send_event(const char* event, size_t event_size)
{
	esp_err_t err = ESP_FAIL;

	if(uart_write_bytes(GPRS_UART_NUM, event, event_size) != -1)
	{
		LOG_COMM(GPRS_UART_TAG, "OK");
		err = ESP_OK;
	}

	return err;
}

/* Private methods ----------------------------------------------- */
/**
 * @brief 	UART reception task
 * @param	arg[in] : task argument (default NULL)
 */
static void gprs_uart_event_task(void* arg)
{
	uart_event_t event = {};
	uint8_t* dtmp = (uint8_t*) malloc(GPRS_UART_BUF_SIZE);

	while(1)
	{
		//Waiting for UART event.
		if(xQueueReceive(gprs_uart_queue, (void*)&event, (TickType_t)portMAX_DELAY))
		{
			bzero(dtmp, GPRS_UART_BUF_SIZE);

			switch(event.type)
			{
				case UART_DATA:
				{
					if(event.size > 0 && event.size < 3000) // 3 KB
					{
						char* buff_in = (char*)malloc(event.size);
						int aux = 0;

						//Event of UART receving data
						uart_read_bytes(GPRS_UART_NUM, dtmp, event.size, portMAX_DELAY);
						LOG_COMM(GPRS_UART_TAG, "event size : %d", event.size);

						for(int char_position = 0; char_position < event.size; char_position++)
						{
							// 0x7F = ASCII space
							// 0x1A <= ASCII C^ values
							if(dtmp[char_position] != 0x7F && dtmp[char_position] > 0x1A)
							{
								buff_in[aux] = dtmp[char_position];
								aux ++;
							}
						}

						//LOG_COMM(GPRS_UART_TAG, "data : %s", (char*)buff_in);

						gprs_callback(buff_in, COMM_MQTT);
						free(buff_in);
					}
					break;
				}
				case UART_FIFO_OVF:
				{
					//Event of HW FIFO overflow detected
					ESP_LOGW(GPRS_UART_TAG, "hw fifo overflow");
					uart_flush_input(GPRS_UART_NUM);
					xQueueReset(gprs_uart_queue);

					break;
				}
				case UART_BUFFER_FULL:
				{
					//Event of UART ring buffer full
					ESP_LOGW(GPRS_UART_TAG, "ring buffer full");
					uart_flush_input(GPRS_UART_NUM);
					xQueueReset(gprs_uart_queue);

					break;
				}
				case UART_BREAK:
				{
					//Event of UART RX break detected
					LOG_COMM(GPRS_UART_TAG, "UART RX break");

					break;
				}
				case UART_PARITY_ERR:
				{
					//Event of UART parity check error
					ESP_LOGE(GPRS_UART_TAG, "%s, UART parity error", __func__);

					break;
				}
				case UART_FRAME_ERR:
				{
					//Event of UART frame error
					ESP_LOGE(GPRS_UART_TAG, "%s, UART frame error", __func__);

					break;
				}
				case UART_PATTERN_DET:
				{
					//Event of UART frame error
					ESP_LOGW(GPRS_UART_TAG, "%s, UART pattern", __func__);

					break;
				}
				default:
				{
					//Others
					ESP_LOGW(GPRS_UART_TAG, "UART event type: %d", event.type);

					break;
				}
			}
		}

		vTaskDelay(pdMS_TO_TICKS(20));
	}
}
