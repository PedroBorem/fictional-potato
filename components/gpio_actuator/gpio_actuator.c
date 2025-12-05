/**
 * @file gpio_actuator.c
 * @date June 20, 2022
 * @brief Implementation of GPIO actuator control functions.
 */

/* Self include */
#include "gpio_actuator.h"

/* Peripherals include */
#include <time.h>
#include <string.h>

#include "FreeRTOS_defines.h"
#include "log.h"

/* Private definitions ------------------------------------------- */

/**
 * @brief Tag used for logging in the GPIO actuator module.
 */
#define GPIO_ACT_TAG		"gpio_actuator"

/**
 * @brief Default interrupt flag for GPIO handling.
 */
#define GPIO_ACT_INTR_FLAG_DEFAULT 	0

/* Global variables ---------------------------------------------- */
// Callback variables
static app_callback gpio_actuator_callback = NULL;

// FreeRTOS variables
static TimerHandle_t perc_timer_handleOn = NULL;
static TimerHandle_t perc_timer_handleOff = NULL;
static TaskHandle_t xTask_waitpressure = NULL;
static TaskHandle_t xTask_readpercent = NULL;

// Actions duration variables
static pivot_actions pivot_actions_read = {};
static pivot_actions task_actions_set = {};

// GPIO variable time to start
static uint16_t gpio_act_time_to_start = 1000;

// Percentimeter variables
static uint64_t posedge_perc = 0;
static uint64_t negedge_perc = 0;
static uint32_t perc_diff_onoff = 0;
static uint32_t perc_pct_on = 0;
static uint32_t perc_sec_on = 0;
static uint64_t percent_watchdog = 0;

/* Configuration variables */
static uint32_t gpio_act_pressure_timeout = 0;
static uint32_t gpio_act_on_delay = 0;
static uint32_t gpio_act_off_delay = 0;
static uint32_t gpio_act_leaving_barrier_delay = 0;
static bool gpio_act_contactor_type = 0;
static bool gpio_act_pressure_type = 0;

// Rotation variable
static pivot_actions pivot_rotation = {
	.rotation = PIVOT_UNKNOWN
};

// Pressurizing flag
static bool pressurizing = false;

/* Private methods declarations ---------------------------------- */

/**
 * @brief Callback function for the expiration of the Perc On timer.
 * @param pxTimer The timer handle
 */
void vPercTimerOnExpire(TimerHandle_t pxTimer);

/**
 * @brief Callback function for the expiration of the Perc Off timer.
 * @param pxTimer The timer handle
 */
void vPercTimerOffExpire(TimerHandle_t pxTimer);

/**
 * @brief Start the relays accordingly after pressure check or dry mode.
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Fail to initialize
 */
esp_err_t gpio_actuator_start(void);

/**
 * @brief Water pressure application task.
 * @param arg Task argument (default NULL)
 */
void actuator_wait_pressure(void* arg);

/**
 * @brief Percentimeter reading task.
 * @param arg Task argument (default NULL)
 */
void actuator_read_percent(void* arg);


/* Public methods ------------------------------------------------ */

/**
 * @brief GPIO ISR handler for percentimeter interrupt.
 * @param arg ISR handler argument.
 */
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
	if(xTask_readpercent != NULL)
	{
		if (eTaskGetState(xTask_readpercent) == eSuspended
		|| eTaskGetState(xTask_readpercent) == eBlocked)
		{
			vTaskResume(xTask_readpercent);
		}
	}
}

/**
 * @brief Initializes the GPIO actuator module.
 *
 * This function initializes the GPIO actuator module, configures GPIO pins, and sets up timers and tasks.
 *
 * @return esp_err_t Error code indicating the success of the operation.
 */
