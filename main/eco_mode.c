/**
 * @file eco_mode.c
 * @brief Implementation of the Eco Mode module.
 * @author soil-dev
 * @date 15 de out. de 2023
 */

#include "eco_mode.h"
#include "actuation_app.h"
#include "data_app.h"
#include "rtc_app.h"
#include "FreeRTOS_defines.h"
#include "log.h"

#include <string.h>

/**
 * @brief Tag used for Eco Mode log messages.
 */
#define ECO_MODE_TAG "eco_mode"

/**
 * @brief Name of the Eco Mode task.
 */
static bool enabled_rush_mode = false;

/**
 * @brief Tracks if Eco Mode has already turned off the pivot.
 */
static bool already_off = false;

/**
 * @brief Indicates if Eco Mode was temporarily suspended by command.
 */
static bool eco_mode_suspended = false;

/**
 * @brief Task handle for the Eco Mode task.
 */
static TaskHandle_t xTask_eco_mode = NULL;

/**
 * @brief Current Eco Mode configuration.
 */
static eco_mode_config eco_mode = {};

/**
 * @brief Callback function for Eco Mode events.
 */
static app_callback eco_mode_callback = NULL;

static portMUX_TYPE s_eco_mode_mux = portMUX_INITIALIZER_UNLOCKED;

/**
 * @brief Eco Mode task implementation.
 * @param arg Task argument (unused).
 */
static void eco_mode_task(void *arg);

void eco_mode_start(eco_mode_config current_eco_mode);
void eco_mode_stop(void);
void eco_mode_register_callback(const app_callback callback);

/**
 * @brief Checks if a given timestamp falls on a weekend.
 * @param ts Timestamp in seconds.
 * @return true if Saturday or Sunday, false otherwise.
 */
static bool eco_mode_weekend(time_t ts)
{
    int days = ts / 86400;
    int weekday = (days + 4) % 7;
    return (weekday == 0 || weekday == 6);
}

/**
 * @brief Checks if a time is inside the configured Eco Mode window.
 * @param current_time Current timestamp in seconds.
 * @return true if inside the Eco Mode window, false otherwise.
 */
static bool eco_mode_is_in_window_internal(time_t current_time)
{
    time_t start_time = 0;
    time_t end_time = 0;

    taskENTER_CRITICAL(&s_eco_mode_mux);
    start_time = eco_mode.start_time;
    end_time = eco_mode.end_time;
    taskEXIT_CRITICAL(&s_eco_mode_mux);

    if (start_time == 0 || end_time == 0)
    {
        return false;
    }

    if (eco_mode_weekend(current_time))
    {
        return false;
    }

    if (start_time >= end_time)
    {
        return false;
    }

    time_t current_seconds = current_time % 86400;
    time_t start_seconds = start_time % 86400;
    time_t end_seconds = end_time % 86400;

    if (current_seconds >= start_seconds && current_seconds <= end_seconds)
    {
        return true;
    }

    return false;
}

/**
 * @brief Eco Mode task main loop.
 * @param arg Task argument (unused).
 */
