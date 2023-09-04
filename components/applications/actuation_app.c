/**
 * @file actuation_app.c
 * @date June 23, 2022
 * @brief actuation control class
*/

/* Self include */
#include "actuation_app.h"
#include "gpio_actuator.h"

#include "FreeRTOS_defines.h"
#include "idp_parser.h"
#include "log.h"

#include <string.h>

/* Private definitions ------------------------------------------- */
#define ACTUATION_APP_TAG			"actuation_app"

// manual config timeout
#define ACTUATION_APP_POWER_TIME			10000	// 10 sec
#define ACTUATION_APP_WATERING_TIME			30000	// 30 sec
#define ACTUATION_APP_ROTATION_TIME			3000	// 3 sec
#define ACTUATION_APP_PERCENTIMETER_TIME	61000	// 1 min and 10 sec

/* Private variables  -------------------------------------------- */
static TaskHandle_t xTask_actuation_app = NULL;
static app_callback actuation_app_call = NULL;
static pivot_actions actuation_config = {};
static bool actuation_first_interaction = false;

/* Private methods  ---------------------------------------------- */
void actuation_app_task(void* arg);
void actuation_app_manual_call(bool on_off, pivot_actions current_action);

/* Public methods ------------------------------------------------ */
esp_err_t actuation_app_init(const app_callback callback)
{
	esp_err_t err = ESP_OK;

	err = gpio_actuator_init();
	if(callback != NULL && err == ESP_OK)
	{
		actuation_app_call = callback;

		BaseType_t xReturn = xTaskCreate(&actuation_app_task,
								ACTUATION_APP_TASK_NAME,
								ACTUATION_APP_STACK_SIZE,
								NULL,
								ACTUATION_APP_TASK_PRIORITY,
								&xTask_actuation_app);

		if(xReturn != pdPASS || xTask_actuation_app == NULL)
		{
			err = ESP_FAIL;
			ESP_LOGE(ACTUATION_APP_TAG, "%s, failed to create task: %s", __func__, ACTUATION_APP_TASK_NAME);
		}
	}
	else
	{
		ESP_LOGE(ACTUATION_APP_TAG, "%s, invalid argument", __func__);
	}

	return err;
}

void actuation_app_set_actions(const pivot_actions config_in, bool alert_change)
{
	memcpy(&actuation_config, &config_in, sizeof(actuation_config));

	if(alert_change == false)
	{
		gpio_actuator_set(config_in);
	}
	else
	{
		ESP_LOGW(ACTUATION_APP_TAG,"alert, manual configuration !!");

		if (eTaskGetState(xTask_actuation_app) != eSuspended)
		{
			xTaskNotifyGive(xTask_actuation_app);
		}
	}

	if(actuation_first_interaction == false)
	{
		if (eTaskGetState(xTask_actuation_app) != eSuspended)
		{
			xTaskNotifyGive(xTask_actuation_app);
			actuation_first_interaction = true;
		}
	}
}

void actuation_app_get_actions(pivot_actions* config_out, size_t config_size)
{
	pivot_actions current_action = {};

	if(config_size > 0 && config_out != NULL )
	{
		current_action = gpio_actuator_get();
		LOG_ACTUATION(ACTUATION_APP_TAG,"power_state %d", current_action.power_state);
		LOG_ACTUATION(ACTUATION_APP_TAG,"rotation %d", current_action.rotation);
		LOG_ACTUATION(ACTUATION_APP_TAG,"watering_state %d", current_action.watering_state);
		LOG_ACTUATION(ACTUATION_APP_TAG,"percentimeter %d", current_action.percentimeter);

		memcpy(config_out, &current_action, config_size);
	}
}

void actuation_app_set_pump(bool pump_state)
{
	if(pump_state)
	{
		gpio_actuator_pump_on(); //power on
	}
	else
	{
		gpio_actuator_pump_off(); //power off
	}
}

void actuation_app_shutdown(void)
{
	gpio_actuator_shutdown();
}

/* Private methods ----------------------------------------------- */
/**
 * @brief 	Task responsible for monitoring possible changes in equipment status
 * @param	arg - [in]: task argument (default NULL)
 */
void actuation_app_task(void* arg)
{
	pivot_actions current_action = {};
	TickType_t last_tick = xTaskGetTickCount();

	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	while(1)
	{
		current_action = gpio_actuator_get();

		if((current_action.power_state != actuation_config.power_state)
		&& (current_action.watering_state != PIVOT_PRESSURIZING))
		{
			if(pdTICKS_TO_MS(xTaskGetTickCount() - last_tick) > ACTUATION_APP_POWER_TIME)
			{
				LOG_ACTUATION(ACTUATION_APP_TAG,"power_state change");
				if(current_action.power_state == PIVOT_OFF)
				{
					actuation_app_manual_call(false, current_action);
				}
				else
				{
					actuation_app_manual_call(true, current_action);
				}

				ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
				last_tick = xTaskGetTickCount();
			}
		}
		else if((current_action.watering_state != actuation_config.watering_state)
				&& (current_action.watering_state != PIVOT_PRESSURIZING))
		{
			if(pdTICKS_TO_MS(xTaskGetTickCount() - last_tick) > ACTUATION_APP_WATERING_TIME)
			{
				LOG_ACTUATION(ACTUATION_APP_TAG,"watering_state change");
				if(current_action.watering_state == PIVOT_DRY)
				{
					actuation_app_manual_call(false, current_action);
				}
				else if(current_action.watering_state == PIVOT_WET)
				{
					actuation_config.watering_state = PIVOT_WET;
					actuation_app_manual_call(true, current_action);
				}

				last_tick = xTaskGetTickCount();
				ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
			}
		}
		else if(current_action.rotation != actuation_config.rotation && current_action.rotation != PIVOT_UNKNOWN)
		{
			if(pdTICKS_TO_MS(xTaskGetTickCount() - last_tick) > ACTUATION_APP_ROTATION_TIME)
			{
				LOG_ACTUATION(ACTUATION_APP_TAG,"rotation change");
				last_tick = xTaskGetTickCount();
				actuation_app_manual_call(true, current_action);
				ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
			}
		}
		else if(current_action.percentimeter > (actuation_config.percentimeter + 10) // 10% change in percent
			|| (current_action.percentimeter + 10) < actuation_config.percentimeter )
		{
			if(pdTICKS_TO_MS(xTaskGetTickCount() - last_tick) > ACTUATION_APP_PERCENTIMETER_TIME)
			{
				LOG_ACTUATION(ACTUATION_APP_TAG,"percentimeter change");
				last_tick = xTaskGetTickCount();
				actuation_app_manual_call(true, current_action);
				ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
			}
		}
		else
		{
			last_tick = xTaskGetTickCount();
		}

		vTaskDelay(pdMS_TO_TICKS(3000));
	}
}

void actuation_app_manual_call(bool on_off, pivot_actions current_action)
{
	if(on_off == true)
	{
		// send current action
		char str_out[200] = {};
		uint16_t dwp = 0;
		uint8_t idp = IDP_30;

		dwp = idp_parser_create_pwd(current_action);

		arg_pair_t arg_pairs[] = {
			{ "uint8_t", &idp },
			{ "uint16_t", &dwp },
			{ "uint8_t", &current_action.percentimeter },
			{ NULL, NULL }
		};

		idp_parser_create_package(str_out,arg_pairs);
		actuation_app_call(str_out, COMM_MQTT);
	}
	else
	{
		actuation_app_call("#30-off$", COMM_MQTT);
	}
}
