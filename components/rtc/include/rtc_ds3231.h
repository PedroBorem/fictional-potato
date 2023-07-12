/**
 * @file rtc_ds3231.h
 * @brief RTC DS3231 class for interfacing with the DS3231 RTC module.
 *
 * This header file defines the RTC DS3231 class and its associated functions for interfacing with the DS3231 RTC module.
 */

#ifndef COMPONENTS_RTC_INCLUDE_RTC_DS3231_H_
#define COMPONENTS_RTC_INCLUDE_RTC_DS3231_H_

#include <time.h>
#include <stdbool.h>

#include "driver/i2c.h"
#include "rtc_i2c_dev.h"

/**
 * @def DS3231_ADDR
 * @brief The I2C address of the DS3231 RTC module.
 */
#define DS3231_ADDR 0x68

/**
 * @def DS3231_STAT_OSCILLATOR
 * @brief Status bit indicating the oscillator stop flag in the DS3231.
 */
#define DS3231_STAT_OSCILLATOR 0x80

/**
 * @def DS3231_STAT_32KHZ
 * @brief Status bit indicating the 32kHz output flag in the DS3231.
 */
#define DS3231_STAT_32KHZ 0x08

/**
 * @def DS3231_STAT_BUSY
 * @brief Status bit indicating the busy flag in the DS3231.
 */
#define DS3231_STAT_BUSY 0x04

/**
 * @def DS3231_STAT_ALARM_2
 * @brief Status bit indicating the alarm 2 flag in the DS3231.
 */
#define DS3231_STAT_ALARM_2 0x02

/**
 * @def DS3231_STAT_ALARM_1
 * @brief Status bit indicating the alarm 1 flag in the DS3231.
 */
#define DS3231_STAT_ALARM_1 0x01

/**
 * @def DS3231_CTRL_OSCILLATOR
 * @brief Control bit for enabling/disabling the oscillator in the DS3231.
 */
#define DS3231_CTRL_OSCILLATOR 0x80

/**
 * @def DS3231_CTRL_SQUAREWAVE_BB
 * @brief Control bit for enabling/disabling the square wave output in the DS3231.
 */
#define DS3231_CTRL_SQUAREWAVE_BB 0x40

/**
 * @def DS3231_CTRL_TEMPCONV
 * @brief Control bit for initiating temperature conversion in the DS3231.
 */
#define DS3231_CTRL_TEMPCONV 0x20

/**
 * @def DS3231_CTRL_ALARM_INTS
 * @brief Control bit for enabling/disabling alarm interrupts in the DS3231.
 */
#define DS3231_CTRL_ALARM_INTS 0x04

/**
 * @def DS3231_CTRL_ALARM2_INT
 * @brief Control bit for enabling/disabling alarm 2 interrupt in the DS3231.
 */
#define DS3231_CTRL_ALARM2_INT 0x02

/**
 * @def DS3231_CTRL_ALARM1_INT
 * @brief Control bit for enabling/disabling alarm 1 interrupt in the DS3231.
 */
#define DS3231_CTRL_ALARM1_INT 0x01

/**
 * @def DS3231_ALARM_WDAY
 * @brief Flag indicating that the alarm is set to match both the date and weekday in the DS3231.
 */
#define DS3231_ALARM_WDAY 0x40

/**
 * @def DS3231_ALARM_NOTSET
 * @brief Flag indicating that the alarm is not set in the DS3231.
 */
#define DS3231_ALARM_NOTSET 0x80

/**
 * @def DS3231_ADDR_TIME
 * @brief The address of the time registers in the DS3231.
 */
#define DS3231_ADDR_TIME 0x00

/**
 * @def DS3231_ADDR_ALARM1
 * @brief The address of the alarm 1 registers in the DS3231.
 */
#define DS3231_ADDR_ALARM1 0x07

/**
 * @def DS3231_ADDR_ALARM2
 * @brief The address of the alarm 2 registers in the DS3231.
 */
#define DS3231_ADDR_ALARM2 0x0b

/**
 * @def DS3231_ADDR_CONTROL
 * @brief The address of the control register in the DS3231.
 */
#define DS3231_ADDR_CONTROL 0x0e

/**
 * @def DS3231_ADDR_STATUS
 * @brief The address of the status register in the DS3231.
 */
#define DS3231_ADDR_STATUS 0x0f

/**
 * @def DS3231_ADDR_AGING
 * @brief The address of the aging offset register in the DS3231.
 */
