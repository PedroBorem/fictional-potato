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
#include "esp_http_server.h"

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
 * @brief	Method to start HTTP server.
 * @return
 * 	- ESP_OK: Success.
 * 	- ESP_FAIL: Failed to start file server.
 * 	- ESP_ERR_INVALID_STATE - File server already started.
 * 	- ESP_ERR_NO_MEM - Failed to allocate memory for server data.
 */
httpd_handle_t http_server_start(void);

/**
 * @brief	Method to stop HTTP server.
 * @return
 *  - ESP_OK : Server stopped successfully.
 *  - ESP_ERR_INVALID_ARG : Handle argument is Null.
 *  - ESP_ERR_INVALID_STATE : HTTP server not started.
 */
esp_err_t http_server_stop(httpd_handle_t http_handle);

/**
 * @brief	Register the HTTP event callback.
 * @param	callback[in] - event callback.
 * @return
 *  - ESP_OK : callback registered successfully.
 *  - ESP_FAIL : invalid null pointer argument.
 */
esp_err_t http_server_register_callback(app_callback callback);


void http_server_send_resp(char* package);


#endif /* COMPONENTS_HTTP_SERVER_INCLUDE_HTTP_API_H_ */
