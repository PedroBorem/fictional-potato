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

/**
 * @brief Persisted state for the current Eco Mode window.
 */
static eco_mode_saved_state eco_mode_state = {};

static portMUX_TYPE s_eco_mode_mux = portMUX_INITIALIZER_UNLOCKED;

/**
 * @brief Eco Mode task implementation.
 * @param arg Task argument (unused).
 */
static void eco_mode_task(void *arg);
static bool eco_mode_get_window_bounds_internal(time_t current_time, time_t *window_start, time_t *window_end);
static time_t eco_mode_get_current_time_internal(void);
static void eco_mode_set_runtime_flags_internal(bool already_off_value, bool suspended_value);
static bool eco_mode_has_saved_window_internal(const eco_mode_saved_state *saved_state);
static bool eco_mode_window_is_suspended_internal(const eco_mode_saved_state *saved_state);
static void eco_mode_store_saved_state_internal(const eco_mode_saved_state *saved_state);
static void eco_mode_clear_saved_state_internal(void);
static void eco_mode_store_suspended_window_internal(time_t current_time);
static void eco_mode_restore_saved_state_internal(const eco_mode_saved_state *saved_state);
static bool eco_mode_capture_current_actions(pivot_actions *actions_out);

void eco_mode_start(eco_mode_config current_eco_mode);
void eco_mode_stop(void);
void eco_mode_register_callback(const app_callback callback);
void eco_mode_suspend_current_window(bool notify_override);

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
    time_t window_start = 0;
    time_t window_end = 0;

    return eco_mode_get_window_bounds_internal(current_time, &window_start, &window_end);
}

/**
 * @brief Resolves the active window absolute bounds for the current timestamp.
 *
 * @param current_time Current timestamp in seconds.
 * @param window_start Absolute start of the active window.
 * @param window_end Absolute end of the active window.
 * @return true if the current time is inside the configured window, false otherwise.
 */
static bool eco_mode_get_window_bounds_internal(time_t current_time, time_t *window_start, time_t *window_end)
{
    time_t start_time = 0;
    time_t end_time = 0;

    taskENTER_CRITICAL(&s_eco_mode_mux);
    start_time = eco_mode.start_time;
    end_time = eco_mode.end_time;
    taskEXIT_CRITICAL(&s_eco_mode_mux);

    time_t day_start = current_time - (current_time % 86400);
    time_t current_seconds = current_time % 86400;
    time_t start_seconds = start_time % 86400;
    time_t end_seconds = end_time % 86400;

    if (start_seconds == end_seconds)
    {
        return false;
    }

    if (start_seconds < end_seconds)
    {
        if (eco_mode_weekend(current_time))
        {
            return false;
        }

        if (window_start != NULL)
        {
            *window_start = day_start + start_seconds;
        }

        if (window_end != NULL)
        {
            *window_end = day_start + end_seconds;
        }

        if (current_seconds >= start_seconds && current_seconds <= end_seconds)
        {
            return true;
        }

        return false;
    }

    if (current_seconds >= start_seconds)
    {
        if (eco_mode_weekend(current_time))
        {
            return false;
        }

        if (window_start != NULL)
        {
            *window_start = day_start + start_seconds;
        }

        if (window_end != NULL)
        {
            *window_end = day_start + 86400 + end_seconds;
        }

        return true;
    }

    if (current_seconds <= end_seconds)
    {
        // The post-midnight slice belongs to the window that started yesterday.
        time_t previous_day = current_time - 86400;

        if (eco_mode_weekend(previous_day))
        {
            return false;
        }

        if (window_start != NULL)
        {
            *window_start = day_start - 86400 + start_seconds;
        }

        if (window_end != NULL)
        {
            *window_end = day_start + end_seconds;
        }

        return true;
    }

    return false;
}

/**
 * @brief Gets the current local timestamp used by Eco Mode.
 */
