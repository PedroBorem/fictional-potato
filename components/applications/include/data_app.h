/*
 * data_app.h
 *
 *  Created on: 18 de set de 2022
 *      Author: bruno
 */

#ifndef COMPONENTS_APPLICATIONS_INCLUDE_DATA_APP_H_
#define COMPONENTS_APPLICATIONS_INCLUDE_DATA_APP_H_

/**
 * @file data_app.h
 * @date June 15, 2022
 * @brief Memory control application.
*/

#include "esp_err.h"

/**
 * @brief Enumeration defining various data types used in the application.
 * @enum data_type_t
 * 
 * This enumeration represents the different types of data that can be managed
 * by the data application. Each value corresponds to a specific data type.
 */
typedef enum
{
    DATA_TYPE_ACTIONS = 0, /**< Actions data type. */
    DATA_TYPE_PIVOT_CONFIG, /**< Pivot configuration data type. */
    DATA_TYPE_NETWORK_CONFIG, /**< Network configuration data type. */
    DATA_TYPE_ECO_MODE_CONFIG, /**< Eco mode configuration data type. */
    DATA_TYPE_SECTOR_CONFIG, /**< Sector configuration data type. */
    DATA_TYPE_GPS_CONFIG, /**< GPS configuration data type. */
    DATA_TYPE_REBOOT_CONFIG, /**< Reboot configuration data type. */
    DATA_TYPE_SCHEDULING_DATE, /**< Scheduling date data type. */
    DATA_TYPE_SCHEDULING_OFF_DATE, /**< Scheduling off date data type. */
    DATA_TYPE_SCHEDULING_ANGLE, /**< Scheduling angle data type. */
    DATA_TYPE_SCHEDULING_OFF_ANGLE, /**< Scheduling off angle data type. */
    DATA_TYPE_HISTORY, /**< History data type. */
    DATA_TYPE_OLD_HISTORY, /**< Old history data type. */
    DATA_TYPE_TIMESTAMP, /**< Timestamp data type. */
    DATA_TYPE_BARRIER_STATUS,  /**< Barrier status data. */
    DATA_TYPE_VIRTUAL_BARRIER, /**< Virtual Barrier configuration */
    DATA_TYPE_PHYSICAL_BARRIER, /**< Physical Barrier configuration */
    DATA_TYPE_INITIAL_ANGLE, /**< Initial angle data type. */
    DATA_TYPE_MANUAL_COUNTER, /**< Manual counter */
    DATA_TYPE_RAINFALL_ACCUMULATED, /**< Rainfall accumulated data type. */
    DATA_TYPE_RAIN_PER_PULSE, /**< Rain per pulse configuration. */
    DATA_TYPE_REASON_HANG_UP, /**<Reason why the pivot turned off  */
    DATA_TYPE_COMM_MAIN_MODE, /**< Communication Principal Mode data type. */
    DATA_TYPE_RAIN_SHUTDOWN_VALUE, /**< Rain needed to turn off the pivot */
    DATA_TYPE_RAINFALL_DAILY, /**< Daily rainfall data type. */
} data_type_t;


/**
 * @brief Initializes the data application.
 * @return esp_err_t: Error code indicating the success of the initialization.
 * 
 * This function initializes the data application for managing various types of data.
 */
esp_err_t data_app_init(void);

/**
 * @brief Saves data of a specific type.
 * @param data_type [in]: Type of data to save.
 * @param data [in]: Pointer to the data.
 * @param data_size [in]: Size of the data in bytes.
 * @return esp_err_t: Error code indicating the success of the save operation.
 * 
 * This function saves data of a specific type to the underlying storage.
 */
esp_err_t data_app_save(data_type_t data_type, const void* data, size_t data_size);

/**
 * @brief Loads data of a specific type.
 * @param data_type [in]: Type of data to load.
 * @param data [out]: Pointer to the buffer for loading data.
 * @return esp_err_t: Error code indicating the success of the load operation.
 * 
 * This function loads data of a specific type from the underlying storage.
 */
esp_err_t data_app_load(data_type_t data_type, void* data);

/**
 * @brief Deletes scheduling data based on the scheduling ID.
 * @param scheduling_id [in]: Scheduling ID to delete.
 * @return esp_err_t: Error code indicating the success of the delete operation.
 * 
 * This function deletes scheduling data based on the scheduling ID.
 */
esp_err_t data_app_delete_scheduling(char* scheduling_id);

/**
 * @brief Generates a scheduling key based on the current timestamp.
 * @param scheduling_id [out]: Buffer to store the generated scheduling ID.
 * 
 * This function generates a scheduling key based on the current timestamp.
 */
void data_app_gen_scheduling_key(char* scheduling_id);

/**
 * @brief Gets the size of data associated with a given label name.
 * @param label_name [in]: Label name for which to get the data size.
 * @return size_t: Size of the data associated with the label name.
 * 
 * This function gets the size of data associated with a given label name.
 */
size_t data_app_get_data_size(const char* label_name);

#endif /* COMPONENTS_APPLICATIONS_INCLUDE_DATA_APP_H_ */
