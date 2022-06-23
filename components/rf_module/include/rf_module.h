/*
 * rf_module.h
 *
 *  Created on: 21 de jun. de 2022
 *      Author: brunolima
 */

#ifndef COMPONENTS_RF_MODULE_INCLUDE_RF_MODULE_H_
#define COMPONENTS_RF_MODULE_INCLUDE_RF_MODULE_H_

#include "project_config.h"

esp_err_t rf_module_init(void);

esp_err_t rf_module_send_event(pivot_config config_in);

uint16_t rf_module_get_angle(void);

time_t rf_module_get_timestamp(void);


// Callback
void RF_MODULO_NOTIFY_APP(pivot_config config);

#endif /* COMPONENTS_RF_MODULE_INCLUDE_RF_MODULE_H_ */
