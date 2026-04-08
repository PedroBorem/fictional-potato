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
 * @brief Persistent runtime state of the single active start schedule.
 */
static pivot_scheduling_start_state scheduling_start_state = {};

/**
 * @brief End-angle stabilization deadline for the active IDP 15 schedule.
 */
static time_t scheduling_angle_end_guard_until = 0;

/**
 * @brief Scheduling identifier protected by the current IDP 15 guard window.
 */
static char scheduling_angle_end_guard_id[30] = {};

/**
 * @brief Synchronizes access to the shared runtime scheduling state.
 */
static portMUX_TYPE s_scheduling_mux = portMUX_INITIALIZER_UNLOCKED;

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
 * @brief Stores the runtime state for IDP 14 scheduling atomically.
 *
 * @param scheduling_date Runtime array for date schedules.
 * @param scheduling_status Runtime execution flags for date schedules.
 * @param start_state Persisted active start schedule state mirrored in RAM.
 */
static void scheduling_store_runtime_date_internal(
    const pivot_scheduling_date scheduling_date[CONFIG_SCHEDULING_MAX_VALUE],
    const bool scheduling_status[CONFIG_SCHEDULING_MAX_VALUE],
    const pivot_scheduling_start_state *start_state);

/**
 * @brief Stores the runtime state for IDP 15 scheduling atomically.
 *
 * @param scheduling_angle Runtime array for angle schedules.
 * @param scheduling_status Runtime execution flags for angle schedules.
 * @param start_state Persisted active start schedule state mirrored in RAM.
 */
static void scheduling_store_runtime_angle_internal(
    const pivot_scheduling_angle scheduling_angle[CONFIG_SCHEDULING_MAX_VALUE],
    const bool scheduling_status[CONFIG_SCHEDULING_MAX_VALUE],
    const pivot_scheduling_start_state *start_state);

/**
 * @brief Stores the runtime state for IDP 16 scheduling atomically.
 *
 * @param scheduling_off_date Runtime array for date shutdown schedules.
 */
static void scheduling_store_runtime_off_date_internal(const pivot_scheduling_off_date scheduling_off_date[CONFIG_SCHEDULING_MAX_VALUE]);

/**
 * @brief Stores the runtime state for IDP 17 scheduling atomically.
 *
 * @param scheduling_off_angle Runtime array for angle shutdown schedules.
 */
static void scheduling_store_runtime_off_angle_internal(const pivot_scheduling_off_angle scheduling_off_angle[CONFIG_SCHEDULING_MAX_VALUE]);

/**
 * @brief Arms the IDP 15 end-angle stabilization guard for 5 minutes.
 *
 * @param scheduling_id Scheduling identifier currently protected.
 * @param current_time Current timestamp used as the guard base.
 */
static void scheduling_angle_guard_start_internal(const char *scheduling_id, time_t current_time);

/**
 * @brief Clears the IDP 15 end-angle stabilization guard.
 *
 * @param scheduling_id Scheduling identifier to be cleared from the guard state.
 */
static void scheduling_angle_guard_clear_internal(const char *scheduling_id);

/**
 * @brief Activates the scheduling at the specified position.
 * @param scheduling_idp The scheduling IDP type that is starting.
 * @param position The position of the scheduling date or angle in the array.
 * @param scheduling_id The ID of the scheduling.
 * @param actions The pivot actions to be performed.
 */
static void scheduling_active(idp_type scheduling_idp, uint8_t position, char* scheduling_id, pivot_actions actions);

/**
 * @brief Requests a schedule removal using the normal IDP 13 flow.
 * @param scheduling_id The scheduling ID to be removed.
 */
static void scheduling_request_schedule_delete(char* scheduling_id);

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
 * @brief Stores the runtime state for IDP 14 scheduling atomically.
 *
 * @param scheduling_date Runtime array for date schedules.
 * @param scheduling_status Runtime execution flags for date schedules.
 * @param start_state Persisted active start schedule state mirrored in RAM.
 */
