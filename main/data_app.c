/*
 * data_app.c
 *
 *  Created on: 15 de jun. de 2022
 *      Author: brunolima
 */

/* Self include */
#include "data_app.h"

/* NVS include */
#include "nvs_data.h"
#include "nvs_config.h"

/**
 * @file data_app.c
 * @brief data app
*/

#define DATA_APP_TAG			"data_app"
#define DATA_APP_SIZE_QUEUE		10

typedef enum
{
	DATA_APP_SAVE_CONFIG = 0,
	DATA_APP_LOAD_CONFIG = 1
}data_app_request_type;


typedef struct
{
	data_app_request_type request_type;
	union
	{
		pivot_config config_in;
		pivot_config* config_out;
		//TODO: added scheduling and history
	};
	union
	{
		size_t config_size_in;
		size_t * config_size_out;
	};

}data_app_request;

data_app_callback data_app_call = NULL;

/* freertos variables */
TaskHandle_t xTask_data_app = NULL;
QueueHandle_t xQueue_data_app = NULL;

/* Private function prototype ------------------------------------ */
void data_app_task(void* arg);

/* Public methods ------------------------------------------------ */
bool data_app_init(data_app_callback app_callback)
{
	bool ret = false;
	BaseType_t xReturn = pdPASS;
	esp_err_t err = ESP_FAIL;

	err = nvs_data_init();
	if(err == ESP_OK)
	{
		err = nvs_config_init();
		if(err == ESP_OK)
		{
			xReturn = xTaskCreate(&data_app_task,
									DATA_APP_TASK_NAME,
									DATA_APP_STACK_SIZE,
									NULL,
									DATA_APP_TASK_PRIORITY,
									&xTask_data_app);

			if(xReturn == pdPASS || xTask_data_app != NULL)
			{
				xQueue_data_app = xQueueCreate(DATA_APP_SIZE_QUEUE, sizeof(data_app_request));
				if(xQueue_data_app != NULL)
				{
					ret = true;
				}
				else
				{
					ESP_LOGE( DATA_APP_TAG, "%s, failed to create task: %s", __func__, DATA_APP_TASK_NAME);
				}
			}
			else
			{
				ESP_LOGE( DATA_APP_TAG, "%s, failed to create queue", __func__);
			}
		}
	}

	if(data_app_call != NULL && ret == true)
	{
		data_app_call = app_callback;
	}
	else
	{
		ESP_LOGE( DATA_APP_TAG, "%s, failed to start data application", __func__);
	}

	return ret;
}


bool data_app_save_config(pivot_config config_in, size_t config_length)
{
	bool ret = false;
	data_app_request data_queue_request = {};

	data_queue_request.request_type = DATA_APP_SAVE_CONFIG;
	data_queue_request.config_size_in = config_length;

	if(config_length > 0)
	{
		memcpy(&data_queue_request.config_in , &config_in , config_length);
		if(xQueueSend(xQueue_data_app, &data_queue_request ,(TickType_t)20) == pdPASS)
		{
			ret = true;
		}
		else
		{
			ESP_LOGE( DATA_APP_TAG, "%s, failed to publish to queue", __func__);
		}
	}
	else
	{
		ESP_LOGE( DATA_APP_TAG, "%s, invalid configuration length", __func__);
	}

	return ret;
}

bool data_app_load_config(pivot_config* config_out, size_t* config_length)
{
	bool ret = false;
	data_app_request data_queue_request = {};

	data_queue_request.request_type = DATA_APP_LOAD_CONFIG;
	data_queue_request.config_size_out = config_length;
	data_queue_request.config_out = config_out;

	if(xQueueSend(xQueue_data_app, &data_queue_request ,(TickType_t)20) == pdPASS)
	{
		ret = true;
	}
	else
	{
		ESP_LOGE( DATA_APP_TAG, "%s, failed to publish to queue", __func__);
	}

	return ret;
}


void data_app_task(void* arg)
{
	data_app_request data_queue_receive = {};
	esp_err_t err = ESP_FAIL;

	while(1)
	{
		if(xQueueReceive(xQueue_data_app,&data_queue_receive, (TickType_t)100) == pdTRUE)
		{
			switch (data_queue_receive.request_type)
			{
				case DATA_APP_SAVE_CONFIG:
				{
					err = nvs_config_set(data_queue_receive.config_in, data_queue_receive.config_size_in);
					if(err == ESP_OK)
					{
						//TODO: nvs_config print
					}
					break;
				}
				case DATA_APP_LOAD_CONFIG:
				{
					err = nvs_config_get(data_queue_receive.config_out, data_queue_receive.config_size_out);
					if(err != ESP_OK)
					{
						ESP_LOGE( DATA_APP_TAG, "%s, failed to get settings", __func__);
					}
					// TODO : call pointer function.
					if(data_app_call != NULL)
					{
						data_app_call(CALL_LOAD_CONFIG);
					}
					break;
				}
				default:
				{
					break;
				}
			}
		}

		vTaskDelay(pdMS_TO_TICKS(20));
	}
}

