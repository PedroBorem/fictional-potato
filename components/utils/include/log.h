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

#define DBG_MQTT_ERROR

/**
 * @def LOG_COLOR_WHITE
 * @brief ANSI color code for white.
 *
 * This define specifies the ANSI color code for white, which can be used to customize log colors.
 */
#ifndef LOG_COLOR_WHITE
#define LOG_COLOR_WHITE   "37"
#endif

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


/**
 * @def LOG_DBF_ERROR
 * @brief Debug error logs.
 *
 * This macro is used to create debug error logs with a customizable tag and message.
 *
 * @param tag The tag for the log message.
 * @param msg The message for the log message.
 */
#ifdef DBG_MQTT_ERROR

#include "esp_err.h"
#include "project_config.h"

esp_err_t gprs_uart_send_event(const char* event, size_t event_size) __attribute__((weak));

#define LOG_DBG_ERROR(tag, msg) \
    do { \
        char pacote[200]; \
        snprintf(pacote, sizeof(pacote), "#99-%s-%s-%s$",system_id, tag, msg); \
        gprs_uart_send_event(pacote, strlen(pacote)); \
    } while(0)

#else
#define LOG_DBG_ERROR(tag, msg) \
    do { \
    } while(0)
#endif /* DBG_MQTT_ERROR */

#endif /* COMPONENTS_UTILS_INCLUDE_LOG_H_ */
