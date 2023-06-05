/*
 * rf_module.h
 *
 *  Created on: 21 de jun. de 2022
 *      Author: brunolima
 */

#ifndef COMPONENTS_RF_MODULE_INCLUDE_RF_MODULE_H_
#define COMPONENTS_RF_MODULE_INCLUDE_RF_MODULE_H_

/**
 * @file common_parser.h
 * @date June 21, 2022
 * @brief controls the flow of reception and transmission of the RF module
*/

#include "project_config.h"

/**
 * @brief	start the RF module
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail
 */
esp_err_t rf_module_init(void);

/**
 * @brief	convert the structure to string and send it through the UART
 * @param 	config_in[in]:  configuration structure to send
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail
 */
esp_err_t rf_module_send_event(pivot_actions config_in);

/**
 * @brief	get the angle in degrees
 * @return	angle: returns the current angle (degrees) received
 */
uint16_t rf_module_get_angle(void);

/* Public Callback  ---------------------------------------------- */
/**
 * @brief	trigger callback to each status message received (write and read)
 * @param 	config_in[in]:  configuration structure
 */
void RF_MODULE_NOTIFY_APP(const void* notify_buffer);

#endif /* COMPONENTS_RF_MODULE_INCLUDE_RF_MODULE_H_ */
