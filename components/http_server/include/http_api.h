/*
 * http_server.h
 *
 *  Created on: 20 de Jan de 2023
 *      Author: bruno
 */

#ifndef COMPONENTS_HTTP_SERVER_INCLUDE_HTTP_API_H_
#define COMPONENTS_HTTP_SERVER_INCLUDE_HTTP_API_H_

// Components include
#include "project_config.h"
#include "http_config_parser.h"

/**
 * @brief	Method to start HTTP server.
 * @return
 * 	- ESP_OK: Success.
 * 	- ESP_FAIL: Failed to start file server.
 * 	- ESP_ERR_INVALID_STATE - File server already started.
 * 	- ESP_ERR_NO_MEM - Failed to allocate memory for server data.
 */
esp_err_t http_server_start(void);

/**
 * @brief	Method to init HTTP server memory and variables.
 * @return
 * 	- ESP_OK: Success.
 * 	- ESP_FAIL: Failed to mount or format filesystem.
 * 	- ESP_ERR_NOT_FOUND - Failed to initialize SPIFFS.
 * 	- ESP_ERR_NO_MEM - Failed to allocate memory for server data.
 */
esp_err_t http_server_init(void);

/**
 * @brief	Method to stop HTTP server.
 * @return
 *  - ESP_OK : Server stopped successfully.
 *  - ESP_ERR_INVALID_ARG : Handle argument is Null.
 *  - ESP_ERR_INVALID_STATE : HTTP server not started.
 */
esp_err_t http_server_stop(void);

/**
 * @brief	Register the HTTP event callback.
 * @param	callback[in] - event callback.
 * @return
 *  - ESP_OK : callback registered successfully.
 *  - ESP_FAIL : invalid null pointer argument.
 */
esp_err_t http_server_register_callback(app_callback callback);

/**
 * @brief	Set the configuration string sent when receiving a GET request
 * @param	str_config[in] - buffer in json format containing the settings
 * @return
 *  - ESP_OK : Set the configuration string successfully.
 *  - ESP_ERR_INVALID_ARG : invalid null pointer argument.
 *  - ESP_FAIL : Error converting string.
 */
esp_err_t http_server_set_str_action(pivot_actions current_action);

/**
 * @brief	Set the configuration string sent when receiving a GET request
 * @param	str_config[in] - buffer in json format containing the settings
 * @return
 *  - ESP_OK : Set the configuration string successfully.
 *  - ESP_ERR_INVALID_ARG : invalid null pointer argument.
 *  - ESP_FAIL : Error converting string.
 */
esp_err_t http_server_set_str_config(pivot_config current_config);

esp_err_t http_server_set_str_actions(const pivot_actions action, const pivot_config config, uint16_t start_angle, uint16_t end_angle);

#endif /* COMPONENTS_HTTP_SERVER_INCLUDE_HTTP_API_H_ */
