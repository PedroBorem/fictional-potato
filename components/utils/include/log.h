/**
 * @file log.h
 * @date June 15, 2022
 * @brief Creating customizable logs.
 *
 * This file contains macros for creating customizable logs.
 */

#ifndef COMPONENTS_UTILS_INCLUDE_LOG_H_
#define COMPONENTS_UTILS_INCLUDE_LOG_H_

/* Components include */
#include <inttypes.h>
#include "esp_log.h"

/**
 * @def LOG_COLOR_WHITE
 * @brief ANSI color code for white.
 *
 * This define specifies the ANSI color code for white, which can be used to customize log colors.
 */
#define LOG_COLOR_WHITE   "37"

/**
 * @def LOG_COMM
 * @brief Communication logs.
 *
 * This macro is used to create communication logs with a customizable tag and message.
 *
 * @param tag The tag for the log message.
 * @param ... The message or additional parameters for the log message.
 */
#define LOG_COMM(tag,...) ESP_LOGI(LOG_COLOR(LOG_COLOR_CYAN) tag, __VA_ARGS__)

/**
 * @def LOG_DATA
 * @brief Data logs.
 *
 * This macro is used to create data logs with a customizable tag and message.
 *
 * @param tag The tag for the log message.
 * @param ... The message or additional parameters for the log message.
 */
#define LOG_DATA(tag,...) ESP_LOGI(LOG_COLOR(LOG_COLOR_WHITE) tag, __VA_ARGS__)

/**
 * @def LOG_ACTUATION
 * @brief Actuation logs.
 *
 * This macro is used to create actuation logs with a customizable tag and message.
 *
 * @param tag The tag for the log message.
 * @param ... The message or additional parameters for the log message.
 */
#define LOG_ACTUATION(tag,...) ESP_LOGI(LOG_COLOR(LOG_COLOR_PURPLE) tag, __VA_ARGS__)

/**
 * @def LOG_MANAGER
 * @brief System manager logs.
 *
 * This macro is used to create actuation logs with a customizable tag and message.
 *
 * @param tag The tag for the log message.
 * @param ... The message or additional parameters for the log message.
 */
#define LOG_MANAGER(tag,...) ESP_LOGI(LOG_COLOR(LOG_COLOR_GREEN) tag, __VA_ARGS__)

#endif /* COMPONENTS_UTILS_INCLUDE_LOG_H_ */