static time_t eco_mode_get_current_time_internal(void)
{
    return rtc_app_get_timestamp(false) + (RTC_CONFIG_TIMEZONE * 3600);
}

/**
 * @brief Updates Eco Mode runtime flags under the module lock.
 *
 * @param already_off_value New `already_off` value.
 * @param suspended_value New `eco_mode_suspended` value.
 */
static void eco_mode_set_runtime_flags_internal(bool already_off_value, bool suspended_value)
{
    taskENTER_CRITICAL(&s_eco_mode_mux);
    already_off = already_off_value;
    eco_mode_suspended = suspended_value;
    taskEXIT_CRITICAL(&s_eco_mode_mux);
}

/**
 * @brief Checks whether there is any persisted Eco Mode window state.
 *
 * @param saved_state Persisted state to be checked.
 * @return true when the current window has either a restore action or a suspension marker.
 */
static bool eco_mode_has_saved_window_internal(const eco_mode_saved_state *saved_state)
{
    return (saved_state != NULL && (saved_state->valid == true || saved_state->window_end_time != 0));
}

/**
 * @brief Checks whether the current Eco Mode window was suspended by command.
 *
 * @param saved_state Persisted state to be checked.
 * @return true when the current window must remain suspended until it ends.
 */
static bool eco_mode_window_is_suspended_internal(const eco_mode_saved_state *saved_state)
{
    return (saved_state != NULL && saved_state->valid == false && saved_state->window_end_time != 0);
}

/**
 * @brief Persists the current Eco Mode window state in RAM and NVS.
 *
 * @param saved_state State to be persisted. If NULL, a zeroed state is stored.
 */
static void eco_mode_store_saved_state_internal(const eco_mode_saved_state *saved_state)
{
    eco_mode_saved_state state_to_store = {};

    if (saved_state != NULL)
    {
        memcpy(&state_to_store, saved_state, sizeof(state_to_store));
    }

    taskENTER_CRITICAL(&s_eco_mode_mux);
    memcpy(&eco_mode_state, &state_to_store, sizeof(eco_mode_state));
    taskEXIT_CRITICAL(&s_eco_mode_mux);

    data_app_save(DATA_TYPE_ECO_MODE_STATE, &state_to_store, sizeof(state_to_store));
}

/**
 * @brief Clears the persisted restore state for the current Eco Mode window.
 */
static void eco_mode_clear_saved_state_internal(void)
{
    eco_mode_store_saved_state_internal(NULL);
}

/**
 * @brief Persists that the current Eco Mode window was overridden by command.
 *
 * @param current_time Current local timestamp.
 */
static void eco_mode_store_suspended_window_internal(time_t current_time)
{
    time_t window_start = 0;
    time_t window_end = 0;

    if (eco_mode_get_window_bounds_internal(current_time, &window_start, &window_end))
    {
        eco_mode_saved_state suspended_state = {
            .valid = false,
            .window_start_time = window_start,
            .window_end_time = window_end,
        };

        eco_mode_store_saved_state_internal(&suspended_state);
    }
    else
    {
        eco_mode_clear_saved_state_internal();
    }
}

/**
 * @brief Restores the persisted pivot state after the Eco Mode window ends.
 *
 * @param saved_state Restore state captured before the Eco Mode shutdown.
 */
static void eco_mode_restore_saved_state_internal(const eco_mode_saved_state *saved_state)
{
    if (saved_state == NULL || saved_state->valid == false)
    {
        return;
    }

    data_app_save(DATA_TYPE_ACTIONS, &saved_state->actions, sizeof(saved_state->actions));
    actuation_app_set_actions(saved_state->actions, false);
}

/**
 * @brief Captures the current actionable pivot state from the live hardware reading.
 *
 * @param actions_out Output buffer for the captured actions.
 * @return true when the pivot is currently ON and the state can be restored later.
 */