esp_err_t gpio_actuator_init(const app_callback callback)
{
	esp_err_t err = ESP_FAIL;

	if(callback != NULL)
	{
		gpio_actuator_callback = callback;
	}
	else
	{
		return err;
	}

	perc_pct_on = 0;
	perc_sec_on = 0;

	//output actions
	gpio_config_t io_conf_out = {};
	io_conf_out.intr_type = GPIO_INTR_DISABLE;
	io_conf_out.mode = GPIO_MODE_OUTPUT_OD;
	io_conf_out.pin_bit_mask = GPIO_OUTPUT_PIN_GROUP;
	io_conf_out.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf_out.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf_out);

    gpio_set_level(GPIO_ACT_PIN_OFF, GPIO_ACT_SYS_DISABLE);
	gpio_set_level(GPIO_ACT_PIN_ON, GPIO_ACT_SYS_DISABLE);
	gpio_set_level(GPIO_ACT_PIN_AUX, GPIO_ACT_SYS_DISABLE);
	gpio_set_level(GPIO_ACT_PIN_CW, GPIO_ACT_SYS_DISABLE);
	gpio_set_level(GPIO_ACT_PIN_CCW, GPIO_ACT_SYS_DISABLE);
	gpio_set_level(GPIO_ACT_PIN_WATERING, GPIO_ACT_SYS_DISABLE);
	gpio_set_level(GPIO_ACT_PIN_PERC_AUX, GPIO_ACT_SYS_DISABLE);
	gpio_set_level(GPIO_ACT_PIN_PERC_OUT, GPIO_ACT_SYS_DISABLE);
	gpio_set_level(GPIO_ACT_PIN_PUMP, GPIO_ACT_SYS_DISABLE);

    //input configuration
	gpio_config_t io_conf_in = {};
	io_conf_in.intr_type = GPIO_INTR_DISABLE;
	io_conf_in.mode = GPIO_MODE_INPUT;
	io_conf_in.pin_bit_mask = GPIO_INPUT_PIN_GROUP;
	io_conf_in.pull_down_en = GPIO_PULLDOWN_ENABLE;
	io_conf_in.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf_in);

    //percent reader interrupt setup
	gpio_config_t io_conf_int = {};
    io_conf_int.intr_type = GPIO_INTR_ANYEDGE;
    io_conf_int.pin_bit_mask = GPIO_INT_PERC;
    io_conf_int.mode = GPIO_MODE_INPUT;
    io_conf_int.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf_int.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf_int);

    if(xTask_readpercent == NULL)
	{
		BaseType_t xReturn = xTaskCreate(&actuator_read_percent,
								ACTUATOR_PERCENT_TASK_NAME,
								ACTUATOR_PERCENT_STACK_SIZE,
								NULL,
								ACTUATOR_PERCENT_TASK_PRIORITY,
								&xTask_readpercent);

		if(xReturn != pdPASS || xTask_readpercent == NULL)
		{
			ESP_LOGE(GPIO_ACT_TAG, "%s, Failed to create task: %s", __func__, ACTUATOR_PERCENT_TASK_NAME);
			return ESP_FAIL;
		}
	}
	else
	{
		vTaskResume(xTask_readpercent);
	}

    err = gpio_install_isr_service(GPIO_ACT_INTR_FLAG_DEFAULT);
    if(err != ESP_OK)
    {
    	ESP_LOGE(GPIO_ACT_TAG, "%s, Install ISR failed with error: %d", __func__, err);
    }

    err = gpio_isr_handler_add(GPIO_ACT_PIN_PERC_IN, gpio_isr_handler, (void*) GPIO_ACT_PIN_PERC_IN);
    if(err != ESP_OK)
    {
    	ESP_LOGE(GPIO_ACT_TAG, "%s, ISR handler add failed with error: %d", __func__, err);
    }

	return err;
}

/**
 * @brief Set the delay time for the GPIO actuator when leaving the barrier.
 * @param barrier_config [in]: Configuration structure containing the time leaving barrier settings.
 */
void set_gpio_leaving_barrier_time(pivot_physical_config barrier_config)
{
	gpio_act_leaving_barrier_delay = (barrier_config.time_leaving_barrier * 1000);
}

/**
* @brief Sets the time to start for the GPIO actuator based on the barrier status.
* 
* This function sets the time to start for the GPIO actuator based on whether the pivot is leaving the barrier,
* inside the barrier, or outside the barrier.
*
* @param barrier_status The status of the barrier (PIVOT_LEAVING_THE_BARRIER, PIVOT_IN_THE_BARRIER, or others).
* @return esp_err_t Error code indicating the success of the operation.
*/

