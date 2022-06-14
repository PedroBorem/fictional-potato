/*
 * log.h
 *
 *  Created on: 14 de jun. de 2022
 *      Author: brunolima
 */

#ifndef COMPONENTS_UTILS_INCLUDE_LOG_H_
#define COMPONENTS_UTILS_INCLUDE_LOG_H_

/* Components include */
#include <inttypes.h>
#include "esp_log.h"

#define LOG_COLOR_WHITE   "37"

/* Communication logs */
#define LOG_COMM(tag,...) ESP_LOGI(LOG_COLOR(LOG_COLOR_CYAN) tag, __VA_ARGS__)

/* Data logs */
#define LOG_DATA(tag,...) ESP_LOGI(LOG_COLOR(LOG_COLOR_WHITE) tag, __VA_ARGS__)

/* Actuation logs */
#define LOG_ACTUATION(tag,...) ESP_LOGI(LOG_COLOR(LOG_COLOR_PURPLE) tag, __VA_ARGS__)

#endif /* COMPONENTS_UTILS_INCLUDE_LOG_H_ */
