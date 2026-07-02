/**
 * @file i2cdev.h
 * @brief I2C device class for handling I2C communication.
 *
 * This header file defines the I2C device class and its associated functions for handling I2C communication.
 */

#ifndef COMPONENTS_RTC_INCLUDE_RTC_I2C_DEV_H_
#define COMPONENTS_RTC_INCLUDE_RTC_I2C_DEV_H_

#include "driver/i2c_master.h"

/**
 * @def I2C_FREQ_HZ
 * @brief The I2C bus frequency in Hz.
 */
#define I2C_FREQ_HZ 400000

/**
 * @def I2CDEV_TIMEOUT
 * @brief The timeout value for I2C communication in milliseconds.
 */
#define I2CDEV_TIMEOUT 1000

/**
 * @struct i2c_dev_t
 * @brief Structure representing an I2C device.
 */
typedef struct {
    i2c_port_num_t port;                    // I2C port number
    uint8_t addr;                           // I2C address
    gpio_num_t sda_io_num;                  // GPIO number for I2C sda signal
    gpio_num_t scl_io_num;                  // GPIO number for I2C scl signal
    uint32_t clk_speed;                     // I2C clock frequency for master mode
    i2c_master_bus_handle_t bus_handle;     // I2C master bus handle
    i2c_master_dev_handle_t dev_handle;     // I2C master device handle
} rtc_i2c_dev_t;

/**
 * @brief Initializes the I2C master bus and device handle.
 *
 * This function initializes the I2C master bus and registers the device
 * described by the given RTC I2C device structure.
 *
 * @param dev The pointer to the I2C device structure.
 * @return esp_err_t An error code indicating the success or failure of the initialization.
 */
esp_err_t i2c_master_init(rtc_i2c_dev_t *dev);

/**
 * @brief Reads data from the I2C device.
 *
 * This function reads data from the I2C device specified by the given device structure.
 *
 * @param dev The pointer to the I2C device structure.
 * @param out_data The buffer containing the data to be written before the read operation.
 * @param out_size The size of the output data buffer.
 * @param in_data The buffer to store the read data.
 * @param in_size The size of the input data buffer.
 * @return esp_err_t An error code indicating the success or failure of the operation.
 */
esp_err_t i2c_dev_read(const rtc_i2c_dev_t *dev, const void *out_data, size_t out_size, void *in_data, size_t in_size);

/**
 * @brief Writes data to the I2C device.
 *
 * This function writes data to the I2C device specified by the given device structure.
 *
 * @param dev The pointer to the I2C device structure.
 * @param out_reg The buffer containing the register address before the write operation.
 * @param out_reg_size The size of the register address buffer.
 * @param out_data The buffer containing the data to be written.
 * @param out_size The size of the output data buffer.
 * @return esp_err_t An error code indicating the success or failure of the operation.
 */
esp_err_t i2c_dev_write(const rtc_i2c_dev_t *dev, const void *out_reg, size_t out_reg_size, const void *out_data, size_t out_size);

/**
 * @brief Reads data from the I2C device register.
 *
 * This function reads data from the specified register of the I2C device.
 *
 * @param dev The pointer to the I2C device structure.
 * @param reg The register address.
 * @param in_data The buffer to store the read data.
 * @param in_size The size of the input data buffer.
 * @return esp_err_t An error code indicating the success or failure of the operation.
 */
inline esp_err_t i2c_dev_read_reg(const rtc_i2c_dev_t *dev, uint8_t reg, void *in_data, size_t in_size)
{
    return i2c_dev_read(dev, &reg, 1, in_data, in_size);
}

/**
 * @brief Writes data to the I2C device register.
 *
 * This function writes data to the specified register of the I2C device.
 *
 * @param dev The pointer to the I2C device structure.
 * @param reg The register address.
 * @param out_data The buffer containing the data to be written.
 * @param out_size The size of the output data buffer.
 * @return esp_err_t An error code indicating the success or failure of the operation.
 */
inline esp_err_t i2c_dev_write_reg(const rtc_i2c_dev_t *dev, uint8_t reg, const void *out_data, size_t out_size)
{
    return i2c_dev_write(dev, &reg, 1, out_data, out_size);
}

#endif /* COMPONENTS_RTC_INCLUDE_RTC_I2C_DEV_H_ */
