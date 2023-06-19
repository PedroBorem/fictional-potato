/*
 * gpio_actuator.c
 *
 *  Created on: Jun 20, 2022
 *      Author: guilhermerossi
 */

/**
 * @file gpio_actuator.c
 * @date June 20, 2022
 * @brief general gpio functions
*/

/* Self include */
#include "gpio_actuator.h"

/* Peripherals include */
#include <time.h>

/* Private definitions ------------------------------------------- */
#define GPIO_ACT_TAG		"gpio_actuator"
#define GPIO_ACT_INTR_FLAG_DEFAULT 	0

/**\addtogroup components
 * @{
 *
 */

/**\addtogroup gpio_actuator
 * @{
 *
 */

/* Global variables ---------------------------------------------- */

//FreeRTOS variables
static TimerHandle_t perc_timer_handleOn = NULL;
static TimerHandle_t perc_timer_handleOff = NULL;
static TaskHandle_t xTask_waitpressure = NULL;
static TaskHandle_t xTask_readpercent = NULL;

//Configuration variables
static pivot_actions pivot_config_read = {};
static pivot_actions task_config_set = {};

//Percentimeter variables
static uint64_t posedge_perc = 0;
static uint64_t negedge_perc = 0;
static uint32_t perc_diff_onoff = 0;
static uint32_t perc_pct_on = 0;
static uint32_t perc_sec_on = 0;
static uint64_t percent_watchdog = 0;

//Pressurizing flag
static bool pressurizing = false;

/* Private methods declarations ---------------------------------- */
/**
 * @brief	Perc On timer expiration callback.
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail to initialize
 */
void vPercTimerOnExpire(TimerHandle_t pxTimer);

/**
 * @brief	Perc Off timer expiration callback.
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail to initialize
 */
void vPercTimerOffExpire(TimerHandle_t pxTimer);

/**
 * @brief	Start relays accordingly, after pressure check or dry mode
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail to initialize
 */
esp_err_t gpio_actuator_start(void);

/**
 * @brief 	Water pressure application task.
 * @param	arg - [in]: task argument (default NULL)
 */
void actuator_wait_pressure(void* arg);


void actuator_read_percent(void* arg);

/* Public methods ------------------------------------------------ */
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
	if(xTask_readpercent != NULL)
	{
		vTaskResume(xTask_readpercent);
	}
}

