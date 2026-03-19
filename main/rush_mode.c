/**
 * @file rush_mode.c
 * @brief Implementation of the Rush Mode module.
 * @author soil-dev
 * @date 15 de out. de 2023
 */

#include "rush_mode.h"
#include "actuation_app.h"
#include "data_app.h"
#include "rtc_app.h"
#include "FreeRTOS_defines.h"
#include "log.h"

#include <stdio.h>
#include <string.h>

/**
 * @brief Tag used for Rush Mode log messages.
 */
#define RUSH_MODE_TAG "rush_mode"

/**
 * @brief Maximum delay, in seconds, allowed to restore the pivot after a Rush Mode window ends.
 */
#define RUSH_MODE_RESTORE_TIMEOUT_SEC (1800)

/**
 * @brief Indicates whether Rush Mode is currently enabled.
 */
static bool rush_mode_enabled = false;

/**
 * @brief Tracks if Rush Mode has already turned off the pivot.
 */
static bool already_off = false;

/**
 * @brief Indicates if Rush Mode was temporarily suspended by command.
 */
static bool rush_mode_suspended = false;

/**
 * @brief Task handle for the Rush Mode task.
 */
static TaskHandle_t xTask_rush_mode = NULL;

/**
 * @brief Current Rush Mode configuration.
 */
static rush_mode_config rush_mode = {};

/**
 * @brief Callback function for Rush Mode events.
 */
static app_callback rush_mode_callback = NULL;

/**
 * @brief Persisted state for the current Rush Mode window.
 */
static rush_mode_saved_state rush_mode_state = {};

static portMUX_TYPE s_rush_mode_mux = portMUX_INITIALIZER_UNLOCKED;

/**
 * @brief Rush Mode task implementation.
 * @param arg Task argument (unused).
 */
static void rush_mode_task(void *arg);
static bool rush_mode_get_window_bounds_internal(time_t current_time, time_t *window_start, time_t *window_end);
static time_t rush_mode_get_current_time_internal(void);
static void rush_mode_set_runtime_flags_internal(bool already_off_value, bool suspended_value);
static bool rush_mode_has_saved_window_internal(const rush_mode_saved_state *saved_state);
static bool rush_mode_window_is_suspended_internal(const rush_mode_saved_state *saved_state);
static void rush_mode_store_saved_state_internal(const rush_mode_saved_state *saved_state);
static void rush_mode_clear_saved_state_internal(void);
static void rush_mode_store_suspended_window_internal(time_t current_time);
static void rush_mode_restore_saved_state_internal(const rush_mode_saved_state *saved_state);
static bool rush_mode_capture_current_actions(pivot_actions *actions_out);

/**
 * @brief Checks if a given timestamp falls on a weekend.
 * @param ts Timestamp in seconds.
 * @return true if Saturday or Sunday, false otherwise.
 */
static bool rush_mode_weekend(time_t ts)
{
    int days = ts / 86400;
    int weekday = (days + 4) % 7;
    return (weekday == 0 || weekday == 6);
}

/**
 * @brief Checks if a time is inside the configured Rush Mode window.
 * @param current_time Current timestamp in seconds.
 * @return true if inside the Rush Mode window, false otherwise.
 */
static bool rush_mode_is_in_window_internal(time_t current_time)
{
    time_t window_start = 0;
    time_t window_end = 0;

    return rush_mode_get_window_bounds_internal(current_time, &window_start, &window_end);
}

/**
 * @brief Resolves the active window absolute bounds for the current timestamp.
 *
 * @param current_time Current timestamp in seconds.
 * @param window_start Absolute start of the active window.
 * @param window_end Absolute end of the active window.
 * @return true if the current time is inside the configured window, false otherwise.
 */