static void scheduling_store_runtime_date_internal(
    const pivot_scheduling_date scheduling_date[CONFIG_SCHEDULING_MAX_VALUE],
    const bool scheduling_status[CONFIG_SCHEDULING_MAX_VALUE],
    const pivot_scheduling_start_state *start_state)
{
    taskENTER_CRITICAL(&s_scheduling_mux);
    memcpy(scheduling_date_current, scheduling_date, sizeof(scheduling_date_current));
    memcpy(scheduling_date_status, scheduling_status, sizeof(scheduling_date_status));
    memcpy(&scheduling_start_state, start_state, sizeof(scheduling_start_state));
    taskEXIT_CRITICAL(&s_scheduling_mux);
}

/**
 * @brief Stores the runtime state for IDP 15 scheduling atomically.
 *
 * @param scheduling_angle Runtime array for angle schedules.
 * @param scheduling_status Runtime execution flags for angle schedules.
 * @param start_state Persisted active start schedule state mirrored in RAM.
 */
static void scheduling_store_runtime_angle_internal(
    const pivot_scheduling_angle scheduling_angle[CONFIG_SCHEDULING_MAX_VALUE],
    const bool scheduling_status[CONFIG_SCHEDULING_MAX_VALUE],
    const pivot_scheduling_start_state *start_state)
{
    taskENTER_CRITICAL(&s_scheduling_mux);
    memcpy(scheduling_angle_current, scheduling_angle, sizeof(scheduling_angle_current));
    memcpy(scheduling_angle_status, scheduling_status, sizeof(scheduling_angle_status));
    memcpy(&scheduling_start_state, start_state, sizeof(scheduling_start_state));
    taskEXIT_CRITICAL(&s_scheduling_mux);
}

/**
 * @brief Stores the runtime state for IDP 16 scheduling atomically.
 *
 * @param scheduling_off_date Runtime array for date shutdown schedules.
 */
static void scheduling_store_runtime_off_date_internal(const pivot_scheduling_off_date scheduling_off_date[CONFIG_SCHEDULING_MAX_VALUE])
{
    taskENTER_CRITICAL(&s_scheduling_mux);
    memcpy(scheduling_off_date_current, scheduling_off_date, sizeof(scheduling_off_date_current));
    taskEXIT_CRITICAL(&s_scheduling_mux);
}

/**
 * @brief Stores the runtime state for IDP 17 scheduling atomically.
 *
 * @param scheduling_off_angle Runtime array for angle shutdown schedules.
 */
static void scheduling_store_runtime_off_angle_internal(const pivot_scheduling_off_angle scheduling_off_angle[CONFIG_SCHEDULING_MAX_VALUE])
{
    taskENTER_CRITICAL(&s_scheduling_mux);
    memcpy(scheduling_off_angle_current, scheduling_off_angle, sizeof(scheduling_off_angle_current));
    taskEXIT_CRITICAL(&s_scheduling_mux);
}

/**
 * @brief Arms the IDP 15 end-angle stabilization guard for 5 minutes.
 *
 * @param scheduling_id Scheduling identifier currently protected.
 * @param current_time Current timestamp used as the guard base.
 */
static void scheduling_angle_guard_start_internal(const char *scheduling_id, time_t current_time)
{
    if (scheduling_id == NULL || scheduling_id[0] == '\0')
    {
        return;
    }

    taskENTER_CRITICAL(&s_scheduling_mux);
    scheduling_angle_end_guard_until = current_time + 300;
    memset(scheduling_angle_end_guard_id, 0x00, sizeof(scheduling_angle_end_guard_id));
    memcpy(scheduling_angle_end_guard_id, scheduling_id, strlen(scheduling_id));
    taskEXIT_CRITICAL(&s_scheduling_mux);
}

/**
 * @brief Clears the IDP 15 end-angle stabilization guard.
 *
 * @param scheduling_id Scheduling identifier to be cleared from the guard state.
 */
static void scheduling_angle_guard_clear_internal(const char *scheduling_id)
{
    taskENTER_CRITICAL(&s_scheduling_mux);
    if (scheduling_id == NULL ||
        scheduling_id[0] == '\0' ||
        strcmp(scheduling_angle_end_guard_id, scheduling_id) == 0)
    {
        scheduling_angle_end_guard_until = 0;
        memset(scheduling_angle_end_guard_id, 0x00, sizeof(scheduling_angle_end_guard_id));
    }
    taskEXIT_CRITICAL(&s_scheduling_mux);
}