esp_err_t gpio_actuator_set_time_start(barrier_status barrier_status)
{	
	esp_err_t err = ESP_FAIL;

	if(barrier_status == PIVOT_LEAVING_THE_BARRIER)
	{
		gpio_act_time_to_start = gpio_act_leaving_barrier_delay;
	}
	else if(barrier_status == PIVOT_IN_THE_BARRIER)
	{
		gpio_act_time_to_start = HIGH_LOGIC_LEVEL_TIME_AGAINST_BARRIER;
	}
	else
	{
		gpio_act_time_to_start = gpio_act_on_delay;
		// gpio_act_time_to_start = HIGH_LOGIC_LEVEL_TIME_OUTSIDE_BARRIER;
	}

	err = ESP_OK;

	return err;
}
/**
 * @brief Configures the GPIO actuator module based on the provided configuration.
 *
 * This function configures the GPIO actuator module based on the provided configuration.
 *
 * @param config Pivot configuration.
 * @return esp_err_t Error code indicating the success of the operation.
 */
esp_err_t gpio_actuator_config(pivot_config config)
{
	esp_err_t err = ESP_FAIL;

	/* configuration variables */
	if(strcmp(config.contactor, "NA") == 0)
	{
		gpio_act_contactor_type = 0;
	}
	else if(strcmp(config.contactor, "NF") == 0)
	{
		gpio_act_contactor_type = 1;
	}
	else
	{
		ESP_LOGE(GPIO_ACT_TAG,"Invalid contactor type configuration");
		LOG_DBG_ERROR(GPIO_ACT_TAG, "Invalid_contactor_type");
		return err;
	}

	if(strcmp(config.pressure, "NA") == 0)
	{
		gpio_act_pressure_type = 0;
	}
	else if(strcmp(config.pressure, "NF") == 0)
	{
		gpio_act_pressure_type = 1;
	}
	else
	{
		ESP_LOGE(GPIO_ACT_TAG,"Invalid pressure type configuration");
		LOG_DBG_ERROR(GPIO_ACT_TAG, "Invalid_pressure_type");
		return err;
	}

	// convert sec to mili;
	gpio_act_pressure_timeout = (config.pressurization_time * 1000);
	gpio_act_on_delay = (config.on_time * 1000);
	gpio_act_off_delay = (config.off_time * 1000);

	// TODO testar no pivo o tempo de retorno

	return ESP_OK;
}

/**
 * @brief Controls the relay for percentage-based actions.
 *
 * This function controls the relay for percentage-based actions, such as adjusting the pivot angle.
 *
 * @param actions Pivot actions containing the percentage information.
 */
void percent_relay_control(pivot_actions actions, uint16_t perc_sec)
{
    if (actions.percentimeter > 0 && actions.percentimeter < 100)
    {

		perc_timer_handleOn = xTimerCreate(
			"percTimerON",           /* name */
			pdMS_TO_TICKS(perc_sec), /* period/time */
			pdFALSE,                 /* auto reload */
			(void *)0,               /* timer ID */
			vPercTimerOnExpire);     /* callback */
		
		if(GPIO_ACT_PERC_FULL_CYCLE - perc_sec > 0)
		{	
			perc_timer_handleOff = xTimerCreate(
				"percTimerOff",                                       /* name */
				pdMS_TO_TICKS((GPIO_ACT_PERC_FULL_CYCLE - perc_sec)), /* period/time */
				pdFALSE,                                              /* auto reload */
				(void *)0,                                            /* timer ID */
				vPercTimerOffExpire);     							  /* callback */
		}
		else
		{
			perc_timer_handleOff = xTimerCreate(
			"percTimerOff",                                       /* name */
			pdMS_TO_TICKS((GPIO_ACT_PERC_FULL_CYCLE)), 			  /* period/time */
			pdFALSE,                                              /* auto reload */
			(void *)0,                                            /* timer ID */
			vPercTimerOffExpire);     							  /* callback */
		}

        if ((perc_timer_handleOn != NULL) && (perc_timer_handleOff != NULL))
        {
            gpio_set_level(GPIO_ACT_PIN_PERC_AUX, GPIO_ACT_SYS_ENABLE);
            gpio_set_level(GPIO_ACT_PIN_PERC_OUT, GPIO_ACT_SYS_ENABLE);
            xTimerStart(perc_timer_handleOn, pdMS_TO_TICKS(1000));
        }
    }
    else if (actions.percentimeter == 0)
    {
        gpio_set_level(GPIO_ACT_PIN_PERC_OUT, GPIO_ACT_SYS_DISABLE);
        gpio_set_level(GPIO_ACT_PIN_PERC_AUX, GPIO_ACT_SYS_DISABLE);

        if (perc_timer_handleOn != NULL)
        {
            xTimerStop(perc_timer_handleOn, pdMS_TO_TICKS(1000));
            xTimerDelete(perc_timer_handleOn, pdMS_TO_TICKS(1000));
			perc_timer_handleOn = NULL;
            negedge_perc = 0;
        }

        if (perc_timer_handleOff != NULL)
        {
            xTimerStop(perc_timer_handleOff, pdMS_TO_TICKS(1000));
            xTimerDelete(perc_timer_handleOff, pdMS_TO_TICKS(1000));
			perc_timer_handleOff = NULL;
            posedge_perc = 0;
        }

        pivot_actions_read.percentimeter = 0;
    }
    else if (actions.percentimeter == 100)
    {
        gpio_set_level(GPIO_ACT_PIN_PERC_OUT, GPIO_ACT_SYS_ENABLE);
        gpio_set_level(GPIO_ACT_PIN_PERC_AUX, GPIO_ACT_SYS_ENABLE);

        if (perc_timer_handleOn != NULL)
        {
            xTimerStop(perc_timer_handleOn, pdMS_TO_TICKS(1000));
            xTimerDelete(perc_timer_handleOn, pdMS_TO_TICKS(1000));
			perc_timer_handleOn = NULL;
        }

        if (perc_timer_handleOff != NULL)
        {
            xTimerStop(perc_timer_handleOff, pdMS_TO_TICKS(1000));
            xTimerDelete(perc_timer_handleOff, pdMS_TO_TICKS(1000));
			perc_timer_handleOff = NULL;
        }
    }
}