static bool rush_mode_get_window_bounds_internal(time_t current_time, time_t *window_start, time_t *window_end)
{
    time_t start_time = 0;
    time_t end_time = 0;

    taskENTER_CRITICAL(&s_rush_mode_mux);
    start_time = rush_mode.start_time;
    end_time = rush_mode.end_time;
    taskEXIT_CRITICAL(&s_rush_mode_mux);

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
        if (rush_mode_weekend(current_time))
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
        if (rush_mode_weekend(current_time))
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

        if (rush_mode_weekend(previous_day))
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
 * @brief Gets the current local timestamp used by Rush Mode.
 */
static time_t rush_mode_get_current_time_internal(void)
{
    return rtc_app_get_timestamp(false) + (RTC_CONFIG_TIMEZONE * 3600);
}

/**
 * @brief Updates Rush Mode runtime flags under the module lock.
 *
 * @param already_off_value New `already_off` value.
 * @param suspended_value New `rush_mode_suspended` value.
 */
static void rush_mode_set_runtime_flags_internal(bool already_off_value, bool suspended_value)
{
    taskENTER_CRITICAL(&s_rush_mode_mux);
    already_off = already_off_value;
    rush_mode_suspended = suspended_value;
    taskEXIT_CRITICAL(&s_rush_mode_mux);
}

/**
 * @brief Checks whether there is any persisted Rush Mode window state.
 *
 * @param saved_state Persisted state to be checked.
 * @return true when the current window has either a restore action or a suspension marker.
 */
static bool rush_mode_has_saved_window_internal(const rush_mode_saved_state *saved_state)
{
    return (saved_state != NULL && (saved_state->valid == true || saved_state->window_end_time != 0));
}

/**
 * @brief Checks whether the current Rush Mode window was suspended by command.
 *
 * @param saved_state Persisted state to be checked.
 * @return true when the current window must remain suspended until it ends.
 */
static bool rush_mode_window_is_suspended_internal(const rush_mode_saved_state *saved_state)
{
    return (saved_state != NULL && saved_state->valid == false && saved_state->window_end_time != 0);
}

/**
 * @brief Persists the current Rush Mode window state in RAM and NVS.
 *
 * @param saved_state State to be persisted. If NULL, a zeroed state is stored.
 */
static void rush_mode_store_saved_state_internal(const rush_mode_saved_state *saved_state)
{
    rush_mode_saved_state state_to_store = {};

    if (saved_state != NULL)
    {
        memcpy(&state_to_store, saved_state, sizeof(state_to_store));
    }

    taskENTER_CRITICAL(&s_rush_mode_mux);
    memcpy(&rush_mode_state, &state_to_store, sizeof(rush_mode_state));
    taskEXIT_CRITICAL(&s_rush_mode_mux);

    data_app_save(DATA_TYPE_RUSH_MODE_STATE, &state_to_store, sizeof(state_to_store));
}

/**
 * @brief Clears the persisted restore state for the current Rush Mode window.
 */
static void rush_mode_clear_saved_state_internal(void)
{
    rush_mode_store_saved_state_internal(NULL);
}

/**
 * @brief Persists that the current Rush Mode window was overridden by command.
 *
 * @param current_time Current local timestamp.
 */
static void rush_mode_store_suspended_window_internal(time_t current_time)
{
    time_t window_start = 0;
    time_t window_end = 0;

    if (rush_mode_get_window_bounds_internal(current_time, &window_start, &window_end))
    {
        rush_mode_saved_state suspended_state = {
            .valid = false,
            .window_start_time = window_start,
            .window_end_time = window_end,
        };

        rush_mode_store_saved_state_internal(&suspended_state);
    }
    else
    {
        rush_mode_clear_saved_state_internal();
    }
}

/**
 * @brief Restores the persisted pivot state after the Rush Mode window ends.
 *
 * @param saved_state Restore state captured before the Rush Mode shutdown.
 */
static void rush_mode_restore_saved_state_internal(const rush_mode_saved_state *saved_state)
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
static bool rush_mode_capture_current_actions(pivot_actions *actions_out)
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
 * @brief Rush Mode task main loop.
 * @param arg Task argument (unused).
 */
static void rush_mode_task(void *arg)
{
    time_t current_time = 0;
    bool window_state_initialized = false;
    bool last_window_state = false;

    while (1)
    {
        bool enabled = false;

        taskENTER_CRITICAL(&s_rush_mode_mux);
        enabled = rush_mode.enable;
        taskEXIT_CRITICAL(&s_rush_mode_mux);

        if (enabled == false)
        {
            if (window_state_initialized == false || last_window_state != false)
            {
                actuation_app_set_rush_mode_window_state(false);
                last_window_state = false;
                window_state_initialized = true;
            }
            vTaskDelay(pdMS_TO_TICKS(15000));
            continue;
        }

        current_time = rush_mode_get_current_time_internal();
        bool in_window = rush_mode_is_in_window_internal(current_time);

        if (window_state_initialized == false || last_window_state != in_window)
        {
            actuation_app_set_rush_mode_window_state(in_window);
            last_window_state = in_window;
            window_state_initialized = true;
        }

        bool suspended = false;
        bool restore_pending = false;
        bool window_suspended = false;
        rush_mode_saved_state restore_state = {};

        taskENTER_CRITICAL(&s_rush_mode_mux);
        suspended = rush_mode_suspended;
        restore_pending = rush_mode_state.valid;
        memcpy(&restore_state, &rush_mode_state, sizeof(restore_state));
        taskEXIT_CRITICAL(&s_rush_mode_mux);

        window_suspended = rush_mode_window_is_suspended_internal(&restore_state);

        if (rush_mode_has_saved_window_internal(&restore_state) && current_time > restore_state.window_end_time)
        {
            time_t restore_delay = current_time - restore_state.window_end_time;

            if (restore_delay <= RUSH_MODE_RESTORE_TIMEOUT_SEC)
            {
                rush_mode_restore_saved_state_internal(&restore_state);
            }

            rush_mode_clear_saved_state_internal();
            rush_mode_set_runtime_flags_internal(false, false);

            vTaskDelay(pdMS_TO_TICKS(15000));
            continue;
        }

        if (suspended || window_suspended)
        {
            if (!in_window)
            {
                rush_mode_clear_saved_state_internal();
                rush_mode_set_runtime_flags_internal(false, false);
            }
            else
            {
                vTaskDelay(pdMS_TO_TICKS(15000));
                continue;
            }
        }

        bool local_already_off = false;

        taskENTER_CRITICAL(&s_rush_mode_mux);
        local_already_off = already_off;
        taskEXIT_CRITICAL(&s_rush_mode_mux);

        if (in_window && !restore_pending)
        {
            if (!local_already_off)
            {
                if (rush_mode_callback != NULL)
                {
                    pivot_actions restore_actions = {};
                    time_t window_start = 0;
                    time_t window_end = 0;

                    if (rush_mode_capture_current_actions(&restore_actions)
                    && rush_mode_get_window_bounds_internal(current_time, &window_start, &window_end))
                    {
                        rush_mode_saved_state new_state = {
                            .valid = true,
                            .window_start_time = window_start,
                            .window_end_time = window_end,
                            .actions = restore_actions,
                        };
                        rush_mode_store_saved_state_internal(&new_state);
                        rush_mode_callback("#01-rush_mode-002-000-rush_mode$", comm_main_mode);
                        rush_mode_set_runtime_flags_internal(true, false);
                    }
                }
            }
        }
        else if (local_already_off)
        {
            rush_mode_set_runtime_flags_internal(false, suspended);
        }

        vTaskDelay(pdMS_TO_TICKS(15000)); // 15 seconds
    }
}

/**
 * @brief Starts Rush Mode with the provided configuration.
 * @param current_rush_mode Rush Mode configuration.
 */
void rush_mode_start(rush_mode_config current_rush_mode)
{
    if (current_rush_mode.enable == false || current_rush_mode.start_time == current_rush_mode.end_time)
    {
        rush_mode_stop();
    }
    else
    {
        rush_mode_saved_state saved_state = {};

        taskENTER_CRITICAL(&s_rush_mode_mux);
        memcpy(&rush_mode, &current_rush_mode, sizeof(rush_mode));
        taskEXIT_CRITICAL(&s_rush_mode_mux);

        if (data_app_load(DATA_TYPE_RUSH_MODE_STATE, &saved_state) == ESP_OK)
        {
            if (rush_mode_has_saved_window_internal(&saved_state))
            {
                rush_mode_store_saved_state_internal(&saved_state);
                rush_mode_set_runtime_flags_internal(saved_state.valid, rush_mode_window_is_suspended_internal(&saved_state));
            }
            else
            {
                rush_mode_clear_saved_state_internal();
                rush_mode_set_runtime_flags_internal(false, false);
            }
        }
        else
        {
            rush_mode_clear_saved_state_internal();
            rush_mode_set_runtime_flags_internal(false, false);
        }

        if (current_rush_mode.enable == false)
        {
            rush_mode_clear_saved_state_internal();
            rush_mode_set_runtime_flags_internal(false, false);
        }

        if (xTask_rush_mode == NULL)
        {
            BaseType_t ok = xTaskCreate(&rush_mode_task,
                                       RUSH_MODE_TASK_NAME,
                                       RUSH_MODE_TASK_SIZE,
                                       NULL,
                                       RUSH_MODE_TASK_PRIORITY,
                                       &xTask_rush_mode);
            if (ok != pdPASS)
            {
                xTask_rush_mode = NULL;
                ESP_LOGE(RUSH_MODE_TAG, "Rush Mode task creation failed");
            }
        }

        rush_mode_enabled = rush_mode.enable;

        if (rush_mode_enabled)
        {
            ESP_LOGI(RUSH_MODE_TAG, "Rush Mode enabled");
            ESP_LOGI(RUSH_MODE_TAG, "Rush Mode started: Start Time: %lld, End Time: %lld", rush_mode.start_time, rush_mode.end_time);
        }
        else
        {
            ESP_LOGE(RUSH_MODE_TAG, "Rush Mode disabled");
        }
    }
}

/**
 * @brief Stops Rush Mode and resets its state.
 */
void rush_mode_stop(void)
{
    actuation_app_set_rush_mode_window_state(false);
    rush_mode_clear_saved_state_internal();

    if (xTask_rush_mode != NULL)
    {
        vTaskDelete(xTask_rush_mode);
        xTask_rush_mode = NULL;
        ESP_LOGE(RUSH_MODE_TAG, "Rush Mode stopped");
    }

    rush_mode_set_runtime_flags_internal(false, false);
}

/**
 * @brief Suspends Rush Mode for the current window and clears any persisted restore state.
 *
 * @param notify_override True to emit the rush override notification.
 */
void rush_mode_suspend_current_window(bool notify_override)
{
    bool local_already_off = false;
    bool local_saved_state = false;
    rush_mode_saved_state local_state = {};
    time_t current_time = rush_mode_get_current_time_internal();
    bool in_window = rush_mode_is_in_window_internal(current_time);

    taskENTER_CRITICAL(&s_rush_mode_mux);
    local_already_off = already_off;
    memcpy(&local_state, &rush_mode_state, sizeof(local_state));
    taskEXIT_CRITICAL(&s_rush_mode_mux);

    local_saved_state = rush_mode_has_saved_window_internal(&local_state);

    if (xTask_rush_mode != NULL && (local_already_off == true || local_saved_state == true || in_window))
    {
        if (in_window)
        {
            rush_mode_store_suspended_window_internal(current_time);
            rush_mode_set_runtime_flags_internal(false, true);
        }
        else
        {
            rush_mode_clear_saved_state_internal();
            rush_mode_set_runtime_flags_internal(false, false);
        }

        ESP_LOGE(RUSH_MODE_TAG, "Rush Mode suspended for current window");
        if (notify_override && in_window && rush_mode_callback != NULL)
        {
            rush_mode_callback("#32-rush_mode_deactivated$", comm_main_mode);
        }
    }
}

/**
 * @brief Handles a higher-priority command that may override the current Rush Mode window.
 *
 * @param new_actions Actions requested by the current command.
 * @param old_actions Actions applied before the current command.
 * @param ignore_override True when the command itself was generated by Rush Mode.
 * @param override_on_any_idp_01 True to give priority to any external IDP 01 command.
 */
void rush_mode_handle_override(const pivot_actions *new_actions, const pivot_actions *old_actions, bool ignore_override, bool override_on_any_idp_01)
{
    if (ignore_override == false && new_actions != NULL && old_actions != NULL
    && (override_on_any_idp_01 == true || new_actions->power_state != PIVOT_OFF))
    {
        bool notify_override = ((new_actions->power_state != PIVOT_OFF) && (old_actions->power_state == PIVOT_OFF));
        rush_mode_suspend_current_window(notify_override);
    }
}

/**
 * @brief Converts an HHMM value from IDP 04 into seconds since midnight.
 *
 * @param hhmm_value Time in HHMM format.
 * @param seconds_out Output buffer for the converted value in seconds.
 * @return true when the value is a valid HHMM time, false otherwise.
 */
bool rush_mode_hhmm_to_seconds(uint32_t hhmm_value, time_t *seconds_out)
{
    uint32_t hours = 0;
    uint32_t minutes = 0;

    if (seconds_out == NULL)
    {
        return false;
    }

    hours = (hhmm_value / 100);
    minutes = (hhmm_value % 100);

    if (hours > 23 || minutes > 59)
    {
        return false;
    }

    *seconds_out = (time_t)((hours * 3600) + (minutes * 60));
    return true;
}

/**
 * @brief Formats seconds since midnight into a zero-padded HHMM string.
 *
 * @param seconds_value Time value stored internally in seconds.
 * @param hhmm_out Output buffer for the HHMM string.
 * @param hhmm_out_size Output buffer size.
 */
void rush_mode_seconds_to_hhmm_string(time_t seconds_value, char *hhmm_out, size_t hhmm_out_size)
{
    uint32_t seconds_of_day = 0;
    uint32_t hours = 0;
    uint32_t minutes = 0;

    if (hhmm_out == NULL || hhmm_out_size == 0)
    {
        return;
    }

    seconds_of_day = (uint32_t)(seconds_value % 86400);
    hours = (seconds_of_day / 3600);
    minutes = ((seconds_of_day % 3600) / 60);

    snprintf(hhmm_out, hhmm_out_size, "%02lu%02lu", (unsigned long)hours, (unsigned long)minutes);
}

/**
 * @brief Registers a callback for Rush Mode events.
 * @param callback Callback to be registered.
 */
void rush_mode_register_callback(const app_callback callback)
{
    if (callback != NULL)
    {
        ESP_LOGE(RUSH_MODE_TAG, "Rush Mode callback registered");
        taskENTER_CRITICAL(&s_rush_mode_mux);
        rush_mode_callback = callback;
        taskEXIT_CRITICAL(&s_rush_mode_mux);
    }
}