/**
 * @brief Activates the scheduling at the specified position.
 * @param scheduling_idp The scheduling IDP type that is starting.
 * @param position The position of the scheduling date or angle in the array.
 * @param scheduling_id The ID of the scheduling.
 * @param actions The pivot actions to be performed.
 */
static void scheduling_active(idp_type scheduling_idp, uint8_t position, char* scheduling_id, pivot_actions actions)
{
    uint8_t idp = IDP_INVALID;
    uint16_t dwp = 0;
    char str_out[50] = {};
    esp_err_t ret = ESP_FAIL;
    pivot_scheduling_start_state scheduling_start_state_local = {};

    if (scheduling_callback == NULL)
    {
        ESP_LOGE(SCHEDULING_TAG, "invalid callback");
        return;
    }

    taskENTER_CRITICAL(&s_scheduling_mux);
    if (scheduling_idp == IDP_14)
    {
        scheduling_date_status[position] = true;
    }
    else if (scheduling_idp == IDP_15)
    {
        scheduling_angle_status[position] = true;
    }

    scheduling_start_state_local.active = true;
    scheduling_start_state_local.scheduling_idp = scheduling_idp;
    memset(scheduling_start_state_local.scheduling_id, 0x00, sizeof(scheduling_start_state_local.scheduling_id));
    memcpy(scheduling_start_state_local.scheduling_id, scheduling_id, strlen(scheduling_id));
    memcpy(&scheduling_start_state, &scheduling_start_state_local, sizeof(scheduling_start_state));
    taskEXIT_CRITICAL(&s_scheduling_mux);

    ret = data_app_save(DATA_TYPE_SCHEDULING_START_STATE, &scheduling_start_state_local, sizeof(scheduling_start_state_local));

    if (ret != ESP_OK)
    {
        ESP_LOGE(SCHEDULING_TAG, "failed to persist active start state for schedule id : %s", scheduling_id);
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
 * @brief Requests a schedule removal using the normal IDP 13 flow.
 * @param scheduling_id The scheduling ID to be removed.
 */
static void scheduling_request_schedule_delete(char* scheduling_id)
{
    uint8_t idp = IDP_13;
    char str_out[50] = {};
    char scheduling_id_copy[50] = {};

    if (scheduling_callback == NULL || scheduling_id == NULL || scheduling_id[0] == '\0')
    {
        return;
    }

    memcpy(scheduling_id_copy, scheduling_id, strlen(scheduling_id));

    arg_pair_t arg_idp_13[] =
    {
        { "uint8_t", &idp },
        { "string", SCHEDULING_TAG },
        { "string", scheduling_id_copy },
        { "string", SCHEDULING_TAG },
        { NULL, NULL }
    };

    memset(str_out, 0x00, sizeof(str_out));
    idp_parser_create_package(str_out, arg_idp_13);
    scheduling_callback(str_out, COMM_MQTT);
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
    bool clear_start_state = false;
    pivot_scheduling_start_state scheduling_start_state_clear = {};

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

    taskENTER_CRITICAL(&s_scheduling_mux);
    if (scheduling_start_state.active &&
        strcmp(scheduling_start_state.scheduling_id, scheduling_id) == 0)
    {
        memcpy(&scheduling_start_state, &scheduling_start_state_clear, sizeof(scheduling_start_state));
        clear_start_state = true;
    }
    taskEXIT_CRITICAL(&s_scheduling_mux);

    if (clear_start_state)
    {
        data_app_save(DATA_TYPE_SCHEDULING_START_STATE, &scheduling_start_state_clear, sizeof(scheduling_start_state_clear));
    }

    scheduling_angle_guard_clear_internal(scheduling_id);

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
    const time_t date_offset = TIMESTAMP_OFFSET_SCHEDULING;
    pivot_scheduling_date scheduling_date = {};
    pivot_scheduling_start_state scheduling_start_state_local = {};
    bool scheduling_date_status_local = false;

    while (1)
    {
        scheduling_timestamp_now = rtc_app_get_timestamp(false);

        for (uint8_t date_position = 0; date_position < CONFIG_SCHEDULING_MAX_VALUE; date_position++)
        {
            taskENTER_CRITICAL(&s_scheduling_mux);
            memcpy(&scheduling_date, &scheduling_date_current[date_position], sizeof(scheduling_date));
            memcpy(&scheduling_start_state_local, &scheduling_start_state, sizeof(scheduling_start_state_local));
            scheduling_date_status_local = scheduling_date_status[date_position];
            taskEXIT_CRITICAL(&s_scheduling_mux);

            if (strcmp(scheduling_date.scheduling_id, "") > 0)
            {
                if (!scheduling_date_status_local &&
                    !scheduling_start_state_local.active &&
                    (scheduling_timestamp_now >= scheduling_date.start_date) &&
                    (scheduling_timestamp_now <= (scheduling_date.start_date) + date_offset))
                {
                    scheduling_active(IDP_14,
                                      date_position,
                                      scheduling_date.scheduling_id,
                                      scheduling_date.actions);
                }
                else if (scheduling_date_status_local &&
                         (scheduling_timestamp_now >= scheduling_date.end_date) &&
                         (scheduling_timestamp_now <= (scheduling_date.end_date + date_offset)))
                {
                    scheduling_hang_up_call(TYPE_HANGS_UP_SCHEDULE_14, IDP_14,
                                            scheduling_date.scheduling_id,
                                            scheduling_date.str_author);
                    taskENTER_CRITICAL(&s_scheduling_mux);
                    scheduling_date_status[date_position] = false;
                    taskEXIT_CRITICAL(&s_scheduling_mux);
                    scheduling_deactivate(scheduling_date.scheduling_id, false);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // 5 segundos
    }
}


/**
 * @brief Task for processing scheduling events with IDP 15.
 */
static void scheduling_task_idp_15(void* arg)
{
    const uint16_t angle_off_set = 3;
    const time_t date_offset = TIMESTAMP_OFFSET_SCHEDULING;
    time_t scheduling_timestamp_now = 0;
    time_t scheduling_angle_end_guard_until_local = 0;
    pivot_scheduling_angle scheduling_angle = {};
    pivot_scheduling_start_state scheduling_start_state_local = {};
    char scheduling_angle_end_guard_id_local[30] = {};
    bool scheduling_angle_status_local = false;

    while (1)
    {
        scheduling_timestamp_now = rtc_app_get_timestamp(false);

        for (uint8_t angle_position = 0; angle_position < CONFIG_SCHEDULING_MAX_VALUE; angle_position++)
        {
            taskENTER_CRITICAL(&s_scheduling_mux);
            memcpy(&scheduling_angle, &scheduling_angle_current[angle_position], sizeof(scheduling_angle));
            memcpy(&scheduling_start_state_local, &scheduling_start_state, sizeof(scheduling_start_state_local));
            scheduling_angle_status_local = scheduling_angle_status[angle_position];
            scheduling_angle_end_guard_until_local = scheduling_angle_end_guard_until;
            memcpy(scheduling_angle_end_guard_id_local,
                   scheduling_angle_end_guard_id,
                   sizeof(scheduling_angle_end_guard_id_local));
            taskEXIT_CRITICAL(&s_scheduling_mux);

            if (strcmp(scheduling_angle.scheduling_id, "") > 0)
            {
                if (!scheduling_angle_status_local &&
                    !scheduling_start_state_local.active &&
                    (scheduling_timestamp_now >= scheduling_angle.start_date) &&
                    (scheduling_timestamp_now <= (scheduling_angle.start_date) + date_offset))
                {
                    scheduling_active(IDP_15,
                                      angle_position,
                                      scheduling_angle.scheduling_id,
                                      scheduling_angle.actions);

                    if (*scheduling_current_angle >= scheduling_angle.end_angle - angle_off_set
                        && *scheduling_current_angle <= scheduling_angle.end_angle + angle_off_set)
                    {
                        scheduling_angle_guard_start_internal(scheduling_angle.scheduling_id, scheduling_timestamp_now);
                    }
                }

                else if (scheduling_angle_status_local &&
                         (*scheduling_current_angle >= scheduling_angle.end_angle - angle_off_set) &&
                         (*scheduling_current_angle <= scheduling_angle.end_angle + angle_off_set))
                {
                    bool scheduling_angle_guard_active = false;

                    if (strcmp(scheduling_angle_end_guard_id_local, scheduling_angle.scheduling_id) == 0 &&
                        scheduling_timestamp_now < scheduling_angle_end_guard_until_local)
                    {
                        scheduling_angle_guard_active = true;
                    }

                    if (!scheduling_angle_guard_active)
                    {
                        scheduling_hang_up_call(TYPE_HANGS_UP_SCHEDULE_15, IDP_15,
                                                scheduling_angle.scheduling_id,
                                                scheduling_angle.str_author);
                        taskENTER_CRITICAL(&s_scheduling_mux);
                        scheduling_angle_status[angle_position] = false;
                        taskEXIT_CRITICAL(&s_scheduling_mux);
                        scheduling_deactivate(scheduling_angle.scheduling_id, false);
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/**
 * @brief Task for processing scheduling events with IDP 16.
 */
static void scheduling_task_idp_16(void* arg)
{
    time_t scheduling_timestamp_now = 0;
    const time_t date_offset = TIMESTAMP_OFFSET_SCHEDULING;
    char active_scheduling_id[50] = {};
    pivot_scheduling_off_date scheduling_off_date = {};
    pivot_scheduling_start_state scheduling_start_state_local = {};

    while (1)
    {
        scheduling_timestamp_now = rtc_app_get_timestamp(false);

        for (uint8_t date_position = 0; date_position < CONFIG_SCHEDULING_MAX_VALUE; date_position++)
        {
            taskENTER_CRITICAL(&s_scheduling_mux);
            memcpy(&scheduling_off_date, &scheduling_off_date_current[date_position], sizeof(scheduling_off_date));
            memcpy(&scheduling_start_state_local, &scheduling_start_state, sizeof(scheduling_start_state_local));
            taskEXIT_CRITICAL(&s_scheduling_mux);

            if(strcmp(scheduling_off_date.scheduling_id, "") > 0)
            {
                if (scheduling_off_date.end_date != 0)
                {
                    if ((scheduling_timestamp_now >= scheduling_off_date.end_date) &&
                        (scheduling_timestamp_now <= (scheduling_off_date.end_date + date_offset)))
                    {
                        if (scheduling_start_state_local.active)
                        {
                            memcpy(active_scheduling_id, scheduling_start_state_local.scheduling_id, strlen(scheduling_start_state_local.scheduling_id));
                            scheduling_request_schedule_delete(active_scheduling_id);
                            memset(active_scheduling_id, 0x00, sizeof(active_scheduling_id));
                        }

                        scheduling_hang_up_call(TYPE_HANGS_UP_SCHEDULE_16, IDP_16,
                                                scheduling_off_date.scheduling_id,
                                                scheduling_off_date.str_author);
                        scheduling_deactivate(scheduling_off_date.scheduling_id, true);
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/**
 * @brief Task for processing scheduling events with IDP 17.
 */
static void scheduling_task_idp_17(void* arg)
{
    const uint8_t angle_off_set = 5;
    char active_scheduling_id[50] = {};
    pivot_scheduling_off_angle scheduling_off_angle = {};
    pivot_scheduling_start_state scheduling_start_state_local = {};

    while (1)
    {
        for(uint8_t angle_position = 0; angle_position < CONFIG_SCHEDULING_MAX_VALUE; angle_position++)
        {
            taskENTER_CRITICAL(&s_scheduling_mux);
            memcpy(&scheduling_off_angle, &scheduling_off_angle_current[angle_position], sizeof(scheduling_off_angle));
            memcpy(&scheduling_start_state_local, &scheduling_start_state, sizeof(scheduling_start_state_local));
            taskEXIT_CRITICAL(&s_scheduling_mux);

            if(strcmp(scheduling_off_angle.scheduling_id, "") > 0)
            {
                if (*scheduling_current_angle > (scheduling_off_angle.end_angle - angle_off_set)
                    && *scheduling_current_angle < (scheduling_off_angle.end_angle + angle_off_set))
                {
                    if (scheduling_start_state_local.active)
                    {
                        memcpy(active_scheduling_id, scheduling_start_state_local.scheduling_id, strlen(scheduling_start_state_local.scheduling_id));
                        scheduling_request_schedule_delete(active_scheduling_id);
                        memset(active_scheduling_id, 0x00, sizeof(active_scheduling_id));
                    }

                    scheduling_hang_up_call(TYPE_HANGS_UP_SCHEDULE_17, IDP_17, scheduling_off_angle.scheduling_id, scheduling_off_angle.str_author);
                    scheduling_deactivate(scheduling_off_angle.scheduling_id, true);
                }
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

	// Runtime context used to reconcile persisted start schedules after reboot.
	time_t scheduling_timestamp_now = rtc_app_get_timestamp(false);
    bool pivot_is_off = actuation_app_is_pivot_off();
    bool pivot_is_pressurizing = actuation_app_is_pivot_pressurizing();
    pivot_scheduling_start_state scheduling_start_state_local = {};
    data_app_load(DATA_TYPE_SCHEDULING_START_STATE, &scheduling_start_state_local);

	switch (scheduling_idp)
	{
		case IDP_14:
		{
			// Indicates whether the persisted active start state matches a loaded IDP 14.
            bool active_schedule_found = false;
            bool scheduling_date_status_local[CONFIG_SCHEDULING_MAX_VALUE] = {};
            pivot_scheduling_date scheduling_date_local[CONFIG_SCHEDULING_MAX_VALUE] = {};
			memcpy(scheduling_date_local, scheduling_data, sizeof(scheduling_date_local));

			for(uint8_t date_position = 0; date_position < CONFIG_SCHEDULING_MAX_VALUE; date_position++)
			{
				if(strcmp(scheduling_date_local[date_position].scheduling_id,"") > 0)
				{
					if(scheduling_timestamp_now > scheduling_date_local[date_position].end_date)
					{
						data_app_delete_scheduling(scheduling_date_local[date_position].scheduling_id);
						data_app_load(DATA_TYPE_SCHEDULING_DATE, &scheduling_date_local);
                        data_app_load(DATA_TYPE_SCHEDULING_START_STATE, &scheduling_start_state_local);
					}
                    else if (scheduling_start_state_local.active &&
                             scheduling_start_state_local.scheduling_idp == IDP_14 &&
                             strcmp(scheduling_start_state_local.scheduling_id, scheduling_date_local[date_position].scheduling_id) == 0)
                    {
                        active_schedule_found = true;

                        if (pivot_is_off && !pivot_is_pressurizing)
                        {
							ESP_LOGW(SCHEDULING_TAG,
									"Clearing interrupted schedule date id : %s",
									scheduling_date_local[date_position].scheduling_id);
							data_app_delete_scheduling(scheduling_date_local[date_position].scheduling_id);
							data_app_load(DATA_TYPE_SCHEDULING_DATE, &scheduling_date_local);
                            data_app_load(DATA_TYPE_SCHEDULING_START_STATE, &scheduling_start_state_local);
                        }
                        else
                        {
                            scheduling_date_status_local[date_position] = true;
                        }
                    }
				}
			}

            if (scheduling_start_state_local.active &&
                scheduling_start_state_local.scheduling_idp == IDP_14 &&
                !active_schedule_found)
            {
                memset(&scheduling_start_state_local, 0x00, sizeof(scheduling_start_state_local));
                data_app_save(DATA_TYPE_SCHEDULING_START_STATE, &scheduling_start_state_local, sizeof(scheduling_start_state_local));
            }

            scheduling_store_runtime_date_internal(
                scheduling_date_local,
                scheduling_date_status_local,
                &scheduling_start_state_local);

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
			// Indicates whether the persisted active start state matches a loaded IDP 15.
            bool active_schedule_found = false;
            bool scheduling_angle_status_local[CONFIG_SCHEDULING_MAX_VALUE] = {};
            pivot_scheduling_angle scheduling_angle_local[CONFIG_SCHEDULING_MAX_VALUE] = {};
			memcpy(scheduling_angle_local, scheduling_data, sizeof(scheduling_angle_local));

			for(uint8_t angle_position = 0; angle_position < CONFIG_SCHEDULING_MAX_VALUE; angle_position++)
			{
				if(strcmp(scheduling_angle_local[angle_position].scheduling_id,"") > 0)
				{
                    if((scheduling_timestamp_now > scheduling_angle_local[angle_position].start_date)
                    && (scheduling_timestamp_now - scheduling_angle_local[angle_position].start_date) > 3600
                    && !scheduling_start_state_local.active)
                    {
                        scheduling_angle_guard_clear_internal(scheduling_angle_local[angle_position].scheduling_id);
						data_app_delete_scheduling(scheduling_angle_local[angle_position].scheduling_id);
						data_app_load(DATA_TYPE_SCHEDULING_ANGLE, &scheduling_angle_local);
                        data_app_load(DATA_TYPE_SCHEDULING_START_STATE, &scheduling_start_state_local);
                    }
                    else if (scheduling_start_state_local.active &&
                             scheduling_start_state_local.scheduling_idp == IDP_15 &&
                             strcmp(scheduling_start_state_local.scheduling_id, scheduling_angle_local[angle_position].scheduling_id) == 0)
                    {
                        active_schedule_found = true;

                        if (pivot_is_off && !pivot_is_pressurizing)
                        {
							ESP_LOGW(SCHEDULING_TAG,
									"Clearing interrupted schedule angle id : %s",
									scheduling_angle_local[angle_position].scheduling_id);
                            scheduling_angle_guard_clear_internal(scheduling_angle_local[angle_position].scheduling_id);
							data_app_delete_scheduling(scheduling_angle_local[angle_position].scheduling_id);
							data_app_load(DATA_TYPE_SCHEDULING_ANGLE, &scheduling_angle_local);
                            data_app_load(DATA_TYPE_SCHEDULING_START_STATE, &scheduling_start_state_local);
                        }
                        else
                        {
                            scheduling_angle_status_local[angle_position] = true;
                        }
                    }
				}
			}

            if (scheduling_start_state_local.active &&
                scheduling_start_state_local.scheduling_idp == IDP_15 &&
                !active_schedule_found)
            {
                scheduling_angle_guard_clear_internal(scheduling_start_state_local.scheduling_id);
                memset(&scheduling_start_state_local, 0x00, sizeof(scheduling_start_state_local));
                data_app_save(DATA_TYPE_SCHEDULING_START_STATE, &scheduling_start_state_local, sizeof(scheduling_start_state_local));
            }

            scheduling_store_runtime_angle_internal(
                scheduling_angle_local,
                scheduling_angle_status_local,
                &scheduling_start_state_local);

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
            pivot_scheduling_off_date scheduling_off_date_local[CONFIG_SCHEDULING_MAX_VALUE] = {};
			memcpy(scheduling_off_date_local, scheduling_data, sizeof(scheduling_off_date_local));

			for(uint8_t date_position = 0; date_position < CONFIG_SCHEDULING_MAX_VALUE; date_position++)
			{
				if(scheduling_timestamp_now > scheduling_off_date_local[date_position].end_date
				&& strcmp(scheduling_off_date_local[date_position].scheduling_id,"") > 0)
				{
					data_app_delete_scheduling(scheduling_off_date_local[date_position].scheduling_id);
					data_app_load(DATA_TYPE_SCHEDULING_OFF_DATE, &scheduling_off_date_local);
				}
			}

            scheduling_store_runtime_off_date_internal(scheduling_off_date_local);

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
            pivot_scheduling_off_angle scheduling_off_angle_local[CONFIG_SCHEDULING_MAX_VALUE] = {};

			memcpy(scheduling_off_angle_local, scheduling_data, sizeof(scheduling_off_angle_local));

            for(uint8_t angle_position = 0; angle_position < CONFIG_SCHEDULING_MAX_VALUE; angle_position++)
			{
				if((*scheduling_current_angle + angle_offset > scheduling_off_angle_local[angle_position].end_angle
                && *scheduling_current_angle - angle_offset < scheduling_off_angle_local[angle_position].end_angle)
				&& strcmp(scheduling_off_angle_local[angle_position].scheduling_id,"") > 0)
				{
					data_app_delete_scheduling(scheduling_off_angle_local[angle_position].scheduling_id);
					data_app_load(DATA_TYPE_SCHEDULING_OFF_ANGLE, &scheduling_off_angle_local);
				}
			}

            scheduling_store_runtime_off_angle_internal(scheduling_off_angle_local);

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
