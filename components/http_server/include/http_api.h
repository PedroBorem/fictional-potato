/**
 * @file http_server.h
 * @brief HTTP Server API for handling server initialization, start, stop, and callbacks.
 *
 * This file provides functions to initialize, start, stop, and manage an HTTP server.
 */

#ifndef COMPONENTS_HTTP_SERVER_INCLUDE_HTTP_API_H_
#define COMPONENTS_HTTP_SERVER_INCLUDE_HTTP_API_H_

// Components include
#include "project_config.h"
#include "esp_http_server.h"

/**
 * @brief Initialize the HTTP server memory and variables.
 *
 * This function initializes the HTTP server memory and variables, including file system setup.
 *
 * @return
 * 	- ESP_OK: Success.
 * 	- ESP_FAIL: Failed to mount or format filesystem.
 * 	- ESP_ERR_NOT_FOUND - Failed to initialize SPIFFS.
 * 	- ESP_ERR_NO_MEM - Failed to allocate memory for server data.
 */
esp_err_t http_server_init(void);

/**
 * @brief Start the HTTP server.
 *
 * This function starts the HTTP server.
 *
 * @return
 * 	- ESP_OK: Success.
 * 	- ESP_FAIL: Failed to start file server.
 * 	- ESP_ERR_INVALID_STATE - File server already started.
 * 	- ESP_ERR_NO_MEM - Failed to allocate memory for server data.
 */
httpd_handle_t http_server_start(void);

/**
 * @brief Stop the HTTP server.
 *
 * This function stops the HTTP server.
 *
 * @param http_handle The handle to the HTTP server.
 * @return
 *  - ESP_OK: Server stopped successfully.
 *  - ESP_ERR_INVALID_ARG: Handle argument is Null.
 *  - ESP_ERR_INVALID_STATE: HTTP server not started.
 */
esp_err_t http_server_stop(httpd_handle_t http_handle);

/**
 * @brief Register the HTTP event callback.
 *
 * This function registers the HTTP event callback.
 *
 * @param callback The event callback function.
 * @return
 *  - ESP_OK: Callback registered successfully.
 *  - ESP_FAIL: Invalid null pointer argument.
 */
esp_err_t http_server_register_callback(app_callback callback);

/**
 * @brief Send a response through the HTTP server.
 *
 * This function sends a response through the HTTP server.
 *
 * @param package The response package to be sent.
 */
void http_server_send_resp(char* package);

#endif /* COMPONENTS_HTTP_SERVER_INCLUDE_HTTP_API_H_ */
