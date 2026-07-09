/**
 * @file system_monitoring.h
 * @brief Connectivity heartbeat monitoring over the GPRS UART transport.
 */

#ifndef MAIN_INCLUDE_SYSTEM_MONITORING_H_
#define MAIN_INCLUDE_SYSTEM_MONITORING_H_

#include "esp_err.h"
#include "project_config.h"

esp_err_t system_monitoring_init(const app_callback send_callback);
void system_monitoring_heartbeat_feed(const char *heartbeat_state);
bool system_monitoring_connectivity_alive(void);
void system_monitoring_shutdown(void);

#endif /* MAIN_INCLUDE_SYSTEM_MONITORING_H_ */