esp_err_t gpio_actuator_init()
{
	esp_err_t err = ESP_FAIL;

	perc_pct_on = 0;
	perc_sec_on = 0;

	//output configuration
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
			ESP_LOGE(GPIO_ACT_TAG, "%s, failed to create task: %s", __func__, ACTUATOR_PERCENT_TASK_NAME);
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

esp_err_t gpio_actuator_set(pivot_actions config)
{
	esp_err_t err = ESP_FAIL;
	int perc_sec = 0;

	//task_config_set = config;
	memcpy(&task_config_set, &config, sizeof(task_config_set));
	perc_sec = config.percentimeter*((GPIO_ACT_PERC_FULL_CYCLE)/100);

	LOG_ACTUATION(GPIO_ACT_TAG,"%s, Perc sec: %d", __func__, perc_sec);

	if(config.power_state == PIVOT_ON)
	{
		if(config.rotation == PIVOT_CW)
		{
			gpio_set_level(GPIO_ACT_PIN_CW, GPIO_ACT_SYS_ENABLE);
			gpio_set_level(GPIO_ACT_PIN_AUX, GPIO_ACT_SYS_ENABLE);
			gpio_set_level(GPIO_ACT_PIN_CCW, GPIO_ACT_SYS_DISABLE);
		}
		else if(config.rotation == PIVOT_CCW)
		{
			gpio_set_level(GPIO_ACT_PIN_CW, GPIO_ACT_SYS_DISABLE);
			gpio_set_level(GPIO_ACT_PIN_AUX, GPIO_ACT_SYS_ENABLE);
			gpio_set_level(GPIO_ACT_PIN_CCW, GPIO_ACT_SYS_ENABLE);
		}

		if(config.percentimeter > 0 && config.percentimeter < 100)
		{
			perc_timer_handleOn = xTimerCreate(
					  "percTimerON", /* name */
					  pdMS_TO_TICKS(perc_sec), /* period/time */
					  pdFALSE, /* auto reload */
					  (void*)0, /* timer ID */
					  vPercTimerOnExpire); /* callback */

			perc_timer_handleOff = xTimerCreate(
					  "percTimerOff", /* name */
					  pdMS_TO_TICKS((GPIO_ACT_PERC_FULL_CYCLE-perc_sec)), /* period/time */
					  pdFALSE, /* auto reload */
					  (void*)0, /* timer ID */
					  vPercTimerOffExpire); /* callback */

			if((perc_timer_handleOn != NULL) && (perc_timer_handleOff != NULL))
			{
				gpio_set_level(GPIO_ACT_PIN_PERC_AUX, GPIO_ACT_SYS_ENABLE);
				gpio_set_level(GPIO_ACT_PIN_PERC_OUT, GPIO_ACT_SYS_ENABLE);
				xTimerStart(perc_timer_handleOn, 0);
			}

		}
		else if(config.percentimeter == 0)
		{
			gpio_set_level(GPIO_ACT_PIN_PERC_OUT, GPIO_ACT_SYS_DISABLE);
			gpio_set_level(GPIO_ACT_PIN_PERC_AUX, GPIO_ACT_SYS_DISABLE);

			if(perc_timer_handleOn != 0)
			{
				xTimerDelete(perc_timer_handleOn,0);
				negedge_perc = 0;
			}

			if(perc_timer_handleOff != 0)
			{
				xTimerDelete(perc_timer_handleOff,0);
				posedge_perc = 0;
			}

			pivot_config_read.percentimeter = 0;
		}
		else if(config.percentimeter == 100)
		{
			gpio_set_level(GPIO_ACT_PIN_PERC_OUT, GPIO_ACT_SYS_ENABLE);
			gpio_set_level(GPIO_ACT_PIN_PERC_AUX, GPIO_ACT_SYS_ENABLE);

			if(perc_timer_handleOn != 0)
			{
				xTimerDelete(perc_timer_handleOn,0);
			}

			if(perc_timer_handleOff != 0)
			{
				xTimerDelete(perc_timer_handleOff,0);
			}
		}

		if(config.watering_state == PIVOT_DRY)
		{
			gpio_set_level(GPIO_ACT_PIN_WATERING, GPIO_ACT_SYS_DISABLE);
			gpio_actuator_pressure_off();
		}
		else if(config.watering_state == PIVOT_WET)
		{
			gpio_set_level(GPIO_ACT_PIN_WATERING, GPIO_ACT_SYS_ENABLE);
			gpio_actuator_pressure_on();
		}
	}
	else if(config.power_state == PIVOT_OFF)
	{
		gpio_actuator_shutdown();
	}

	err = ESP_OK;

	return err;
}

pivot_actions gpio_actuator_get(void)
{
	if(gpio_get_level(GPIO_ACT_PIN_CW_IN) == GPIO_ACT_SYS_ENABLE)
	{
		pivot_config_read.rotation = PIVOT_CW;
		pivot_config_read.power_state = PIVOT_ON;
	}
	else if(gpio_get_level(GPIO_ACT_PIN_CCW_IN) == GPIO_ACT_SYS_ENABLE)
	{
		pivot_config_read.rotation = PIVOT_CCW;
		pivot_config_read.power_state = PIVOT_ON;
	}
	else
	{
		pivot_config_read.power_state = PIVOT_OFF;
		pivot_config_read.rotation = PIVOT_UNKNOWN;
	}

	if(pressurizing == true)
	{
		pivot_config_read.watering_state = PIVOT_PRESSURIZING;
	}
	else if(gpio_get_level(GPIO_ACT_PIN_PRESS) == GPIO_ACT_SYS_ENABLE)
	{
		pivot_config_read.watering_state = PIVOT_WET;
	}
	else if(gpio_get_level(GPIO_ACT_PIN_PRESS) == GPIO_ACT_SYS_DISABLE)
	{
		pivot_config_read.watering_state = PIVOT_DRY;
	}

	if(((clock() - percent_watchdog)/CLOCKS_PER_SEC) > 70)
	{
		if(gpio_get_level(GPIO_ACT_PIN_PERC_IN) == GPIO_ACT_SYS_ENABLE)
		{
			pivot_config_read.percentimeter = 100;
		}
		else if(gpio_get_level(GPIO_ACT_PIN_PERC_IN) == GPIO_ACT_SYS_DISABLE)
		{
			pivot_config_read.percentimeter = 0;
		}
	}

	return pivot_config_read;
}

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
	gpio_actuator_pump_off();

	vTaskDelay(pdMS_TO_TICKS(GPIO_ACT_ONOFF_DELAY)); //TODO: set param as configurable

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

	pivot_config_read.percentimeter = 0;
}