/**
 * @brief Controls the relay for water pump actions.
 *
 * This function controls the relay for water pump actions based on the pivot's watering state.
 *
 * @param actions Pivot actions containing the watering state information.
 */
void water_pump_relay_control(pivot_actions actions)
{
    if (actions.watering_state == PIVOT_DRY)
    {
        gpio_set_level(GPIO_ACT_PIN_WATERING, GPIO_ACT_SYS_DISABLE);
        gpio_actuator_pressure_off();
        gpio_actuator_start();
    }
    else if (actions.watering_state == PIVOT_WET)
    {
		gpio_set_level(GPIO_ACT_PIN_WATERING, GPIO_ACT_SYS_ENABLE);
		gpio_actuator_pressure_on();
    }
}

/**
 * @brief Controls the relay for pivot rotation actions.
 *
 * This function controls the relay for pivot rotation actions, such as clockwise or counterclockwise rotation.
 *
 * @param actions Pivot actions containing the rotation information.
 */
void rotation_relay_control(pivot_actions actions)
{
    if (actions.rotation == PIVOT_CW)
    {
        gpio_set_level(GPIO_ACT_PIN_CW, GPIO_ACT_SYS_ENABLE);
        gpio_set_level(GPIO_ACT_PIN_AUX, GPIO_ACT_SYS_ENABLE);
        gpio_set_level(GPIO_ACT_PIN_CCW, GPIO_ACT_SYS_DISABLE);
    }
    else if (actions.rotation == PIVOT_CCW)
    {
        gpio_set_level(GPIO_ACT_PIN_CW, GPIO_ACT_SYS_DISABLE);
        gpio_set_level(GPIO_ACT_PIN_AUX, GPIO_ACT_SYS_ENABLE);
        gpio_set_level(GPIO_ACT_PIN_CCW, GPIO_ACT_SYS_ENABLE);
    }
}

/**
 * @brief Sets the GPIO actuator actions based on the provided actions.
 *
 * This function sets the GPIO actuator actions based on the provided actions.
 *
 * @param actions Pivot actions to set.
 * @return esp_err_t Error code indicating the success of the operation.
 */
esp_err_t gpio_actuator_set(pivot_actions actions)
{
	esp_err_t err = ESP_FAIL;
	uint16_t perc_sec = 0;

	//task_actions_set = actions;
	memcpy(&task_actions_set, &actions, sizeof(task_actions_set));
	perc_sec = actions.percentimeter*((GPIO_ACT_PERC_FULL_CYCLE)/100);

	LOG_ACTUATION(GPIO_ACT_TAG,"%s, Perc sec: %d", __func__, perc_sec);
	
	if(actions.power_state == PIVOT_ON)
	{
		if(actions.watering_state == PIVOT_DRY)
		{
			// if dry mode, start rotation and percent control
			rotation_relay_control(actions);
			percent_relay_control(actions, perc_sec);
			
		}
		
		//otherwise, wait for pressure to stabilize to start rotation and percent control
		// this will be done in the pressure task
		water_pump_relay_control(actions);

	}
	else if(actions.power_state == PIVOT_OFF)
	{
		if(actions.rotation == PIVOT_SUSPENDED)
		{
			pivot_rotation.rotation = PIVOT_SUSPENDED;
		}
		gpio_actuator_shutdown();
	}

	err = ESP_OK;

	return err;
}

