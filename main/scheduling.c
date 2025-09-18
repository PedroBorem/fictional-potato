/**
 * @file scheduling.c
 * @brief Implementation of the Scheduling module.
 * @author soil-dev
 * @date 31 de jul. de 2023
 */

#include "scheduling.h"
#include "FreeRTOS_defines.h"
#include "log.h"
#include "actuation_app.h"
#include "data_app.h"
#include "comm_app.h"
#include "rtc_app.h"
#include "idp_parser.h"
#include <string.h>
#include <stdbool.h>

#define SCHEDULING_TAG "scheduling"

/**
 * @brief Task handle for Scheduling IDP 14.
 */
static TaskHandle_t xTask_scheduling_idp_14 = NULL;

/**
 * @brief Task handle for Scheduling IDP 15.
 */
static TaskHandle_t xTask_scheduling_idp_15 = NULL;

/**
 * @brief Task handle for Scheduling IDP 16.
 */
static TaskHandle_t xTask_scheduling_idp_16 = NULL;

/**
 * @brief Task handle for Scheduling IDP 17.
 */
static TaskHandle_t xTask_scheduling_idp_17 = NULL;

/**
 * @brief Array to store current scheduling dates.
 */
static pivot_scheduling_date scheduling_date_current[CONFIG_SCHEDULING_MAX_VALUE] = {};

/**
 * @brief Array to store current scheduling off dates.
 */
static pivot_scheduling_off_date scheduling_off_date_current[CONFIG_SCHEDULING_MAX_VALUE] = {};

/**
 * @brief Array to store current scheduling angles.
 */
static pivot_scheduling_angle scheduling_angle_current[CONFIG_SCHEDULING_MAX_VALUE] = {};

/**
 * @brief Structure to store the current scheduling off angle.
 */
static pivot_scheduling_off_angle scheduling_off_angle_current[CONFIG_SCHEDULING_MAX_VALUE] = {};

/**
 * @brief Array to store the status of scheduling dates.
 */
static bool scheduling_date_status[CONFIG_SCHEDULING_MAX_VALUE] = {};

/**
 * @brief Array to store the status of scheduling angles.
 */
static bool scheduling_angle_status[CONFIG_SCHEDULING_MAX_VALUE] = {};

/**
 * @brief Pointer to the current angle.
 */
static uint16_t* scheduling_current_angle = &global_angle;

/**
 * @brief Callback function for scheduling events.
 */
static app_callback scheduling_callback = NULL;

/**
 * @brief Initializes the scheduling module.
 * @param callback The callback function for scheduling events.
 */
static hangs_up_callback scheduling_hang_up_call = NULL;

/**
 * @brief Activates the scheduling at the specified position.
 * @param position The position of the scheduling date or angle in the array.
 * @param scheduling_id The ID of the scheduling.
 * @param actions The pivot actions to be performed.
 */
static void scheduling_active(uint8_t position, char* scheduling_id, pivot_actions actions);

/**
 * @brief Deactivates the scheduling with the specified ID.
 * @param scheduling_id The ID of the scheduling to be deactivated.
 * @param shceduling_type The type of the scheduling (IDP)
 * @param scheduling_notify_server Flag to indicate whether to notify the server about deactivation.
 */
static void scheduling_deactivate(char* scheduling_id, bool scheduling_notify_server);

/**
 * @brief Task for processing scheduling events with IDP 14.
 * @param arg Task argument (not used).
 */
static void scheduling_task_idp_14(void* arg);

/**
 * @brief Task for processing scheduling events with IDP 15.
 * @param arg Task argument (not used).
 */
static void scheduling_task_idp_15(void* arg);

/**
 * @brief Task for processing scheduling events with IDP 16.
 * @param arg Task argument (not used).
 */
static void scheduling_task_idp_16(void* arg);

/**
 * @brief Task for processing scheduling events with IDP 17.
 * @param arg Task argument (not used).
 */
static void scheduling_task_idp_17(void* arg);

