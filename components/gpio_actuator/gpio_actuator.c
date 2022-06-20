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

	//setar cada relé de acordo
	//criar task de timer do percentimetro

	return err;
}


