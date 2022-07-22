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
#define ACTUATOR_TAG			"actuator_main"

#define ESP_INTR_FLAG_DEFAULT 0
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
pivot_config config_in = {};
int last_edge;
int diff;

/* Public methods ------------------------------------------------ */
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
	if(gpio_get_level(PIN_PERC_IN) == SYS_ENABLE){
		posedge_perc = clock();
	}
	if(gpio_get_level(PIN_PERC_IN) == SYS_DISABLE){
		negedge_perc = clock();

		if(posedge_perc != 0 && negedge_perc != 0){
			diff = (negedge_perc - posedge_perc);
			if (diff != 0){
				perc_t_on = diff / CLOCKS_PER_SEC;
				config_in.percentimeter = perc_t_on;
			}
		}
	}
}

void vPercTimerOnExpire(xTimerHandle pxTimer) {
	gpio_set_level(PIN_PERC_OUT, SYS_DISABLE);
	xTimerStart(perc_timer_handleOff, 0);
	ESP_LOGE(ACTUATOR_TAG, "%s, Timer On expired", __func__);
}

void vPercTimerOffExpire(xTimerHandle pxTimer) {
	gpio_set_level(PIN_PERC_OUT, SYS_ENABLE);
	xTimerStart(perc_timer_handleOn, 0);
	ESP_LOGE(ACTUATOR_TAG, "%s, Timer Off expired", __func__);
}

esp_err_t gpio_actuator_init()
{
	esp_err_t err = ESP_FAIL;

	perc_t_off = 0;
	perc_t_on = 0;

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

    //reader initial setup
    config_in.power_state = PIVOT_UNKNOWN;
    config_in.rotation = PIVOT_UNKNOWN;
    config_in.watering_state = PIVOT_UNKNOWN;
    config_in.percentimeter = PIVOT_UNKNOWN;

    //percent reader interrupt setup
	gpio_config_t io_conf_int = {};
    io_conf_int.intr_type = GPIO_INTR_ANYEDGE;
    io_conf_int.pin_bit_mask = GPIO_INT_PERC;
    io_conf_int.mode = GPIO_MODE_INPUT;
    io_conf_int.pull_down_en = 0;
    io_conf_int.pull_up_en = 1;
    gpio_config(&io_conf_int);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(PIN_PERC_IN, gpio_isr_handler, (void*) PIN_PERC_IN);

    err = ESP_OK;

	return err;
}

esp_err_t gpio_actuator_set(pivot_config config)
{
	esp_err_t err = ESP_FAIL;
	int perc_sec;

	perc_sec = config.percentimeter*((PERC_FULL_CYCLE)/100);

	ESP_LOGE(ACTUATOR_TAG, "%s, Perc sec: %d", __func__, perc_sec);

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
			xTimerStart(perc_timer_handleOn, 0);
		}
		else if(config.percentimeter == 0)
		{
			gpio_set_level(PIN_PERC_OUT, SYS_DISABLE);
			gpio_set_level(PIN_PERC_AUX, SYS_DISABLE);

			//kill percent timers if they exist
			if(perc_timer_handleOn != 0)
			{
				xTimerDelete(perc_timer_handleOn,0);
			}
			if(perc_timer_handleOff != 0)
			{
				xTimerDelete(perc_timer_handleOff,0);
			}
		}
		else if(config.percentimeter == 100)
		{
			gpio_set_level(PIN_PERC_OUT, SYS_ENABLE);
			gpio_set_level(PIN_PERC_AUX, SYS_ENABLE);
		}

		if(config.watering_state == PIVOT_WET)
		{
			gpio_set_level(PIN_WATERING, SYS_ENABLE);
		}
		else if(config.watering_state == PIVOT_DRY)
		{
			gpio_set_level(PIN_WATERING, SYS_DISABLE);
		}

		gpio_set_level(PIN_ON, SYS_DISABLE);
	}

	return err;
}

pivot_config gpio_actuator_get(void)
{
	if(gpio_get_level(PIN_CW_IN) == SYS_ENABLE){
		config_in.rotation = PIVOT_CW;
		config_in.power_state = PIVOT_ON;
	}
	else if(gpio_get_level(PIN_CCW_IN) == SYS_ENABLE){
		config_in.rotation = PIVOT_CCW;
		config_in.power_state = PIVOT_ON;
	}else{
		config_in.power_state = PIVOT_OFF;
		config_in.rotation = PIVOT_OFF;
	}

	if(gpio_get_level(PIN_PRESS) == SYS_ENABLE){
		config_in.watering_state = PIVOT_WET;
	}else if(gpio_get_level(PIN_PRESS == SYS_DISABLE)){
		config_in.watering_state = PIVOT_DRY;
	}

	return config_in;
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

	if(perc_timer_handleOn != 0)
	{
		xTimerDelete(perc_timer_handleOn,0);
	}
	if(perc_timer_handleOff != 0)
	{
		xTimerDelete(perc_timer_handleOff,0);
	}
}
