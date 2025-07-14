/**
 * @file v9881d_spi.h
 * @brief Header file for V9881D SPI communication functions.
 */

#ifndef COMPONENTS_AC_READER_INCLUDE_V9881D_SPI_H_
#define COMPONENTS_AC_READER_INCLUDE_V9881D_SPI_H_

#include "project_config.h"
#include "esp_err.h"

/**
 * @brief Initialize the SPI module.
 *
 * This function initializes the SPI module, setting up the SPI communication.
 *
 * @param callback The callback function to be called when data is received.
 *
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Fail to initialize
 *     - ESP_ERR_INVALID_ARG: Invalid callback function
 */
esp_err_t spi_init(const app_callback callback);

#endif /* COMPONENTS_AC_READER_INCLUDE_V9881D_SPI_H_ */