/**
 * @brief Gets the current GPIO actuator state.
 *
 * This function retrieves the current GPIO actuator state.
 *
 * @return pivot_actions Current pivot actions.
 */
pivot_actions gpio_actuator_get(void)
{
	if(gpio_get_level(GPIO_ACT_PIN_CW_IN) == gpio_act_contactor_type)
	{
		pivot_actions_read.rotation = PIVOT_CW;
		pivot_actions_read.power_state = PIVOT_ON;
	}
	else if(gpio_get_level(GPIO_ACT_PIN_CCW_IN) == gpio_act_contactor_type)
	{
		pivot_actions_read.rotation = PIVOT_CCW;
		pivot_actions_read.power_state = PIVOT_ON;
	}
	else
	{
		pivot_actions_read.power_state = PIVOT_OFF;
		pivot_actions_read.rotation = pivot_rotation.rotation;
	}

	if(pressurizing == true)
	{
		pivot_actions_read.watering_state = PIVOT_PRESSURIZING;
	}
	else if(gpio_get_level(GPIO_ACT_PIN_PRESS) == gpio_act_pressure_type)
	{
		pivot_actions_read.watering_state = PIVOT_WET;
	}
	else if(gpio_get_level(GPIO_ACT_PIN_PRESS) == !gpio_act_pressure_type)
	{
		pivot_actions_read.watering_state = PIVOT_DRY;
	}

	if(((clock() - percent_watchdog)/CLOCKS_PER_SEC) > 70)
	{
		if(gpio_get_level(GPIO_ACT_PIN_PERC_IN) == gpio_act_contactor_type)
		{
			pivot_actions_read.percentimeter = 100;
		}
		else if(gpio_get_level(GPIO_ACT_PIN_PERC_IN) == !gpio_act_contactor_type)
		{
			pivot_actions_read.percentimeter = 0;
		}
	}

	return pivot_actions_read;
}

/**
 * @brief Shuts down the GPIO actuator module.
 *
 * This function shuts down the GPIO actuator module by turning off relays and cleaning up resources.
 */
void gpio_actuator_shutdown(void)
{
	gpio_set_level(GPIO_ACT_PIN_OFF, GPIO_ACT_SYS_ENABLE);
	gpio_set_level(GPIO_ACT_PIN_ON, GPIO_ACT_SYS_DISABLE);
	gpio_set_level(GPIO_ACT_PIN_AUX, GPIO_ACT_SYS_DISABLE);
	gpio_set_level(GPIO_ACT_PIN_CW, GPIO_ACT_SYS_DISABLE);
	gpio_set_level(GPIO_ACT_PIN_CCW, GPIO_ACT_SYS_DISABLE);
	gpio_set_level(GPIO_ACT_PIN_WATERING, GPIO_ACT_SYS_DISABLE);
	gpio_set_level(GPIO_ACT_PIN_PERC_AUX, GPIO_ACT_SYS_DISABLE);
	gpio_set_level(GPIO_ACT_PIN_PERC_OUT, GPIO_ACT_SYS_DISABLE);

	vTaskDelay(pdMS_TO_TICKS(gpio_act_off_delay));
	gpio_set_level(GPIO_ACT_PIN_OFF, GPIO_ACT_SYS_DISABLE);
	
	if(perc_timer_handleOn != NULL)
	{
		xTimerStop(perc_timer_handleOn, pdMS_TO_TICKS(1000));
		xTimerDelete(perc_timer_handleOn, pdMS_TO_TICKS(1000));
		perc_timer_handleOn = NULL;
	}
	if(perc_timer_handleOff != NULL)
	{
		xTimerStop(perc_timer_handleOff, pdMS_TO_TICKS(1000));
		xTimerDelete(perc_timer_handleOff, pdMS_TO_TICKS(1000));
		perc_timer_handleOff = NULL;
	}
	
	pivot_actions_read.percentimeter = 0;
}

