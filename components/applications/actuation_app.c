/**
 * @file actuation_app.c
 * @brief Actuation application for four independent ON/OFF channels.
 */

#include "actuation_app.h"

#include "data_app.h"
#include "gpio_actuator.h"

#include "FreeRTOS_defines.h"
#include "log.h"

#include <string.h>

/**
 * @brief Tag used for logging within the actuation_app module.
 */
#define ACTUATION_APP_TAG "actuation_app"

typedef enum
{
    ACTUATION_PUMP_COMMAND_NONE = 0,
    ACTUATION_PUMP_COMMAND_START,
    ACTUATION_PUMP_COMMAND_STOP,
} actuation_pump_command;

typedef enum
{
    ACTUATION_PUMP_STATE_STOPPED = 0,
    ACTUATION_PUMP_STATE_STARTING,
    ACTUATION_PUMP_STATE_RUNNING,
    ACTUATION_PUMP_STATE_STOPPING,
    ACTUATION_PUMP_STATE_FAULT,
} actuation_pump_state;

typedef enum
{
    ACTUATION_SEQUENCE_OK = 0,
    ACTUATION_SEQUENCE_STOP_REQUESTED,
    ACTUATION_SEQUENCE_FAULT,
} actuation_sequence_result;

static TaskHandle_t actuation_app_task_handle = NULL;
static QueueHandle_t actuation_app_command_queue = NULL;
static app_callback actuation_app_callback = NULL;
static actuation_actions actuation_app_last_actions = {};
static actuation_pump_state actuation_app_state = ACTUATION_PUMP_STATE_STOPPED;
static actuation_status actuation_app_last_status = {
    .channels = {
        ACTUATION_STATUS_UNKNOWN,
        ACTUATION_STATUS_UNKNOWN,
        ACTUATION_STATUS_UNKNOWN,
        ACTUATION_STATUS_UNKNOWN,
    },
};
static actuation_config actuation_app_config = {
    .relay_pulse_time_ms = CONFIG_ACTUATION_DEFAULT_RELAY_PULSE_MS,
    .read_time_sec = CONFIG_ACTUATION_DEFAULT_READ_TIME_SEC,
    .status_active_level = CONFIG_ACTUATION_DEFAULT_STATUS_ACTIVE_LEVEL,
};

const actuation_actions actuation_actions_none = {};

static uint32_t actuation_app_min_u32(uint32_t value_a, uint32_t value_b)
{
    return (value_a < value_b) ? value_a : value_b;
}

static bool actuation_app_command_is_valid(const actuation_actions *actions)
{
    if (actions == NULL)
    {
        return false;
    }

    for (uint8_t channel = 0; channel < CONFIG_ACTUATION_CHANNEL_COUNT; channel++)
    {
        if (actions->channels[channel] != ACTUATION_COMMAND_NONE &&
            actions->channels[channel] != ACTUATION_COMMAND_ON &&
            actions->channels[channel] != ACTUATION_COMMAND_OFF)
        {
            return false;
        }
    }

    return true;
}

static bool actuation_app_actions_request_start(const actuation_actions *actions)
{
    if (actions == NULL)
    {
        return false;
    }

    for (uint8_t channel = 0; channel < CONFIG_ACTUATION_CHANNEL_COUNT; channel++)
    {
        if (actions->channels[channel] == ACTUATION_COMMAND_ON)
        {
            return true;
        }
    }

    return false;
}

static bool actuation_app_actions_request_stop(const actuation_actions *actions)
{
    if (actions == NULL)
    {
        return false;
    }

    for (uint8_t channel = 0; channel < CONFIG_ACTUATION_CHANNEL_COUNT; channel++)
    {
        if (actions->channels[channel] == ACTUATION_COMMAND_OFF)
        {
            return true;
        }
    }

    return false;
}

static bool actuation_app_status_changed(const actuation_status *status_a, const actuation_status *status_b)
{
    return (memcmp(status_a, status_b, sizeof(actuation_status)) != 0);
}