static void eco_mode_task(void *arg)
{
    pivot_actions old_actions = {};
    time_t current_time = 0;

    while (1)
    {
        bool enabled = false;

        taskENTER_CRITICAL(&s_eco_mode_mux);
        enabled = eco_mode.enable;
        taskEXIT_CRITICAL(&s_eco_mode_mux);

        if (enabled == false)
        {
            vTaskDelay(pdMS_TO_TICKS(15000));
            continue;
        }

        current_time = rtc_app_get_timestamp(false) + (RTC_CONFIG_TIMEZONE * 3600);

        if (eco_mode_weekend(current_time))
        {
            bool local_already_off = false;

            taskENTER_CRITICAL(&s_eco_mode_mux);
            local_already_off = already_off;
            taskEXIT_CRITICAL(&s_eco_mode_mux);

            if (local_already_off)
            {
                actuation_app_set_actions(old_actions, false);
                if (eco_mode_callback != NULL)
                {
                    eco_mode_callback("#00$", comm_main_mode);
                }

                taskENTER_CRITICAL(&s_eco_mode_mux);
                already_off = false;
                eco_mode_suspended = false;
                taskEXIT_CRITICAL(&s_eco_mode_mux);
            }

            ESP_LOGE(ECO_MODE_TAG, "Eco Mode weekend detected, skipping...");
            vTaskDelay(pdMS_TO_TICKS(15000));
            continue;
        }

        time_t start_time = 0;
        time_t end_time = 0;

        taskENTER_CRITICAL(&s_eco_mode_mux);
        start_time = eco_mode.start_time;
        end_time = eco_mode.end_time;
        taskEXIT_CRITICAL(&s_eco_mode_mux);

        if (start_time < end_time)
        {
            bool in_window = eco_mode_is_in_window_internal(current_time);

            actuation_app_set_eco_window_state(in_window);

            bool suspended = false;

            taskENTER_CRITICAL(&s_eco_mode_mux);
            suspended = eco_mode_suspended;
            taskEXIT_CRITICAL(&s_eco_mode_mux);

            if (suspended)
            {
                if (!in_window)
                {
                    taskENTER_CRITICAL(&s_eco_mode_mux);
                    eco_mode_suspended = false;
                    taskEXIT_CRITICAL(&s_eco_mode_mux);
                }
                else
                {
                    vTaskDelay(pdMS_TO_TICKS(15000));
                    continue;
                }
            }

            bool local_already_off = false;

            taskENTER_CRITICAL(&s_eco_mode_mux);
            local_already_off = already_off;
            taskEXIT_CRITICAL(&s_eco_mode_mux);

            if (in_window)
            {
                if (!local_already_off)
                {
                    if (eco_mode_callback != NULL)
                    {
                        data_app_load(DATA_TYPE_ACTIONS, &old_actions);
                        eco_mode_callback("#01-eco_mode-002-000-eco_mode$", comm_main_mode);

                        taskENTER_CRITICAL(&s_eco_mode_mux);
                        already_off = true;
                        taskEXIT_CRITICAL(&s_eco_mode_mux);
                    }
                }
            }
            else if (local_already_off)
            {
                actuation_app_set_actions(old_actions, false);
                if (eco_mode_callback != NULL)
                {
                    eco_mode_callback("#00$", comm_main_mode);
                }

                taskENTER_CRITICAL(&s_eco_mode_mux);
                already_off = false;
                taskEXIT_CRITICAL(&s_eco_mode_mux);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(15000)); // 15 seconds
    }
}

/**
 * @brief Starts Eco Mode with the provided configuration.
 * @param current_eco_mode Eco Mode configuration.
 */
void eco_mode_start(eco_mode_config current_eco_mode)
{
    if (current_eco_mode.start_time == 0 || current_eco_mode.end_time == 0)
    {
        eco_mode_stop();
    }
    else
    {
        taskENTER_CRITICAL(&s_eco_mode_mux);
        memcpy(&eco_mode, &current_eco_mode, sizeof(eco_mode));
        taskEXIT_CRITICAL(&s_eco_mode_mux);

        if (xTask_eco_mode == NULL)
        {
            BaseType_t ok = xTaskCreate(&eco_mode_task,
                                       ECO_MODE_TASK_NAME,
                                       ECO_MODE_TASK_SIZE,
                                       NULL,
                                       ECO_MODE_TASK_PRIORITY,
                                       &xTask_eco_mode);
            if (ok != pdPASS)
            {
                xTask_eco_mode = NULL;
                ESP_LOGE(ECO_MODE_TAG, "Eco Mode task creation failed");
            }
        }

        enabled_rush_mode = eco_mode.enable;

        if (enabled_rush_mode)
        {
            ESP_LOGI(ECO_MODE_TAG, "Rush Mode enabled");
            ESP_LOGI(ECO_MODE_TAG, "Rush Mode started: Start Time: %lld, End Time: %lld", eco_mode.start_time, eco_mode.end_time);
        }
        else
        {
            ESP_LOGE(ECO_MODE_TAG, "Rush Mode disabled");
        }
    }
}

/**
 * @brief Stops Eco Mode and resets its state.
 */
void eco_mode_stop(void)
{
    if (xTask_eco_mode != NULL)
    {
        vTaskDelete(xTask_eco_mode);
        xTask_eco_mode = NULL;
        ESP_LOGE(ECO_MODE_TAG, "Eco Mode stopped");

        taskENTER_CRITICAL(&s_eco_mode_mux);
        already_off = false;
        eco_mode_suspended = false;
        taskEXIT_CRITICAL(&s_eco_mode_mux);
    }
}

/**
 * @brief Suspends Eco Mode actions on command without stopping the task.
 */
void eco_mode_cmd_stop(void)
{
    bool local_already_off = false;

    taskENTER_CRITICAL(&s_eco_mode_mux);
    local_already_off = already_off;
    taskEXIT_CRITICAL(&s_eco_mode_mux);

    if (xTask_eco_mode != NULL && local_already_off == true)
    {
        taskENTER_CRITICAL(&s_eco_mode_mux);
        eco_mode_suspended = true;
        already_off = false;
        taskEXIT_CRITICAL(&s_eco_mode_mux);

        ESP_LOGE(ECO_MODE_TAG, "Eco Mode stopped by command");
        if (eco_mode_callback != NULL)
        {
            eco_mode_callback("#32-rush_mode_deactivated$", comm_main_mode);
        }
    }
}

/**
 * @brief Registers a callback for Eco Mode events.
 * @param callback Callback to be registered.
 */
void eco_mode_register_callback(const app_callback callback)
{
    if (callback != NULL)
    {
        ESP_LOGE(ECO_MODE_TAG, "Eco Mode callback registered");
        taskENTER_CRITICAL(&s_eco_mode_mux);
        eco_mode_callback = callback;
        taskEXIT_CRITICAL(&s_eco_mode_mux);
    }
}
