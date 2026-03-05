/**
 * @file rtc_i2c_dev.c
 * @brief RTC I2C device implementation.
 *
 * This file contains the implementation of the RTC I2C device.
 */

#include "rtc_i2c_dev.h"
#include <string.h>
#include <time.h>

#include "esp_log.h"

/**
 * @def RTC_I2C_DEV_TAG
 * @brief The RTC_I2C_DEV_TAG used for logging related to the RTC I2C device.
 */
#define RTC_I2C_DEV_TAG "I2CDEV"

/* Public methods ----------------------------------- */

/**
 * @brief Initialize the I2C master.
 *
 * This function initializes the I2C master with the specified parameters.
 *
 * @param port I2C port number.
 * @param sda GPIO number for SDA.
 * @param scl GPIO number for SCL.
 * @return ESP_OK on success, ESP_FAIL on failure.
 */
esp_err_t i2c_master_init(i2c_port_t port, int sda, int scl)
{
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda,
        .scl_io_num = scl,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 1000000
    };

    i2c_param_config(port, &i2c_config);
    return i2c_driver_install(port, I2C_MODE_MASTER, 0, 0, 0);
}

/**
 * @brief Read data from the I2C device.
 *
 * This function reads data from the I2C device.
 *
 * @param dev Pointer to the RTC I2C device structure.
 * @param out_data Data to be written before reading (can be NULL).
 * @param out_size Size of the data to be written.
 * @param in_data Pointer to the buffer for reading data.
 * @param in_size Size of the buffer for reading data.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if arguments are invalid, ESP_FAIL on failure.
 */
esp_err_t i2c_dev_read(const rtc_i2c_dev_t *dev, const void *out_data, size_t out_size, void *in_data, size_t in_size)
{
    if (!dev || !in_data || !in_size)
    {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (out_data && out_size)
    {
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write(cmd, (void *)out_data, out_size, true);
    }

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->addr << 1) | 1, true);
    i2c_master_read(cmd, in_data, in_size, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t res = i2c_master_cmd_begin(dev->port, cmd, I2CDEV_TIMEOUT / portTICK_PERIOD_MS);
    if (res != ESP_OK)
    {
        ESP_LOGE(RTC_I2C_DEV_TAG, "Could not read from device [0x%02x at %d]: %d", dev->addr, dev->port, res);
    }

    i2c_cmd_link_delete(cmd);

    return res;
}

/**
 * @brief Write data to the I2C device.
 *
 * This function writes data to the I2C device.
 *
 * @param dev Pointer to the RTC I2C device structure.
 * @param out_reg Register address to write to (can be NULL).
 * @param out_reg_size Size of the register address.
 * @param out_data Data to be written.
 * @param out_size Size of the data to be written.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if arguments are invalid, ESP_FAIL on failure.
 */
esp_err_t i2c_dev_write(const rtc_i2c_dev_t *dev, const void *out_reg, size_t out_reg_size, const void *out_data, size_t out_size)
{
    if (!dev || !out_data || !out_size)
    {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_WRITE, true);

    if (out_reg && out_reg_size)
    {
        i2c_master_write(cmd, (void *)out_reg, out_reg_size, true);
    }

    i2c_master_write(cmd, (void *)out_data, out_size, true);
    i2c_master_stop(cmd);
    esp_err_t res = i2c_master_cmd_begin(dev->port, cmd, I2CDEV_TIMEOUT / portTICK_PERIOD_MS);
    if (res != ESP_OK)
    {
        ESP_LOGE(RTC_I2C_DEV_TAG, "Could not write to device [0x%02x at %d]: %d", dev->addr, dev->port, res);
    }

    i2c_cmd_link_delete(cmd);
    return res;
}
