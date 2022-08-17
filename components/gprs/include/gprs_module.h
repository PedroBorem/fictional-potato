/*
 * gprs_module.h
 *
 *  Created on: 7 de ago de 2022
 *      Author: bruno
 */

#ifndef COMPONENTS_GPRS_INCLUDE_GPRS_MODULE_H_
#define COMPONENTS_GPRS_INCLUDE_GPRS_MODULE_H_

/**
 * @file common_parser.h
 * @date June 21, 2022
 * @brief controls the flow of reception and transmission of the gprs module
*/

#include "project_config.h"

/**
 * @brief	start the gprs module
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail
 */
esp_err_t gprs_module_init(void);

/**
 * @brief	convert the structure to string and send it through the UART
 * @param 	config_in[in]:  configuration structure to send
 * @return
 * 	- ESP_OK: success
 * 	- ESP_FAIL: fail
 */
esp_err_t gprs_module_send_event(pivot_config config_in, uint16_t degree);

/**
 * @brief	get the timestamp in seconds
 * @return	timestamp: returns the timestamp obtained via gprs
 */
time_t gprs_module_get_timestamp(void);

/* Public Callback  ---------------------------------------------- */
/**
 * @brief	trigger callback to each status message received (write and read)
 * @param 	config_in[in]:  configuration structure
 */
void GPRS_MODULE_NOTIFY_APP(const pivot_config config_in);

#endif /* COMPONENTS_GPRS_INCLUDE_GPRS_MODULE_H_ */
