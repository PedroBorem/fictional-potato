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

static TaskHandle_t actuation_app_task_handle = NULL;
static app_callback actuation_app_callback = NULL;
static actuation_actions actuation_app_last_actions = {};
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
    .status_active_level = true,
};

const actuation_actions actuation_actions_none = {};

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

static uint32_t actuation_app_get_read_delay_ms(void)
{
    uint8_t read_time_sec = actuation_app_config.read_time_sec;

    if (read_time_sec == 0)
    {
        read_time_sec = CONFIG_ACTUATION_DEFAULT_READ_TIME_SEC;
    }

    return (uint32_t)read_time_sec * 1000U;
}

static void actuation_app_task(void *arg)
{
    UNUSED(arg);

    while (1)
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

        vTaskDelay(pdMS_TO_TICKS(actuation_app_get_read_delay_ms()));
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

    if (actuation_app_command_is_valid(&actions) != true)
    {
        ESP_LOGE(ACTUATION_APP_TAG, "Invalid actuation command");
        return;
    }

    actuation_app_last_actions = actions;

    if (gpio_actuator_set(actions) == ESP_OK)
    {
        data_app_save(DATA_TYPE_ACTUATION_ACTIONS, &actuation_app_last_actions, sizeof(actuation_app_last_actions));
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