#define DS3231_ADDR_AGING 0x10

/**
 * @def DS3231_ADDR_TEMP
 * @brief The address of the temperature registers in the DS3231.
 */
#define DS3231_ADDR_TEMP 0x11

/**
 * @def DS3231_12HOUR_FLAG
 * @brief Flag indicating 12-hour mode in the DS3231.
 */
#define DS3231_12HOUR_FLAG 0x40

/**
 * @def DS3231_12HOUR_MASK
 * @brief Mask for extracting the hour value in 12-hour mode from the DS3231.
 */
#define DS3231_12HOUR_MASK 0x1f

/**
 * @def DS3231_PM_FLAG
 * @brief Flag indicating PM (post meridiem) in 12-hour mode in the DS3231.
 */
#define DS3231_PM_FLAG 0x20

/**
 * @def DS3231_MONTH_MASK
 * @brief Mask for extracting the month value from the DS3231.
 */
#define DS3231_MONTH_MASK 0x1f

/**
 * @brief Converts a binary-coded decimal (BCD) value to decimal.
 *
 * @param val The BCD value to convert.
 * @return uint8_t The decimal value.
 */
uint8_t bcd2dec(uint8_t val);

/**
 * @brief Converts a decimal value to binary-coded decimal (BCD).
 *
 * @param val The decimal value to convert.
 * @return uint8_t The BCD value.
 */
uint8_t dec2bcd(uint8_t val);

/**
 * @brief Initializes the DS3231 RTC descriptor.
 *
 * This function initializes the DS3231 RTC descriptor with the specified I2C port and GPIO pin numbers for SDA and SCL.
 *
 * @param dev The pointer to the RTC I2C device structure.
 * @param port The I2C port number.
 * @param sda_gpio The GPIO number for the I2C SDA signal.
 * @param scl_gpio The GPIO number for the I2C SCL signal.
 * @return esp_err_t An error code indicating the success or failure of the initialization.
 */
esp_err_t ds3231_init_desc(i2c_dev_t *dev, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio);

/**
 * @brief Sets the time on the DS3231 RTC module.
 *
 * This function sets the time on the DS3231 RTC module using the specified time structure.
 *
 * @param dev The pointer to the RTC I2C device structure.
 * @param time The pointer to the time structure containing the new time.
 * @return esp_err_t An error code indicating the success or failure of the operation.
 */
esp_err_t ds3231_set_time(i2c_dev_t *dev, struct tm *time);

/**
 * @brief Gets the raw temperature value from the DS3231 RTC module.
 *
 * This function retrieves the raw temperature value from the DS3231 RTC module.
 *
 * @param dev The pointer to the RTC I2C device structure.
 * @param temp The pointer to the variable to store the temperature value.
 * @return esp_err_t An error code indicating the success or failure of the operation.
 */
esp_err_t ds3231_get_raw_temp(i2c_dev_t *dev, int16_t *temp);

/**
 * @brief Gets the integer part of the temperature from the DS3231 RTC module.
 *
 * This function retrieves the integer part of the temperature value from the DS3231 RTC module.
 *
 * @param dev The pointer to the RTC I2C device structure.
 * @param temp The pointer to the variable to store the temperature value.
 * @return esp_err_t An error code indicating the success or failure of the operation.
 */
esp_err_t ds3231_get_temp_integer(i2c_dev_t *dev, int8_t *temp);

/**
 * @brief Gets the floating-point representation of the temperature from the DS3231 RTC module.
 *
 * This function retrieves the floating-point representation of the temperature value from the DS3231 RTC module.
 *
 * @param dev The pointer to the RTC I2C device structure.
 * @param temp The pointer to the variable to store the temperature value.
 * @return esp_err_t An error code indicating the success or failure of the operation.
 */
esp_err_t ds3231_get_temp_float(i2c_dev_t *dev, float *temp);

/**
 * @brief Gets the current time from the DS3231 RTC module.
 *
 * This function retrieves the current time from the DS3231 RTC module and stores it in the specified time structure.
 *
 * @param dev The pointer to the RTC I2C device structure.
 * @param time The pointer to the time structure to store the current time.
 * @return esp_err_t An error code indicating the success or failure of the operation.
 */
esp_err_t ds3231_get_time(i2c_dev_t *dev, struct tm *time);

#endif /* COMPONENTS_RTC_INCLUDE_RTC_DS3231_H_ */
