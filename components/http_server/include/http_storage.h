/*
 * http_storage.h
 *
 *  Created on: 20 de ago de 2022
 *      Author: bruno
 */

#ifndef COMPONENTS_HTTP_SERVER_INCLUDE_HTTP_STORAGE_H_
#define COMPONENTS_HTTP_SERVER_INCLUDE_HTTP_STORAGE_H_

// configuration include
#include "project_config.h"

/**
 * @brief	Function to initialize SPIFFS.
 * @param	base_path[in] - memory base directory where the files are saved.
 * @return
 * 	- ESP_OK: Success.
 * 	- ESP_FAIL: Failed to mount or format filesystem.
 * 	- ESP_ERR_NOT_FOUND - Failed to initialize SPIFFS.
 */
esp_err_t http_server_mount_storage(const char* base_path);

#endif /* COMPONENTS_HTTP_SERVER_INCLUDE_HTTP_STORAGE_H_ */