static bool eco_mode_capture_current_actions(pivot_actions *actions_out)
{
    pivot_actions current_actions = {};
    pivot_actions saved_actions = {};

    if (actions_out == NULL)
    {
        return false;
    }

    actuation_app_get_actions(&current_actions, sizeof(current_actions));

    if (current_actions.power_state != PIVOT_ON)
    {
        return false;
    }

    if (current_actions.watering_state == PIVOT_PRESSURIZING)
    {
        current_actions.watering_state = PIVOT_WET;
    }

    if (current_actions.rotation != PIVOT_CW && current_actions.rotation != PIVOT_CCW)
    {
        return false;
    }

    if (current_actions.watering_state != PIVOT_DRY && current_actions.watering_state != PIVOT_WET)
    {
        return false;
    }

    if (current_actions.percentimeter > 100)
    {
        if (data_app_load(DATA_TYPE_ACTIONS, &saved_actions) == ESP_OK && saved_actions.percentimeter <= 100)
        {
            current_actions.percentimeter = saved_actions.percentimeter;
        }
        else
        {
            return false;
        }
    }

    memcpy(actions_out, &current_actions, sizeof(current_actions));
    return true;
}

/**
 * @brief Eco Mode task main loop.
 * @param arg Task argument (unused).
 */
static void eco_mode_task(void *arg)
{
    time_t current_time = 0;
    bool window_state_initialized = false;
    bool last_window_state = false;

    while (1)
    {
        bool enabled = false;

        taskENTER_CRITICAL(&s_eco_mode_mux);
        enabled = eco_mode.enable;
        taskEXIT_CRITICAL(&s_eco_mode_mux);

        if (enabled == false)
        {
            if (window_state_initialized == false || last_window_state != false)
            {
                actuation_app_set_eco_window_state(false);
                last_window_state = false;
                window_state_initialized = true;
            }
            vTaskDelay(pdMS_TO_TICKS(15000));
            continue;
        }

        current_time = eco_mode_get_current_time_internal();
        bool in_window = eco_mode_is_in_window_internal(current_time);

        if (window_state_initialized == false || last_window_state != in_window)
        {
            actuation_app_set_eco_window_state(in_window);
            last_window_state = in_window;
            window_state_initialized = true;
        }

        bool suspended = false;
        bool restore_pending = false;
        bool window_suspended = false;
        eco_mode_saved_state restore_state = {};

        taskENTER_CRITICAL(&s_eco_mode_mux);
        suspended = eco_mode_suspended;
        restore_pending = eco_mode_state.valid;
        memcpy(&restore_state, &eco_mode_state, sizeof(restore_state));
        taskEXIT_CRITICAL(&s_eco_mode_mux);

        window_suspended = eco_mode_window_is_suspended_internal(&restore_state);

        if (eco_mode_has_saved_window_internal(&restore_state))
        {
            time_t current_day = current_time / 86400;
            time_t restore_day = restore_state.window_end_time / 86400;

            if (current_day > restore_day)
            {
                eco_mode_clear_saved_state_internal();
                eco_mode_set_runtime_flags_internal(false, false);

                vTaskDelay(pdMS_TO_TICKS(15000));
                continue;
            }

            if (current_time > restore_state.window_end_time && current_day == restore_day)
            {
                eco_mode_restore_saved_state_internal(&restore_state);
                eco_mode_clear_saved_state_internal();
                eco_mode_set_runtime_flags_internal(false, false);

                vTaskDelay(pdMS_TO_TICKS(15000));
                continue;
            }
        }

        if (suspended || window_suspended)
        {
            if (!in_window)
            {
                eco_mode_clear_saved_state_internal();
                eco_mode_set_runtime_flags_internal(false, false);
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

        if (in_window && !restore_pending)
        {
            if (!local_already_off)
            {
                if (eco_mode_callback != NULL)
                {
                    pivot_actions restore_actions = {};
                    time_t window_start = 0;
                    time_t window_end = 0;

                    if (eco_mode_capture_current_actions(&restore_actions)
                    && eco_mode_get_window_bounds_internal(current_time, &window_start, &window_end))
                    {
                        eco_mode_saved_state new_state = {
                            .valid = true,
                            .window_start_time = window_start,
                            .window_end_time = window_end,
                            .actions = restore_actions,
                        };
                        eco_mode_store_saved_state_internal(&new_state);
                        eco_mode_callback("#01-eco_mode-002-000-eco_mode$", comm_main_mode);
                        eco_mode_set_runtime_flags_internal(true, false);
                    }
                }
            }
        }
        else if (local_already_off)
        {
            eco_mode_set_runtime_flags_internal(false, suspended);
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
    if (current_eco_mode.enable == false || current_eco_mode.start_time == current_eco_mode.end_time)
    {
        eco_mode_stop();
    }
    else
    {
        eco_mode_saved_state saved_state = {};

        taskENTER_CRITICAL(&s_eco_mode_mux);
        memcpy(&eco_mode, &current_eco_mode, sizeof(eco_mode));
        taskEXIT_CRITICAL(&s_eco_mode_mux);

        if (data_app_load(DATA_TYPE_ECO_MODE_STATE, &saved_state) == ESP_OK)
        {
            if (eco_mode_has_saved_window_internal(&saved_state))
            {
                eco_mode_store_saved_state_internal(&saved_state);
                eco_mode_set_runtime_flags_internal(saved_state.valid, eco_mode_window_is_suspended_internal(&saved_state));
            }
            else
            {
                eco_mode_clear_saved_state_internal();
                eco_mode_set_runtime_flags_internal(false, false);
            }
        }
        else
        {
            eco_mode_clear_saved_state_internal();
            eco_mode_set_runtime_flags_internal(false, false);
        }

        if (current_eco_mode.enable == false)
        {
            eco_mode_clear_saved_state_internal();
            eco_mode_set_runtime_flags_internal(false, false);
        }

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
    actuation_app_set_eco_window_state(false);
    eco_mode_clear_saved_state_internal();

    if (xTask_eco_mode != NULL)
    {
        vTaskDelete(xTask_eco_mode);
        xTask_eco_mode = NULL;
        ESP_LOGE(ECO_MODE_TAG, "Eco Mode stopped");
    }

    eco_mode_set_runtime_flags_internal(false, false);
}

/**
 * @brief Suspends Eco Mode actions on command without stopping the task.
 */
void eco_mode_cmd_stop(void)
{
    eco_mode_suspend_current_window(true);
}

/**
 * @brief Suspends Eco Mode for the current window and clears any persisted restore state.
 *
 * @param notify_override True to emit the rush override notification.
 */
void eco_mode_suspend_current_window(bool notify_override)
{
    bool local_already_off = false;
    bool local_saved_state = false;
    eco_mode_saved_state local_state = {};
    time_t current_time = eco_mode_get_current_time_internal();
    bool in_window = eco_mode_is_in_window_internal(current_time);

    taskENTER_CRITICAL(&s_eco_mode_mux);
    local_already_off = already_off;
    memcpy(&local_state, &eco_mode_state, sizeof(local_state));
    taskEXIT_CRITICAL(&s_eco_mode_mux);

    local_saved_state = eco_mode_has_saved_window_internal(&local_state);

    if (xTask_eco_mode != NULL && (local_already_off == true || local_saved_state == true || in_window))
    {
        if (in_window)
        {
            eco_mode_store_suspended_window_internal(current_time);
            eco_mode_set_runtime_flags_internal(false, true);
        }
        else
        {
            eco_mode_clear_saved_state_internal();
            eco_mode_set_runtime_flags_internal(false, false);
        }

        ESP_LOGE(ECO_MODE_TAG, "Eco Mode suspended for current window");
        if (notify_override && in_window && eco_mode_callback != NULL)
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