static void actuation_app_log_status(const actuation_status *status)
{
    if (status == NULL)
    {
        return;
    }

    ESP_LOGI(ACTUATION_APP_TAG,
             "Status C1=%u C2=%u C3=%u C4=%u",
             (unsigned int)status->channels[0],
             (unsigned int)status->channels[1],
             (unsigned int)status->channels[2],
             (unsigned int)status->channels[3]);
}

static actuation_status actuation_app_read_status(void)
{
    actuation_status current_status = gpio_actuator_get();

    if (actuation_app_status_changed(&current_status, &actuation_app_last_status))
    {
        actuation_app_last_status = current_status;
        actuation_app_log_status(&actuation_app_last_status);

        if (actuation_app_callback != NULL)
        {
            actuation_app_callback("#00$", comm_main_mode);
        }
    }

    return current_status;
}

static uint32_t actuation_app_get_read_delay_ms(void)
{
    uint8_t read_time_sec = actuation_app_config.read_time_sec;

    if (read_time_sec == 0)
    {
        read_time_sec = CONFIG_ACTUATION_DEFAULT_READ_TIME_SEC;
    }

    return (uint32_t)read_time_sec * 1000U;
}

static bool actuation_app_stop_command_pending(void)
{
    actuation_pump_command command = ACTUATION_PUMP_COMMAND_NONE;

    while (actuation_app_command_queue != NULL &&
           xQueueReceive(actuation_app_command_queue, &command, 0) == pdPASS)
    {
        if (command == ACTUATION_PUMP_COMMAND_STOP)
        {
            return true;
        }
    }

    return false;
}

static actuation_sequence_result actuation_app_validate_channels(uint8_t enabled_count)
{
    actuation_status status = actuation_app_read_status();

    for (uint8_t channel = 0; channel < enabled_count && channel < CONFIG_ACTUATION_CHANNEL_COUNT; channel++)
    {
        if (status.channels[channel] != ACTUATION_STATUS_ON)
        {
            ESP_LOGE(ACTUATION_APP_TAG,
                     "Pump fault: channel %u status is not ON",
                     (unsigned int)(channel + 1));
            return ACTUATION_SEQUENCE_FAULT;
        }
    }

    return ACTUATION_SEQUENCE_OK;
}

