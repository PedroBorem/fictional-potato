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

#define ECO_MODE_TAG "eco_mode"

/**
 * @brief Variable to track if the Eco Mode has already turned off the pivot.
 */
static bool already_off = false;

/**
 * @brief Task handle for the Eco Mode task.
 */
static TaskHandle_t xTask_eco_mode = NULL;

/**
 * @brief Configuration structure for Eco Mode.
 */
static eco_mode_config eco_mode = {};

/**
 * @brief Callback function for Eco Mode events.
 */
static app_callback eco_mode_callback = NULL;

/**
 * @brief Eco Mode task implementation.
 * @param arg Task argument (not used).
 */
static void eco_mode_task(void *arg);

/**
 * @brief Starts the Eco Mode with the provided configuration.
 * @param current_eco_mode Configuration for Eco Mode.
 * @note If the start or end time is not set, the Eco Mode will be stopped.
 */
void eco_mode_start(eco_mode_config current_eco_mode);

/**
 * @brief Stops the Eco Mode.
 */
void eco_mode_stop(void);

/**
 * @brief Registers a callback function for Eco Mode events.
 * @param callback Callback function to be registered.
 */
void eco_mode_register_callback(const app_callback callback);

static bool eco_mode_weekend(time_t ts)
{
    int days = ts / 86400;
    int weekday = (days + 4) % 7;
    return (weekday == 0 || weekday == 6);
}

/**
 * @brief Eco Mode task implementation.
 */
static void eco_mode_task(void *arg)
{
    pivot_actions old_actions = {};
    time_t current_time = 0;  

    while (1)
    {
        current_time = rtc_app_get_timestamp(false);

        if (eco_mode_weekend(current_time))
        {
            if (already_off)
            {
                already_off = false;
            }
            ESP_LOGE(ECO_MODE_TAG, "Eco Mode weekend detected, skipping...");
            vTaskDelay(pdMS_TO_TICKS(15000));
            continue;
        }

        if (eco_mode.start_time < eco_mode.end_time)
        {
            time_t current_seconds = current_time % 86400;
            time_t start_seconds = eco_mode.start_time % 86400;
            time_t end_seconds = eco_mode.end_time % 86400;

            if (current_seconds >= start_seconds && current_seconds <= end_seconds)
            {
                if (!already_off)
                {
                    if (eco_mode_callback != NULL)
                    {
                        data_app_load(DATA_TYPE_ACTIONS, &old_actions);
                        eco_mode_callback("#01-eco_mode-002-000-eco_mode$", comm_main_mode);
                    }
                    already_off = true;
                }
            }
            else if (already_off)
            {
                actuation_app_set_actions(old_actions, false);
                if (eco_mode_callback != NULL)
                {
                    eco_mode_callback("#00$", comm_main_mode);
                }
                already_off = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(15000)); // 15 seconds
    }
}

void eco_mode_start(eco_mode_config current_eco_mode)
{
    if (current_eco_mode.start_time == 0 || current_eco_mode.end_time == 0)
    {
        eco_mode_stop();
    }
    else
    {
        memcpy(&eco_mode, &current_eco_mode, sizeof(eco_mode));
        xTaskCreate(&eco_mode_task,
                    ECO_MODE_TASK_NAME,
                    ECO_MODE_TASK_SIZE,
                    NULL,
                    ECO_MODE_TASK_PRIORITY,
                    &xTask_eco_mode);

        ESP_LOGI(ECO_MODE_TAG, "Eco Mode started: Start Time: %lld, End Time: %lld", eco_mode.start_time, eco_mode.end_time);
    }
}

void eco_mode_stop(void)
{
    if (xTask_eco_mode != NULL)
    {
        vTaskDelete(xTask_eco_mode);
        xTask_eco_mode = NULL;
        ESP_LOGE(ECO_MODE_TAG, "Eco Mode stopped");
        already_off = false;
    }
}

void eco_mode_cmd_stop(void)
{
    if (xTask_eco_mode != NULL && already_off == true)
    {
        vTaskDelete(xTask_eco_mode);
        xTask_eco_mode = NULL;
        already_off = false;
        ESP_LOGE(ECO_MODE_TAG, "Eco Mode stopped by command");
        eco_mode_callback("#32-soilteste_2-rush_mode_deactivated$", comm_main_mode);
    }
}

void eco_mode_register_callback(const app_callback callback)
{
    if (callback != NULL)
    {
        ESP_LOGE(ECO_MODE_TAG, "Eco Mode callback registered");
        eco_mode_callback = callback;
    }
}