/**
 * @brief Activates the scheduling at the specified position.
 * @param position The position of the scheduling date or angle in the array.
 * @param scheduling_id The ID of the scheduling.
 * @param actions The pivot actions to be performed.
 */
static void scheduling_active(uint8_t position, char* scheduling_id, pivot_actions actions)
{
    uint8_t idp = IDP_INVALID;
    uint16_t dwp = 0;
    char str_out[50] = {};

    if (scheduling_callback == NULL)
    {
        ESP_LOGE(SCHEDULING_TAG, "invalid callback");
        return;
    }

    // create package - send IDP 18
    idp = IDP_18;
    arg_pair_t arg_idp_18[] = {
        { "uint8_t", &idp },
        { "string", SCHEDULING_TAG },
        { "string", scheduling_id},
        { NULL, NULL }
    };

    memset(str_out, 0x00, sizeof(str_out));
    idp_parser_create_package(str_out, arg_idp_18);
    scheduling_callback(str_out, COMM_MQTT);

    // act on the equipment - send IDP 01
    idp = IDP_1;
    dwp = idp_parser_create_pwd(actions);

    arg_pair_t arg_idp_01[] =
    {
        { "uint8_t", &idp },
        { "string", SCHEDULING_TAG },
        { "uint16_t", &dwp },
        { "uint16_t", &actions.percentimeter },
        { NULL, NULL }
    };

    memset(str_out, 0x00, sizeof(str_out));
    idp_parser_create_package(str_out, arg_idp_01);
    scheduling_callback(str_out, COMM_MQTT);

    // log debug
    rtc_app_get_timestamp(true);
    ESP_LOGW(SCHEDULING_TAG, "processing schedule id : %s (%s)",
            scheduling_id, __func__);
}

/**
 * @brief Deactivates the scheduling with the specified ID.
 * @param scheduling_id The ID of the scheduling to be deactivated.
 * @param scheduling_notify_server Flag to indicate whether to notify the server about deactivation.
 */
static void scheduling_deactivate(char* scheduling_id, bool scheduling_notify_server)
{
    uint8_t idp = IDP_INVALID;
    uint16_t dwp = 0;
    char str_out[50] = {};

    if (scheduling_callback == NULL)
    {
        ESP_LOGE(SCHEDULING_TAG, "invalid callback");
        return;
    }

    if (scheduling_notify_server == true)
    {
        // create package - send IDP 18
        idp = IDP_18;
        arg_pair_t arg_idp_18[] = {
            { "uint8_t", &idp },
            { "string", SCHEDULING_TAG },
            { "string", scheduling_id},
            { NULL, NULL }
        };

        memset(str_out, 0x00, sizeof(str_out));
        idp_parser_create_package(str_out, arg_idp_18);
        scheduling_callback(str_out, COMM_MQTT);
    }

    // act on the equipment - send IDP 01
    idp = IDP_1;
    dwp = idp_parser_create_pwd(pivot_actions_off);
    uint16_t percent = 0;

    arg_pair_t arg_idp_01[] =
    {
        { "uint8_t", &idp },
        { "string", SCHEDULING_TAG },
        { "uint16_t", &dwp },
        { "uint16_t", &percent },
        { NULL, NULL }
    };

    memset(str_out, 0x00, sizeof(str_out));
    idp_parser_create_package(str_out, arg_idp_01);
    scheduling_callback(str_out, COMM_MQTT);

    // delete SCHEDULING - send IDP 13
    idp = IDP_13;

    arg_pair_t arg_idp_13[] =
    {
        { "uint8_t", &idp },
        { "string", SCHEDULING_TAG },
        { "string", scheduling_id },
        { "string", SCHEDULING_TAG },
        { NULL, NULL }
    };

    memset(str_out, 0x00, sizeof(str_out));
    idp_parser_create_package(str_out, arg_idp_13);
    scheduling_callback(str_out, COMM_MQTT);

    // log debug
    rtc_app_get_timestamp(true);
    ESP_LOGW(SCHEDULING_TAG, "End schedule by date id : %s (%s)",
            scheduling_id, __func__);
}