static actuation_sequence_result actuation_app_wait_and_monitor(uint32_t wait_ms, uint8_t monitored_count)
{
    uint32_t elapsed_ms = 0;

    while (elapsed_ms < wait_ms)
    {
        uint32_t step_ms = actuation_app_min_u32(CONFIG_PUMP_MONITOR_INTERVAL_MS, wait_ms - elapsed_ms);

        if (actuation_app_stop_command_pending())
        {
            return ACTUATION_SEQUENCE_STOP_REQUESTED;
        }

        if (monitored_count > 0)
        {
            actuation_sequence_result validation_result = actuation_app_validate_channels(monitored_count);
            if (validation_result != ACTUATION_SEQUENCE_OK)
            {
                return validation_result;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(step_ms));
        elapsed_ms += step_ms;
    }

    if (actuation_app_stop_command_pending())
    {
        return ACTUATION_SEQUENCE_STOP_REQUESTED;
    }

    return ACTUATION_SEQUENCE_OK;
}

static void actuation_app_store_stop_actions(void)
{
    actuation_actions stop_actions = {};

    for (uint8_t channel = 0; channel < CONFIG_ACTUATION_CHANNEL_COUNT; channel++)
    {
        stop_actions.channels[channel] = ACTUATION_COMMAND_OFF;
    }

    actuation_app_last_actions = stop_actions;
    data_app_save(DATA_TYPE_ACTUATION_ACTIONS, &actuation_app_last_actions, sizeof(actuation_app_last_actions));
}

static void actuation_app_stop_pump(bool fault)
{
    actuation_app_state = fault ? ACTUATION_PUMP_STATE_FAULT : ACTUATION_PUMP_STATE_STOPPING;

    if (fault)
    {
        ESP_LOGE(ACTUATION_APP_TAG, "Pump fault detected. Stopping all channels");
    }
    else
    {
        ESP_LOGI(ACTUATION_APP_TAG, "Pump stop requested");
    }

    gpio_actuator_stop_all(CONFIG_PUMP_STOP_RELAY_TIME_MS);
    actuation_app_store_stop_actions();
    actuation_app_read_status();

    if (fault == false)
    {
        actuation_app_state = ACTUATION_PUMP_STATE_STOPPED;
    }
}

static bool actuation_app_handle_sequence_result(actuation_sequence_result result)
{
    if (result == ACTUATION_SEQUENCE_OK)
    {
        return true;
    }

    actuation_app_stop_pump(result == ACTUATION_SEQUENCE_FAULT);
    return false;
}

static bool actuation_app_start_channel_and_validate(uint8_t channel, uint32_t wait_ms, uint8_t monitored_count, uint8_t validate_count)
{
    if (gpio_actuator_enable_on_relay(channel) != ESP_OK)
    {
        actuation_app_stop_pump(true);
        return false;
    }

    if (actuation_app_handle_sequence_result(actuation_app_wait_and_monitor(wait_ms, monitored_count)) == false)
    {
        return false;
    }

    return actuation_app_handle_sequence_result(actuation_app_validate_channels(validate_count));
}

static void actuation_app_start_pump_sequence(void)
{
    if (actuation_app_state == ACTUATION_PUMP_STATE_STARTING ||
        actuation_app_state == ACTUATION_PUMP_STATE_RUNNING)
    {
        ESP_LOGW(ACTUATION_APP_TAG, "Pump start ignored: already active");
        return;
    }

    ESP_LOGI(ACTUATION_APP_TAG, "Pump start sequence requested");
    actuation_app_state = ACTUATION_PUMP_STATE_STARTING;

    if (actuation_app_start_channel_and_validate(0, CONFIG_PUMP_STAGE_1_DELAY_MS, 0, 1) == false)
    {
        return;
    }

    if (actuation_app_start_channel_and_validate(1, CONFIG_PUMP_STAGE_2_DELAY_MS, 1, 2) == false)
    {
        return;
    }

    if (actuation_app_start_channel_and_validate(2, CONFIG_PUMP_STAGE_3_DELAY_MS, 2, 3) == false)
    {
        return;
    }

    if (gpio_actuator_enable_on_relay(3) != ESP_OK)
    {
        actuation_app_stop_pump(true);
        return;
    }

    if (actuation_app_handle_sequence_result(actuation_app_wait_and_monitor(CONFIG_PUMP_MONITOR_INTERVAL_MS, 3)) == false)
    {
        return;
    }

    if (actuation_app_handle_sequence_result(actuation_app_validate_channels(CONFIG_ACTUATION_CHANNEL_COUNT)) == false)
    {
        return;
    }

    actuation_app_state = ACTUATION_PUMP_STATE_RUNNING;
    ESP_LOGI(ACTUATION_APP_TAG, "Pump running");

    while (actuation_app_state == ACTUATION_PUMP_STATE_RUNNING)
    {
        if (actuation_app_handle_sequence_result(
                actuation_app_wait_and_monitor(CONFIG_PUMP_MONITOR_INTERVAL_MS, CONFIG_ACTUATION_CHANNEL_COUNT)) == false)
        {
            return;
        }
    }
}

static void actuation_app_task(void *arg)
{
    UNUSED(arg);

    while (1)
    {
        actuation_pump_command command = ACTUATION_PUMP_COMMAND_NONE;

        if (actuation_app_command_queue != NULL &&
            xQueueReceive(actuation_app_command_queue, &command, pdMS_TO_TICKS(actuation_app_get_read_delay_ms())) == pdPASS)
        {
            if (command == ACTUATION_PUMP_COMMAND_START)
            {
                actuation_app_start_pump_sequence();
            }
            else if (command == ACTUATION_PUMP_COMMAND_STOP)
            {
                actuation_app_stop_pump(false);
            }
        }
        else
        {
            actuation_app_read_status();
        }
    }
}

esp_err_t actuation_app_init(const app_callback callback)
{
    esp_err_t err = gpio_actuator_init();

    if (err != ESP_OK)
    {
        return err;
    }

    actuation_app_callback = callback;
    actuation_app_last_status = gpio_actuator_get();
    actuation_app_log_status(&actuation_app_last_status);

    if (actuation_app_command_queue == NULL)
    {
        actuation_app_command_queue = xQueueCreate(4, sizeof(actuation_pump_command));
        if (actuation_app_command_queue == NULL)
        {
            ESP_LOGE(ACTUATION_APP_TAG, "%s, failed to create command queue", __func__);
            return ESP_FAIL;
        }
    }

    if (actuation_app_task_handle == NULL)
    {
        BaseType_t task_ret = xTaskCreate(&actuation_app_task,
                                          ACTUATION_APP_TASK_NAME,
                                          ACTUATION_APP_STACK_SIZE,
                                          NULL,
                                          ACTUATION_APP_TASK_PRIORITY,
                                          &actuation_app_task_handle);

        if (task_ret != pdPASS || actuation_app_task_handle == NULL)
        {
            ESP_LOGE(ACTUATION_APP_TAG, "%s, failed to create task: %s", __func__, ACTUATION_APP_TASK_NAME);
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}

esp_err_t actuation_app_set_config(actuation_config config)
{
    if (config.relay_pulse_time_ms == 0)
    {
        config.relay_pulse_time_ms = CONFIG_ACTUATION_DEFAULT_RELAY_PULSE_MS;
    }

    if (config.read_time_sec == 0)
    {
        config.read_time_sec = CONFIG_ACTUATION_DEFAULT_READ_TIME_SEC;
    }

    actuation_app_config = config;
    return gpio_actuator_config(config);
}

void actuation_app_set_actions(const actuation_actions actions, bool alert_change)
{
    UNUSED(alert_change);
    actuation_pump_command command = ACTUATION_PUMP_COMMAND_NONE;

    if (actuation_app_command_is_valid(&actions) != true)
    {
        ESP_LOGE(ACTUATION_APP_TAG, "Invalid actuation command");
        return;
    }

    actuation_app_last_actions = actions;
    data_app_save(DATA_TYPE_ACTUATION_ACTIONS, &actuation_app_last_actions, sizeof(actuation_app_last_actions));

    if (actuation_app_actions_request_stop(&actions))
    {
        command = ACTUATION_PUMP_COMMAND_STOP;
    }
    else if (actuation_app_actions_request_start(&actions))
    {
        command = ACTUATION_PUMP_COMMAND_START;
    }

    if (command != ACTUATION_PUMP_COMMAND_NONE && actuation_app_command_queue != NULL)
    {
        BaseType_t queue_ret = pdFAIL;

        if (command == ACTUATION_PUMP_COMMAND_STOP)
        {
            queue_ret = xQueueSendToFront(actuation_app_command_queue, &command, 0);
        }
        else
        {
            queue_ret = xQueueSend(actuation_app_command_queue, &command, 0);
        }

        if (queue_ret != pdPASS)
        {
            xQueueReset(actuation_app_command_queue);
            xQueueSend(actuation_app_command_queue, &command, 0);
        }
    }
}

void actuation_app_get_actions(actuation_actions *actions_out, size_t actions_size)
{
    if (actions_out == NULL || actions_size == 0)
    {
        return;
    }

    size_t copy_size = (actions_size < sizeof(actuation_app_last_actions))
        ? actions_size
        : sizeof(actuation_app_last_actions);

    memcpy(actions_out, &actuation_app_last_actions, copy_size);
}

void actuation_app_get_status(actuation_status *status_out, size_t status_size)
{
    if (status_out == NULL || status_size == 0)
    {
        return;
    }

    actuation_status current_status = gpio_actuator_get();
    size_t copy_size = (status_size < sizeof(current_status)) ? status_size : sizeof(current_status);

    memcpy(status_out, &current_status, copy_size);
}

bool actuation_app_is_all_off(void)
{
    actuation_status status = gpio_actuator_get();

    for (uint8_t channel = 0; channel < CONFIG_ACTUATION_CHANNEL_COUNT; channel++)
    {
        if (status.channels[channel] != ACTUATION_STATUS_OFF)
        {
            return false;
        }
    }

    return true;
}

void actuation_app_shutdown(void)
{
    gpio_actuator_shutdown();
}
