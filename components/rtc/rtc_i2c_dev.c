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
esp_err_t i2c_master_init(rtc_i2c_dev_t *dev)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (dev->dev_handle)
    {
        return ESP_OK;
    }

    if (!dev->bus_handle)
    {
        i2c_master_bus_config_t bus_config = {
            .i2c_port = dev->port,
            .sda_io_num = dev->sda_io_num,
            .scl_io_num = dev->scl_io_num,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
        };

        esp_err_t res = i2c_new_master_bus(&bus_config, &dev->bus_handle);
        if (res == ESP_ERR_INVALID_STATE)
        {
            res = i2c_master_get_bus_handle(dev->port, &dev->bus_handle);
        }

        if (res != ESP_OK)
        {
            ESP_LOGE(RTC_I2C_DEV_TAG, "Could not initialize I2C bus %d: %d", dev->port, res);
            return res;
        }
    }

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = dev->addr,
        .scl_speed_hz = dev->clk_speed,
    };

    esp_err_t res = i2c_master_bus_add_device(dev->bus_handle, &dev_config, &dev->dev_handle);
    if (res != ESP_OK)
    {
        ESP_LOGE(RTC_I2C_DEV_TAG, "Could not add I2C device [0x%02x at %d]: %d", dev->addr, dev->port, res);
    }

    return res;
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

    esp_err_t res = ESP_OK;
    if (out_data && out_size)
    {
        res = i2c_master_transmit_receive(dev->dev_handle, out_data, out_size, in_data, in_size, I2CDEV_TIMEOUT);
    }
    else
    {
        res = i2c_master_receive(dev->dev_handle, in_data, in_size, I2CDEV_TIMEOUT);
    }
    if (res != ESP_OK)
    {
        ESP_LOGE(RTC_I2C_DEV_TAG, "Could not read from device [0x%02x at %d]: %d", dev->addr, dev->port, res);
    }

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

    esp_err_t res = ESP_OK;
    if (out_reg && out_reg_size)
    {
        i2c_master_transmit_multi_buffer_info_t buffers[] = {
            {
                .write_buffer = out_reg,
                .buffer_size = out_reg_size,
            },
            {
                .write_buffer = out_data,
                .buffer_size = out_size,
            },
        };

        res = i2c_master_multi_buffer_transmit(dev->dev_handle, buffers, 2, I2CDEV_TIMEOUT);
    }
    else
    {
        res = i2c_master_transmit(dev->dev_handle, out_data, out_size, I2CDEV_TIMEOUT);
    }

    if (res != ESP_OK)
    {
        ESP_LOGE(RTC_I2C_DEV_TAG, "Could not write to device [0x%02x at %d]: %d", dev->addr, dev->port, res);
    }

    return res;
}
