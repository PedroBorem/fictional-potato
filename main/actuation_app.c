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
#include "gpio_actuator.h"

/**\addtogroup main
 * @{
 *
 */

/**\addtogroup actuation_app
 * @{
 *
 */

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
	esp_err_t err = ESP_OK;

	err = gpio_actuator_init();
	if(callback != NULL && err == ESP_OK)
	{
		actuation_app_call = callback;

		BaseType_t xReturn = xTaskCreate(&actuation_app_task,
								ACTUATION_APP_TASK_NAME,
								ACTUATION_APP_STACK_SIZE, // modificar aqui os nomes
								NULL,
								ACTUATION_APP_TASK_PRIORITY,
								&xTask_actuation_app);

		if(xReturn == pdPASS || xTask_actuation_app != NULL)
		{
			ret = true;
		}
		else
		{
			ESP_LOGE(ACTUATION_APP_TAG, "%s, failed to create task: %s", __func__, ACTUATION_APP_TASK_NAME);
		}
	}
	else
	{
		ESP_LOGE(ACTUATION_APP_TAG, "%s, invalid argument", __func__);
	}

	return ret;
}

void actuation_app_set_config(const pivot_config config_in)
{
	memcpy(&actuation_config, &config_in, sizeof(actuation_config));
	gpio_actuator_set(config_in);
	xTaskNotifyGive(xTask_actuation_app);
}

void actuation_app_get_config(pivot_config* config_out, size_t config_size)
{
	pivot_config current_config = {};

	if(config_size > 0 && config_out != NULL )
	{
		current_config = gpio_actuator_get();
		printf("power_state %d\n", current_config.power_state);
		printf("rotation %d\n", current_config.rotation);
		printf("watering_state %d\n", current_config.watering_state);
		printf("percentimeter %d\n", current_config.percentimeter);

		memcpy(config_out, &current_config, config_size);
	}
}

/* Private methods ----------------------------------------------- */
/**
 * @brief 	Task responsible for monitoring possible changes in equipment status
 * @param	arg - [in]: task argument (default NULL)
 */
void actuation_app_task(void* arg)
{
	pivot_config current_config = {};
	TickType_t last_tick = xTaskGetTickCount();

	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	while(1)
	{
		current_config = gpio_actuator_get();

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
			if(pdTICKS_TO_MS(xTaskGetTickCount() - last_tick) > 61000) //double check (1 minute and 1 second)
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

/**@}*/ 	//actuation_app
/** @}*/	//main
