/**
 * @file actuation_app.c
 * @brief Actuation control class responsible for managing actuator actions.
 */

/* Self include */
#include "actuation_app.h"
#include "gpio_actuator.h"
#include "data_app.h"

#include "FreeRTOS_defines.h"
#include "idp_parser.h"
#include "log.h"

#include <string.h>

/* Private definitions ------------------------------------------- */

/**
 * @def ACTUATION_APP_TAG
 * @brief Tag used for logging within the actuation_app module.
 */
#define ACTUATION_APP_TAG "actuation_app"

/**
 * @def ACTUATION_APP_POWER_TIME
 * @brief Timeout duration for manual power configuration in milliseconds (10 seconds).
 */
#define ACTUATION_APP_POWER_TIME 15000    // 10 sec

/**
 * @def ACTUATION_APP_WATERING_TIME
 * @brief Timeout duration for manual watering configuration in milliseconds (30 seconds).
 */
#define ACTUATION_APP_WATERING_TIME 35000 // 30 sec

/**
 * @def ACTUATION_APP_ROTATION_TIME
 * @brief Timeout duration for manual rotation configuration in milliseconds (6 seconds).
 */
#define ACTUATION_APP_ROTATION_TIME 15000  // 6 sec

/**
 * @def ACTUATION_APP_PERCENTIMETER_TIME
 * @brief Timeout duration for manual percentimeter configuration in milliseconds (2 minutes).
 */
#define ACTUATION_APP_PERCENTIMETER_TIME 120000 // 2 min

/* Private variables  -------------------------------------------- */

static TaskHandle_t xTask_actuation_app = NULL; /**< Handle for the actuation_app task. */
static app_callback actuation_app_call = NULL; /**< Callback function for actuation events. */
static pivot_actions actuation_config = {}; /**< Current pivot actions configuration. */
static bool actuation_new_action = false;

const pivot_actions pivot_actions_off = {
    .power_state = PIVOT_OFF,
    .rotation = PIVOT_CW,
    .watering_state = PIVOT_DRY,
    .percentimeter = 0
};

/* Private methods  ---------------------------------------------- */

/**
 * @brief Task responsible for monitoring possible changes in equipment status.
 * @param arg [in]: Task argument (default NULL).
 */
void actuation_app_task(void* arg);

/**
 * @brief Perform a manual call based on the given parameters.
 * @param current_action [in]: The current pivot actions.
 */
void actuation_app_manual_call(pivot_actions current_action);


/* Public methods ------------------------------------------------ */
/**
 * @brief Initialize the actuation control class.
 * @param callback [in]: Callback function for actuation events.
 * @return esp_err_t: Error code indicating the success of the initialization.
 */
esp_err_t actuation_app_init(const app_callback callback)
{
	esp_err_t err = ESP_OK;

	err = gpio_actuator_init(callback);
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

/**
 * @brief Set the configuration for the actuator.
 * @param config [in]: The pivot configuration to set.
 * @return esp_err_t: Error code indicating the success of the configuration.
 */
esp_err_t actuation_app_set_config(pivot_config config)
{
	return gpio_actuator_config(config);
}

/**
 * @brief Set the barrier leaving time for the actuator.
 * @param barrier_config [in]: Configuration structure containing the barrier leaving time settings.
 */
void actuation_app_leaving_barrier_time(pivot_physical_config barrier_config)
{
	set_gpio_leaving_barrier_time(barrier_config);
}

/**
 * @brief Set the actions for the actuator.
 * @param config_in [in]: The pivot actions to set.
 * @param alert_change [in]: Flag indicating whether to alert about manual configuration.
 */
void actuation_app_set_actions(const pivot_actions config_in, bool alert_change)
{
	memcpy(&actuation_config, &config_in, sizeof(actuation_config));

	if(alert_change == false)
	{
		gpio_actuator_set(config_in);
		actuation_new_action = true;
	}
	else
	{
		ESP_LOGW(ACTUATION_APP_TAG,"Alert, Manual Configuration !!");
		ESP_LOGW(ACTUATION_APP_TAG,"%d%d%d-%d", config_in.rotation, config_in.watering_state,
		config_in.power_state, config_in.percentimeter);
		if(config_in.power_state == PIVOT_OFF)
		{
			gpio_actuator_set(config_in);
		}
	}

	if (eTaskGetState(xTask_actuation_app) == eSuspended
	|| eTaskGetState(xTask_actuation_app) == eBlocked)
	{
		xTaskNotifyGive(xTask_actuation_app);
	}
}

/**
 * @brief Get the current actions of the actuator.
 * @param config_out [out]: The buffer to store the current pivot actions.
 * @param config_size [in]: The size of the config_out buffer.
 */
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

		if(current_action.percentimeter > 100)
		{
			current_action.percentimeter = CONFIG_ACTIONS_UNDEF_VALUE;
		}
		memcpy(config_out, &current_action, config_size);
	}
}

