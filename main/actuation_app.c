/*
 * actuation_app.c
 *
 *  Created on: 22 de jul. de 2022
 *      Author: brunolima
 */

/**
 * @file actuation_app.c
 * @date June 23, 2022
 * @brief actuation control class
*/

/* Self include */
#include "actuation_app.h"

/* Components include */
#include "rf_module.h"


/* Private definitions ------------------------------------------- */
#define ACTUATION_APP_TAG			"actuation_app"

/* Private variables  -------------------------------------------- */
static TaskHandle_t xTask_actuation_app = NULL;
static app_callback actuation_app_call = NULL;


static pivot_config actuation_config = {};

/* Private methods  ---------------------------------------------- */
void actuation_app_task(void* arg);

/* Public methods ------------------------------------------------ */
bool actuation_app_init(const app_callback callback)
{
	bool ret = false;

	if(callback != NULL)
	{
		actuation_app_call = callback;

		BaseType_t xReturn = xTaskCreate(&actuation_app_task,
								DATA_APP_TASK_NAME,
								DATA_APP_STACK_SIZE, // modificar aqui os nomes
								NULL,
								DATA_APP_TASK_PRIORITY,
								&xTask_actuation_app);

		if(xReturn == pdPASS || xTask_actuation_app != NULL)
		{
			ret = true;
		}
		else
		{
			ESP_LOGE(ACTUATION_APP_TAG, "%s, failed to create task: %s", __func__, DATA_APP_TASK_NAME);
		}
	}
	else
	{

	}



	return ret;
}

void actuation_app_set_config(const pivot_config config_in)
{
	memcpy(&actuation_config, &config_in, sizeof(actuation_config));

	//TODO : set configuration

	xTaskNotifyGive(xTask_actuation_app);
}

void actuation_app_get_config(pivot_config* config_out, size_t config_size)
{
	if(config_size > 0 && config_out != NULL )
	{
		// TODO: LER AS CONFIGURAÇÕES ATUAIS
		memcpy(&config_out, &actuation_config, config_size);
	}
}

/* Private methods ----------------------------------------------- */
void actuation_app_task(void* arg)
{
	pivot_config current_config = {};
	TickType_t last_tick = xTaskGetTickCount();

	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	while(1)
	{
		// TODO: LER AS CONFIGURAÇÕES ATUAIS

		if(current_config.power_state != actuation_config.power_state)
		{
			if(pdTICKS_TO_MS(xTaskGetTickCount() - last_tick) > 1000) // double check (1 second)
			{
				last_tick = xTaskGetTickCount();
				actuation_app_call(CALL_NEW_CONFIG, &current_config);
				ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
			}
		}
		else if(current_config.watering_state != actuation_config.watering_state)
		{
			if(pdTICKS_TO_MS(xTaskGetTickCount() - last_tick) > 1000)  // double check (1 second)
			{
				last_tick = xTaskGetTickCount();
				actuation_app_call(CALL_NEW_CONFIG, &current_config);
				ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
			}
		}
		else if(current_config.percentimeter > (actuation_config.percentimeter + 10) // 10% change in percent
			|| (current_config.percentimeter + 10) < actuation_config.percentimeter )
		{
			if(pdTICKS_TO_MS(xTaskGetTickCount() - last_tick) > 61000) //double check (1 minute and 10 seconds)
			{
				last_tick = xTaskGetTickCount();
				actuation_app_call(CALL_NEW_CONFIG, &current_config);
				ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
			}
		}
		else
		{
			last_tick = xTaskGetTickCount();
		}

		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

