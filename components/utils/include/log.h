/**
 * @file log.h
 * @brief Centralized colored logging for the firmware.
 */

#ifndef COMPONENTS_UTILS_INCLUDE_LOG_H_
#define COMPONENTS_UTILS_INCLUDE_LOG_H_

#include <inttypes.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_log_timestamp.h"
#include "project_config.h"

#define DBG_MQTT_ERROR

#ifndef LOG_COLOR_WHITE
#define LOG_COLOR_WHITE "37"
#endif

#define PROJECT_LOG_COLOR_GRAY         "90"
#define PROJECT_LOG_COLOR_BRIGHT_GREEN "92"
#define PROJECT_LOG_COLOR_BRIGHT_CYAN  "96"

#define PROJECT_LOG_FORMAT(origin, format) \
    "[" origin "][%" PRIu32 " ms] " format

#define PROJECT_LOGI(color, tag, origin, format, ...) \
    ESP_LOGI(LOG_COLOR(color) tag, PROJECT_LOG_FORMAT(origin, format), esp_log_timestamp(), ##__VA_ARGS__)

#define PROJECT_LOGE(color, tag, origin, format, ...) \
    ESP_LOGE(LOG_COLOR(color) tag, PROJECT_LOG_FORMAT(origin, format), esp_log_timestamp(), ##__VA_ARGS__)

#define PROJECT_LOGW(color, tag, origin, format, ...) \
    ESP_LOGW(LOG_COLOR(color) tag, PROJECT_LOG_FORMAT(origin, format), esp_log_timestamp(), ##__VA_ARGS__)

#define PROJECT_LOGD(color, tag, origin, format, ...) \
    ESP_LOGD(LOG_COLOR(color) tag, PROJECT_LOG_FORMAT(origin, format), esp_log_timestamp(), ##__VA_ARGS__)

/* Severity */
#define LOG_ERROR(tag, origin, format, ...) \
    PROJECT_LOGE(LOG_COLOR_RED, tag, origin, format, ##__VA_ARGS__)
#define LOG_WARNING(tag, origin, format, ...) \
    PROJECT_LOGW(LOG_COLOR_YELLOW, tag, origin, format, ##__VA_ARGS__)

/* Operational categories */
#define LOG_DEFAULT(tag, origin, format, ...) \
    PROJECT_LOGI(LOG_COLOR_GREEN, tag, origin, format, ##__VA_ARGS__)
#define LOG_NVS(tag, format, ...) \
    PROJECT_LOGI(LOG_COLOR_WHITE, tag, "NVS", format, ##__VA_ARGS__)
#define LOG_TIMER(tag, format, ...) \
    PROJECT_LOGI(LOG_COLOR_CYAN, tag, "TIMER", format, ##__VA_ARGS__)
#define LOG_PROCESSING(tag, format, ...) \
    PROJECT_LOGI(LOG_COLOR_BLUE, tag, "PROCESSING", format, ##__VA_ARGS__)
#define LOG_UART_GPRS(tag, format, ...) \
    PROJECT_LOGI(LOG_COLOR_PURPLE, tag, "UART_GPRS", format, ##__VA_ARGS__)
#define LOG_UART_RF(tag, format, ...) \
    PROJECT_LOGI(PROJECT_LOG_COLOR_BRIGHT_CYAN, tag, "UART_RF", format, ##__VA_ARGS__)
#define LOG_SUCCESS(tag, origin, format, ...) \
    PROJECT_LOGI(PROJECT_LOG_COLOR_BRIGHT_GREEN, tag, origin, format, ##__VA_ARGS__)
#define LOG_DEBUG(tag, origin, format, ...) \
    PROJECT_LOGD(PROJECT_LOG_COLOR_GRAY, tag, origin, format, ##__VA_ARGS__)

/* Compatibility aliases for modules not migrated yet. */
#define LOG_COMM(tag, ...) LOG_PROCESSING(tag, __VA_ARGS__)
#define LOG_DATA(tag, ...) LOG_NVS(tag, __VA_ARGS__)
#define LOG_ACTUATION(tag, ...) LOG_PROCESSING(tag, __VA_ARGS__)
#define LOG_MANAGER(tag, ...) LOG_DEFAULT(tag, "SYSTEM", __VA_ARGS__)

esp_err_t gprs_uart_send_event(const char *event, size_t event_size) __attribute__((weak));

#ifdef DBG_MQTT_ERROR
#define LOG_DBG_ERROR(tag, msg) \
    do \
    { \
        char pacote[200]; \
        snprintf(pacote, sizeof(pacote), "#99-%s-%s-%s$", system_id, tag, msg); \
        gprs_uart_send_event(pacote, strlen(pacote)); \
    } while (0)
#else
#define LOG_DBG_ERROR(tag, msg) \
    do \
    { \
    } while (0)
#endif

#endif /* COMPONENTS_UTILS_INCLUDE_LOG_H_ */