void gpio_actuator_pump_on(void)
{
	gpio_set_level(GPIO_ACT_PIN_PUMP, GPIO_ACT_SYS_ENABLE);
}

void gpio_actuator_pump_off(void)
{
	gpio_set_level(GPIO_ACT_PIN_PUMP, GPIO_ACT_SYS_DISABLE);
}

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
void vPercTimerOnExpire(TimerHandle_t pxTimer)
{
	gpio_set_level(GPIO_ACT_PIN_PERC_OUT, GPIO_ACT_SYS_DISABLE);
	xTimerStart(perc_timer_handleOff, 1000);
}

void vPercTimerOffExpire(TimerHandle_t pxTimer)
{
	gpio_set_level(GPIO_ACT_PIN_PERC_OUT, GPIO_ACT_SYS_ENABLE);
	xTimerStart(perc_timer_handleOn, 1000);
}

esp_err_t gpio_actuator_start(void)
{
	esp_err_t err = ESP_FAIL;

	gpio_set_level(GPIO_ACT_PIN_ON, GPIO_ACT_SYS_ENABLE);
	vTaskDelay(pdMS_TO_TICKS(GPIO_ACT_ONOFF_DELAY));
	gpio_set_level(GPIO_ACT_PIN_ON, GPIO_ACT_SYS_DISABLE);

	return err;
}

void actuator_wait_pressure(void* arg)
{
	TickType_t check_start = xTaskGetTickCount();

	pressurizing = true;

	while(1)
	{
		LOG_ACTUATION(GPIO_ACT_TAG,"%s, Result: %lud",__func__, pdTICKS_TO_MS(xTaskGetTickCount() - check_start));
		if(gpio_get_level(GPIO_ACT_PIN_PRESS) == GPIO_ACT_SYS_ENABLE)
		{
			//system on
			gpio_actuator_start();
			pressurizing = false;

			//suspend own task
			vTaskSuspend(NULL);

			//task resume
			pressurizing = true;
			check_start = xTaskGetTickCount();
		}
		else if((pdTICKS_TO_MS(xTaskGetTickCount() - check_start)) > GPIO_ACT_PRESSURE_TIMEOUT)//TODO: set pressure timeout as configurable
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

			vTaskDelay(pdMS_TO_TICKS(GPIO_ACT_ONOFF_DELAY)); //TODO: set param as configurable

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

			pivot_config_read.percentimeter = 0;
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

void actuator_read_percent(void* arg)
{
	while(1)
	{
		//suspend own task
		vTaskSuspend(NULL);

		if(gpio_get_level(GPIO_ACT_PIN_PERC_IN) == GPIO_ACT_SYS_ENABLE)
		{
			posedge_perc = clock();
		}

		if(gpio_get_level(GPIO_ACT_PIN_PERC_IN) == GPIO_ACT_SYS_DISABLE)
		{
			negedge_perc = clock();

			if(posedge_perc != 0 && negedge_perc != 0)
			{
				perc_diff_onoff = (negedge_perc - posedge_perc);
				if (perc_diff_onoff != 0)
				{
					perc_sec_on = perc_diff_onoff / CLOCKS_PER_SEC;
					perc_pct_on = (perc_sec_on * 100) / (GPIO_ACT_PERC_FULL_CYCLE / 1000);
					pivot_config_read.percentimeter = perc_pct_on;
				}
			}
		}

		percent_watchdog = clock();

		vTaskDelay(pdMS_TO_TICKS(200));
	}
}

/**@}*/ 	//actuator_app
/** @}*/	//components