/**
 * @brief Set the state of the pump.
 * @param pump_state [in]: The state to set (true for on, false for off).
 */
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

/**
 * @brief Shutdown the actuation control class.
 */
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
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	pivot_actions current_action = {};
	TickType_t last_tick = xTaskGetTickCount();

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
					actuation_app_manual_call(current_action);
				}
				else
				{
					actuation_app_manual_call(current_action);
				}

				last_tick = xTaskGetTickCount();
			}
		}
		else if((current_action.watering_state != actuation_config.watering_state)
				&& (current_action.watering_state != PIVOT_PRESSURIZING)
				&& (current_action.watering_state != PIVOT_UNKNOWN))
		{
			if(pdTICKS_TO_MS(xTaskGetTickCount() - last_tick) > ACTUATION_APP_WATERING_TIME)
			{
				LOG_ACTUATION(ACTUATION_APP_TAG,"watering_state change");
				if(current_action.watering_state == PIVOT_DRY)
				{
					actuation_app_manual_call(current_action);
				}
				else if(current_action.watering_state == PIVOT_WET)
				{
					actuation_config.watering_state = PIVOT_WET;
					actuation_app_manual_call(current_action);
				}

				last_tick = xTaskGetTickCount();
			}
		}
		else if(current_action.rotation != actuation_config.rotation && current_action.rotation != PIVOT_UNKNOWN)
		{
			if(pdTICKS_TO_MS(xTaskGetTickCount() - last_tick) > ACTUATION_APP_ROTATION_TIME)
			{
				LOG_ACTUATION(ACTUATION_APP_TAG,"rotation change");
				last_tick = xTaskGetTickCount();
				actuation_app_manual_call(current_action);
			}
		}
		// else if(current_action.percentimeter > (actuation_config.percentimeter + 10) // 10% change in percent
		// 	|| (current_action.percentimeter + 10) < actuation_config.percentimeter )
		// {
		// 	if(pdTICKS_TO_MS(xTaskGetTickCount() - last_tick) > ACTUATION_APP_PERCENTIMETER_TIME)
		// 	{
		// 		LOG_ACTUATION(ACTUATION_APP_TAG,"percentimeter change");
		// 		last_tick = xTaskGetTickCount();
		// 		actuation_app_manual_call(current_action);
		// 	}
		// }
		else
		{
			last_tick = xTaskGetTickCount();
		}

		if(actuation_new_action)
		{
			actuation_new_action = false;
			last_tick = xTaskGetTickCount();
		}

		vTaskDelay(pdMS_TO_TICKS(10000));
	}
}

/**
 * @brief Perform a manual call based on the given parameters.
 * @param current_action [in]: The current pivot actions.
 */
void actuation_app_manual_call(pivot_actions current_action)
{
	// send current action
	char str_out[200] = {};
	uint16_t dwp = 0;
	uint8_t idp = IDP_30;

	memcpy(&actuation_config, &current_action, sizeof(actuation_config));
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