/**
 * @brief Turns on the pump in the GPIO actuator module.
 */
void gpio_actuator_pump_on(void)
{
	gpio_set_level(GPIO_ACT_PIN_PUMP, GPIO_ACT_SYS_ENABLE);
}

/**
 * @brief Turns off the pump in the GPIO actuator module.
 */
void gpio_actuator_pump_off(void)
{
	gpio_set_level(GPIO_ACT_PIN_PUMP, GPIO_ACT_SYS_DISABLE);
}

/**
 * @brief Turns on pressure application in the GPIO actuator module.
 */
void gpio_actuator_pressure_on(void)
{
	if(xTask_waitpressure == NULL)
	{
		BaseType_t xReturn = xTaskCreate(&actuator_wait_pressure,
								ACTUATOR_CHECK_TASK_NAME,
								ACTUATOR_CHECK_STACK_SIZE,
								NULL,
								ACTUATOR_CHECK_TASK_PRIORITY,
								&xTask_waitpressure);
		if(xReturn != pdPASS || xTask_waitpressure == NULL)
		{
			ESP_LOGE(GPIO_ACT_TAG, "%s, failed to create task: %s", __func__, ACTUATOR_CHECK_TASK_NAME);
		}
	}
	else
	{
		vTaskResume(xTask_waitpressure);
	}
}

/**
 * @brief Turns off pressure application in the GPIO actuator module.
 */
void gpio_actuator_pressure_off(void)
{
	pressurizing = false;

	if(xTask_waitpressure != NULL)
	{
		vTaskDelete(xTask_waitpressure);
		xTask_waitpressure = NULL;
	}
}

/* Private methods  ---------------------------------------------- */
/**
 * @brief Callback function for the expiration of the Perc On timer.
 * @param pxTimer The timer handle
 */
void vPercTimerOnExpire(TimerHandle_t pxTimer)
{
	gpio_set_level(GPIO_ACT_PIN_PERC_OUT, GPIO_ACT_SYS_DISABLE);
	if(perc_timer_handleOff != NULL)
	{
		xTimerStart(perc_timer_handleOff, pdMS_TO_TICKS(5000));
	}
}

/**
 * @brief Callback function for the expiration of the Perc Off timer.
 * @param pxTimer The timer handle
 */
void vPercTimerOffExpire(TimerHandle_t pxTimer)
{
	gpio_set_level(GPIO_ACT_PIN_PERC_OUT, GPIO_ACT_SYS_ENABLE);
	if(perc_timer_handleOn != NULL)
	{
		xTimerStart(perc_timer_handleOn, pdMS_TO_TICKS(5000));
	}
}

/**
 * @brief Start the relays accordingly after pressure check or dry mode.
 * @param on_barrier parameter that indicates whether the pivot is in the barrier
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Fail to initialize
 */
esp_err_t gpio_actuator_start(void)
{
	esp_err_t err = ESP_FAIL;

	vTaskDelay(pdMS_TO_TICKS(500));
	gpio_set_level(GPIO_ACT_PIN_ON, GPIO_ACT_SYS_ENABLE);
	vTaskDelay(pdMS_TO_TICKS(gpio_act_time_to_start));
	gpio_set_level(GPIO_ACT_PIN_ON, GPIO_ACT_SYS_DISABLE);

	err = ESP_OK;
	return err;
}

/**
 * @brief Water pressure application task.
 * @param arg Task argument (default NULL)
 */