/**
 * @brief Task for processing scheduling events with IDP 14.
 */
static void scheduling_task_idp_14(void* arg)
{
    time_t scheduling_timestamp_now = 0;
    memset(scheduling_date_status, false, sizeof(scheduling_date_status));

    while(1)
    {
        // get timestamp
        scheduling_timestamp_now = rtc_app_get_timestamp(false);


        // date analysis
        for (uint8_t date_position = 0; date_position < CONFIG_SCHEDULING_MAX_VALUE; date_position++)
        {
            if (scheduling_timestamp_now > scheduling_date_current[date_position].start_date
                && scheduling_timestamp_now < scheduling_date_current[date_position].end_date
                && strcmp(scheduling_date_current[date_position].scheduling_id, "") > 0)
            {
                if (scheduling_date_status[date_position] == false)
                {
                    scheduling_date_status[date_position] = true;
                    scheduling_active(date_position,
                                      scheduling_date_current[date_position].scheduling_id,
                                      scheduling_date_current[date_position].actions);
                }
            }
            else if (scheduling_date_status[date_position] == true)
            {
                scheduling_hang_up_call(TYPE_HANGS_UP_SCHEDULE_14, IDP_14, scheduling_date_current[date_position].scheduling_id, scheduling_date_current[date_position].str_author);
                scheduling_date_status[date_position] = false;
                scheduling_deactivate(scheduling_date_current[date_position].scheduling_id, false);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // 5 seconds
    }
}

/**
 * @brief Task for processing scheduling events with IDP 15.
 */
static void scheduling_task_idp_15(void* arg)
{
    const uint16_t angle_off_set = 3;
    time_t scheduling_timestamp_now = 0;

    memset(scheduling_angle_status, false, sizeof(scheduling_angle_status));

    while (1)
    {
        // get timestamp
        scheduling_timestamp_now = rtc_app_get_timestamp(false);

        // angle analysis
        for (uint8_t angle_position = 0; angle_position < CONFIG_SCHEDULING_MAX_VALUE; angle_position++)
        {
            if (scheduling_timestamp_now > scheduling_angle_current[angle_position].start_date
                && strcmp(scheduling_angle_current[angle_position].scheduling_id, "") > 0)
            {
                if (scheduling_angle_status[angle_position] == false)
                {
                    scheduling_angle_status[angle_position] = true;
                    scheduling_active(angle_position,
                                      scheduling_angle_current[angle_position].scheduling_id,
                                      scheduling_angle_current[angle_position].actions);

                    if (*scheduling_current_angle >= scheduling_angle_current[angle_position].end_angle - angle_off_set
                        && *scheduling_current_angle <= scheduling_angle_current[angle_position].end_angle + angle_off_set)
                    {
                        vTaskDelay(pdMS_TO_TICKS(300000)); // 5 minutes
                    }
                }
                else if (*scheduling_current_angle >= scheduling_angle_current[angle_position].end_angle - angle_off_set
                         && *scheduling_current_angle <= scheduling_angle_current[angle_position].end_angle + angle_off_set)
                {
                    scheduling_hang_up_call(TYPE_HANGS_UP_SCHEDULE_15, IDP_15, scheduling_angle_current[angle_position].scheduling_id, scheduling_angle_current[angle_position].str_author);
                    scheduling_angle_status[angle_position] = false;
                    scheduling_deactivate(scheduling_angle_current[angle_position].scheduling_id, false);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // 5 seconds
    }
}

/**
 * @brief Task for processing scheduling events with IDP 16.
 */
static void scheduling_task_idp_16(void* arg)
{
    time_t scheduling_timestamp_now = 0;

    while (1)
    {
        // get timestamp
        scheduling_timestamp_now = rtc_app_get_timestamp(false);

        // angle analysis
        for (uint8_t date_position = 0; date_position < CONFIG_SCHEDULING_MAX_VALUE; date_position++)
        {
            if (scheduling_timestamp_now > scheduling_off_date_current[date_position].end_date
                && scheduling_off_date_current[date_position].end_date != 0
                && strcmp(scheduling_off_date_current[date_position].scheduling_id, "") > 0)
            {
                scheduling_hang_up_call(TYPE_HANGS_UP_SCHEDULE_16, IDP_16, scheduling_off_date_current[date_position].scheduling_id, scheduling_off_date_current[date_position].str_author);
                scheduling_deactivate(scheduling_off_date_current[date_position].scheduling_id, true);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // 5 seconds
    }
}

/**
 * @brief Task for processing scheduling events with IDP 17.
 */
static void scheduling_task_idp_17(void* arg)
{
    const uint8_t angle_off_set = 5;

    while (1)
    {
        for(uint8_t angle_position = 0; angle_position < CONFIG_SCHEDULING_MAX_VALUE; angle_position++)
        {
            if (*scheduling_current_angle > (scheduling_off_angle_current[angle_position].end_angle - angle_off_set)
                && *scheduling_current_angle < (scheduling_off_angle_current[angle_position].end_angle + angle_off_set)
                && strcmp(scheduling_off_angle_current[angle_position].scheduling_id, "") > 0)
            {
                scheduling_hang_up_call(TYPE_HANGS_UP_SCHEDULE_17, IDP_17, scheduling_off_angle_current[angle_position].scheduling_id, scheduling_off_angle_current[angle_position].str_author);
                scheduling_deactivate(scheduling_off_angle_current[angle_position].scheduling_id, true);
            }
        }
 
        vTaskDelay(pdMS_TO_TICKS(5000)); // 5 seconds
    }
}

/**
 * @brief Starts a scheduling task based on the provided IDP and data.
 *
 * This function initiates a scheduling task based on the given IDP (InterDevice Protocol) and the associated data.
 * The task is selected according to the IDP, and the data is used to configure and execute the task.
 *
 * @param scheduling_idp The InterDevice Protocol identifier for scheduling.
 * @param scheduling_data Pointer to the data structure associated with the scheduling task.
 *
 * @note The function checks for null values in the scheduling_data parameter and logs an error if found.
 */
void scheduling_start(idp_type scheduling_idp, void* scheduling_data)
{
	if(scheduling_data == NULL)
	{
		ESP_LOGE(SCHEDULING_TAG, "Scheduling parameter is NULL");
		return;
	}

	time_t scheduling_timestamp_now = rtc_app_get_timestamp(false);

	switch (scheduling_idp)
	{
		case IDP_14:
		{
			memset(scheduling_date_status, false, sizeof(scheduling_date_status));
			memcpy(scheduling_date_current, scheduling_data, sizeof(scheduling_date_current));

			for(uint8_t date_position = 0; date_position < CONFIG_SCHEDULING_MAX_VALUE; date_position++)
			{
				if(scheduling_timestamp_now > scheduling_date_current[date_position].end_date
				&& strcmp(scheduling_date_current[date_position].scheduling_id,"") > 0)
				{
					data_app_delete_scheduling(scheduling_date_current[date_position].scheduling_id);
					data_app_load(DATA_TYPE_SCHEDULING_DATE, &scheduling_date_current);
				}
			}

			if(xTask_scheduling_idp_14 == NULL)
			{
				xTaskCreate(&scheduling_task_idp_14,
						"task_idp_14",
						SCHEDULING_TASK_SIZE,
						NULL,
						SCHEDULING_TASK_PRIORITY,
						&xTask_scheduling_idp_14);
			}

			break;
		}
		case IDP_15:
		{
			memset(scheduling_angle_status, false, sizeof(scheduling_angle_status));
			memcpy(scheduling_angle_current, scheduling_data, sizeof(scheduling_angle_current));

			for(uint8_t angle_position = 0; angle_position < CONFIG_SCHEDULING_MAX_VALUE; angle_position++)
			{
				if((scheduling_timestamp_now > scheduling_angle_current[angle_position].start_date)
				&& (scheduling_timestamp_now - scheduling_angle_current[angle_position].start_date) > 3600
				&& (strcmp(scheduling_angle_current[angle_position].scheduling_id,"") > 0))
				{
					data_app_delete_scheduling(scheduling_angle_current[angle_position].scheduling_id);
					data_app_load(DATA_TYPE_SCHEDULING_ANGLE, &scheduling_angle_current);
				}
			}

			if(xTask_scheduling_idp_15 == NULL)
			{
				xTaskCreate(&scheduling_task_idp_15,
						"task_idp_15",
						SCHEDULING_TASK_SIZE,
						NULL,
						SCHEDULING_TASK_PRIORITY,
						&xTask_scheduling_idp_15);
			}

			break;
		}
		case IDP_16:
		{
			memcpy(scheduling_off_date_current, scheduling_data, sizeof(scheduling_off_date_current));

			for(uint8_t date_position = 0; date_position < CONFIG_SCHEDULING_MAX_VALUE; date_position++)
			{
				if(scheduling_timestamp_now > scheduling_off_date_current[date_position].end_date
				&& strcmp(scheduling_off_date_current[date_position].scheduling_id,"") > 0)
				{
					data_app_delete_scheduling(scheduling_off_date_current[date_position].scheduling_id);
					data_app_load(DATA_TYPE_SCHEDULING_DATE, &scheduling_off_date_current);
				}
			}

			if(xTask_scheduling_idp_16 == NULL)
			{
				xTaskCreate(&scheduling_task_idp_16,
						"task_idp_16",
						SCHEDULING_TASK_SIZE,
						NULL,
						SCHEDULING_TASK_PRIORITY,
						&xTask_scheduling_idp_16);
			}

			break;
		}
		case IDP_17:
		{
            uint8_t angle_offset = 5;

			memcpy(scheduling_off_angle_current, scheduling_data, sizeof(scheduling_off_angle_current));

            for(uint8_t angle_position = 0; angle_position < CONFIG_SCHEDULING_MAX_VALUE; angle_position++)
			{
				if((*scheduling_current_angle + angle_offset > scheduling_off_angle_current[angle_position].end_angle
                && *scheduling_current_angle - angle_offset < scheduling_off_angle_current[angle_position].end_angle)
				&& strcmp(scheduling_off_angle_current[angle_position].scheduling_id,"") > 0)
				{
					data_app_delete_scheduling(scheduling_off_angle_current[angle_position].scheduling_id);
					data_app_load(DATA_TYPE_SCHEDULING_OFF_ANGLE, &scheduling_off_angle_current);
				}
			}

			if(xTask_scheduling_idp_17 == NULL)
			{
				xTaskCreate(&scheduling_task_idp_17,
						"task_idp_17",
						SCHEDULING_TASK_SIZE,
						NULL,
						SCHEDULING_TASK_PRIORITY,
						&xTask_scheduling_idp_17);
			}

			break;
		}
		default:
		{
			ESP_LOGE(SCHEDULING_TAG, "invalid scheduling idp %s", __func__);
			LOG_DBG_ERROR(SCHEDULING_TAG, "invalid_scheduling_idp");
			break;
		}
	}
}

/**
 * @brief Registers a callback function for scheduling events.
 *
 * This function allows the registration of a callback function that will be invoked for scheduling events.
 * The callback function is used to communicate with other modules or external systems.
 *
 * @param callback The callback function to be registered.
 *
 * @note The function checks for null values in the callback parameter.
 */
void scheduling_register_callback(const app_callback callback)
{
	if(callback != NULL)
	{
		scheduling_callback = callback;
	}
}

/**
 * @brief Registers a callback function for scheduling hang up events.
 *
 * This function allows the registration of a callback that will be invoked when scheduling hang up events occur.
 * The callback is used to notify other modules or external systems about these events.
 *
 * @param callback The hang up callback function to be registered.
 *
 * @note The function checks that the callback parameter is not NULL.
 */
void scheduling_hangs_up_callback(const hangs_up_callback callback)
{
    if(callback != NULL)
    {
        scheduling_hang_up_call = callback;
    }
}
