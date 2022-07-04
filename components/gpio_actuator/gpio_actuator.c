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

/* GPIO include */
#include "driver/gpio.h"

/**\addtogroup components
 * @{
 *
 */

/**\addtogroup gpio_actuator
 * @{
 *
 */

/* Global variables ---------------------------------------------- */
xTimerHandle perc_timer_handleOn;
xTimerHandle perc_timer_handleOff;

/* Public methods ------------------------------------------------ */
esp_err_t gpio_actuator_init()
{
	esp_err_t err = ESP_FAIL;

	//output configuration
	gpio_config_t io_conf_out = {};
	io_conf_out.intr_type = GPIO_INTR_DISABLE;
	io_conf_out.mode = GPIO_MODE_OUTPUT;
	io_conf_out.pin_bit_mask = GPIO_OUTPUT_PIN_GROUP;
	io_conf_out.pull_down_en = 0;
	io_conf_out.pull_up_en = 0;
    gpio_config(&io_conf_out);

    //input configuration
	gpio_config_t io_conf_in = {};
	io_conf_in.intr_type = GPIO_INTR_DISABLE;
	io_conf_in.mode = GPIO_MODE_INPUT;
	io_conf_in.pin_bit_mask = GPIO_INPUT_PIN_GROUP;
	io_conf_in.pull_down_en = 0;
	io_conf_in.pull_up_en = 1;
    gpio_config(&io_conf_in);



    err = ESP_OK;

	return err;
}

esp_err_t gpio_actuator_set(pivot_config config)
{
	esp_err_t err = ESP_FAIL;
	int perc_sec;

	perc_sec = config.percentimeter*(PERC_FULL_CYCLE/100)*1000;

	perc_timer_handleOn = xTimerCreate(
		      "percTimerON", /* name */
		      pdMS_TO_TICKS(perc_sec), /* period/time */
		      pdFALSE, /* auto reload */
		      (void*)0, /* timer ID */
			  vPercTimerOnExpire); /* callback */

	perc_timer_handleOff = xTimerCreate(
		      "percTimerOff", /* name */
		      pdMS_TO_TICKS((PERC_FULL_CYCLE-perc_sec)), /* period/time */
		      pdFALSE, /* auto reload */
		      (void*)0, /* timer ID */
			  vPercTimerOffExpire); /* callback */

	if(config.power_state == PIVOT_OFF)
	{
		gpio_actuator_shutdown();
		err = ESP_OK;
		return err;
	}
	else if(config.power_state == PIVOT_ON)
	{
		gpio_set_level(PIN_ON, SYS_ENABLE);
		gpio_set_level(PIN_AUX, SYS_ENABLE);
		if(config.rotation == PIVOT_CW)
		{
			gpio_set_level(PIN_CW, SYS_ENABLE);
			gpio_set_level(PIN_CCW, SYS_DISABLE);
		}
		else if(config.rotation == PIVOT_CCW)
		{
			gpio_set_level(PIN_CW, SYS_DISABLE);
			gpio_set_level(PIN_CCW, SYS_ENABLE);
		}
		gpio_set_level(PIN_ON, SYS_DISABLE);

		if(config.percentimeter > 0 && config.percentimeter < 100)
		{
			gpio_set_level(PIN_PERC_AUX, SYS_ENABLE);
			gpio_set_level(PIN_PERC_OUT, SYS_ENABLE);
			xTimerReset(perc_timer_handleOn, 0);
		}
		else if(config.percentimeter == 0)
		{
			gpio_set_level(PIN_PERC_OUT, SYS_DISABLE);
			gpio_set_level(PIN_PERC_AUX, SYS_ENABLE);
		}
		else if(config.percentimeter == 100)
		{
			gpio_set_level(PIN_PERC_OUT, SYS_ENABLE);
			gpio_set_level(PIN_PERC_AUX, SYS_ENABLE);
		}
	}

	return err;
}

void gpio_actuator_shutdown(void)
{

	gpio_set_level(PIN_OFF, SYS_ENABLE);
	gpio_set_level(PIN_AUX, SYS_DISABLE);
	gpio_set_level(PIN_CW, SYS_DISABLE);
	gpio_set_level(PIN_CCW, SYS_DISABLE);
	gpio_set_level(PIN_WATERING, SYS_DISABLE);
	gpio_set_level(PIN_PERC_AUX, SYS_DISABLE);
	gpio_set_level(PIN_PERC_OUT, SYS_DISABLE);
	//delay
	gpio_set_level(PIN_OFF, SYS_DISABLE);
}

void vPercTimerOnExpire(xTimerHandle pxTimer) {
	gpio_set_level(PIN_PERC_OUT, SYS_DISABLE);
	xTimerReset(perc_timer_handleOff, 0);
}

void vPercTimerOffExpire(xTimerHandle pxTimer) {
	gpio_set_level(PIN_PERC_OUT, SYS_ENABLE);
	xTimerReset(perc_timer_handleOn, 0);
}