void actuator_wait_pressure(void* arg)
{
	TickType_t check_start = xTaskGetTickCount();

	pressurizing = true;

	while(1)
	{
		LOG_ACTUATION(GPIO_ACT_TAG,"%s, Result: %lud",__func__, pdTICKS_TO_MS(xTaskGetTickCount() - check_start));
		if(gpio_get_level(GPIO_ACT_PIN_PRESS) == gpio_act_pressure_type)
		{
			LOG_ACTUATION(GPIO_ACT_TAG,"%s, Pressure detected", __func__);
			vTaskDelay(pdMS_TO_TICKS(5000));

			if(gpio_get_level(GPIO_ACT_PIN_PRESS) == gpio_act_pressure_type)
			{
				//stable pressure for 5 seconds
				LOG_ACTUATION(GPIO_ACT_TAG,"%s, Stable pressure confirmed", __func__);
				//start rotation and percent control
				uint16_t perc_sec = task_actions_set.percentimeter * (GPIO_ACT_PERC_FULL_CYCLE / 100);
				LOG_ACTUATION(GPIO_ACT_TAG,"%s, Perc sec: %d", __func__, perc_sec);

				rotation_relay_control(task_actions_set);
				percent_relay_control(task_actions_set, perc_sec);

				//wait additional 3 seconds before turning on the system
				vTaskDelay(pdMS_TO_TICKS(3000));
				
				//system on
				gpio_actuator_start();
				pressurizing = false;

				// send current action
				gpio_actuator_callback("#00$", comm_main_mode);

				//suspend own task
				vTaskSuspend(NULL);

				//task resume
				pressurizing = true;
				check_start = xTaskGetTickCount();
			}
		}
		else if((pdTICKS_TO_MS(xTaskGetTickCount() - check_start)) > gpio_act_pressure_timeout)
		{
			// Local shutdown
			ESP_LOGE(GPIO_ACT_TAG, "%s, Water Pressure timeout", __func__);

			gpio_set_level(GPIO_ACT_PIN_OFF, GPIO_ACT_SYS_ENABLE);
			gpio_set_level(GPIO_ACT_PIN_ON, GPIO_ACT_SYS_DISABLE);
			gpio_set_level(GPIO_ACT_PIN_AUX, GPIO_ACT_SYS_DISABLE);
			gpio_set_level(GPIO_ACT_PIN_CW, GPIO_ACT_SYS_DISABLE);
			gpio_set_level(GPIO_ACT_PIN_CCW, GPIO_ACT_SYS_DISABLE);
			gpio_set_level(GPIO_ACT_PIN_WATERING, GPIO_ACT_SYS_DISABLE);
			gpio_set_level(GPIO_ACT_PIN_PERC_AUX, GPIO_ACT_SYS_DISABLE);
			gpio_set_level(GPIO_ACT_PIN_PERC_OUT, GPIO_ACT_SYS_DISABLE);
			gpio_set_level(GPIO_ACT_PIN_PUMP, GPIO_ACT_SYS_DISABLE);

			vTaskDelay(pdMS_TO_TICKS(500));

			gpio_set_level(GPIO_ACT_PIN_OFF, GPIO_ACT_SYS_DISABLE);

			if(perc_timer_handleOn != NULL)
			{
				xTimerStop(perc_timer_handleOn,portMAX_DELAY);
				xTimerDelete(perc_timer_handleOn,portMAX_DELAY);
				perc_timer_handleOn = NULL;
			}
			if(perc_timer_handleOff != NULL)
			{
				xTimerStop(perc_timer_handleOff,portMAX_DELAY);
				xTimerDelete(perc_timer_handleOff,portMAX_DELAY);
				perc_timer_handleOff = NULL;
			}

			pivot_actions_read.percentimeter = 0;
			pressurizing = false;

			//suspend own task
			vTaskSuspend(NULL);

			//task resume
			pressurizing = true;
			check_start = xTaskGetTickCount();
		}

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

/**
 * @brief Percentimeter reading task.
 * @param arg Task argument (default NULL)
 */
void actuator_read_percent(void* arg)
{
	while(1)
	{
		//suspend own task
		vTaskSuspend(NULL);

		if(gpio_get_level(GPIO_ACT_PIN_PERC_IN) == gpio_act_contactor_type)
		{
			posedge_perc = clock();
		}

		if(gpio_get_level(GPIO_ACT_PIN_PERC_IN) == !gpio_act_contactor_type)
		{
			negedge_perc = clock();

			if(posedge_perc != 0 && negedge_perc != 0)
			{
				perc_diff_onoff = (negedge_perc - posedge_perc);
				if (perc_diff_onoff != 0)
				{
					perc_sec_on = perc_diff_onoff / CLOCKS_PER_SEC;
					perc_pct_on = (perc_sec_on * 100) / (GPIO_ACT_PERC_FULL_CYCLE / 1000);

					if(perc_pct_on <= 100)
					{
						pivot_actions_read.percentimeter = perc_pct_on;
					}
				}
			}
		}

		percent_watchdog = clock();
		vTaskDelay(pdMS_TO_TICKS(200));
	}
}
