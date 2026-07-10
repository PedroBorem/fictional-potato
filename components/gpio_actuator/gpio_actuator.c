/**
 * @file gpio_actuator.c
 * @brief GPIO actuator driver for four independent ON/OFF relay pairs.
 */

#include "gpio_actuator.h"

#include "FreeRTOS_defines.h"
#include "log.h"

/**
 * @brief Tag used for logging in the GPIO actuator module.
 */
#define GPIO_ACT_TAG "gpio_actuator"

static const gpio_num_t gpio_act_on_pins[CONFIG_ACTUATION_CHANNEL_COUNT] = {
    GPIO_ACT_PIN_CHANNEL_1_ON,
    GPIO_ACT_PIN_CHANNEL_2_ON,
    GPIO_ACT_PIN_CHANNEL_3_ON,
    GPIO_ACT_PIN_CHANNEL_4_ON,
};

static const gpio_num_t gpio_act_off_pins[CONFIG_ACTUATION_CHANNEL_COUNT] = {
    GPIO_ACT_PIN_CHANNEL_1_OFF,
    GPIO_ACT_PIN_CHANNEL_2_OFF,
    GPIO_ACT_PIN_CHANNEL_3_OFF,
    GPIO_ACT_PIN_CHANNEL_4_OFF,
};

static const gpio_num_t gpio_act_status_pins[CONFIG_ACTUATION_CHANNEL_COUNT] = {
    GPIO_ACT_PIN_CHANNEL_1_STATUS,
    GPIO_ACT_PIN_CHANNEL_2_STATUS,
    GPIO_ACT_PIN_CHANNEL_3_STATUS,
    GPIO_ACT_PIN_CHANNEL_4_STATUS,
};

static actuation_config gpio_act_config = {
    .relay_pulse_time_ms = CONFIG_ACTUATION_DEFAULT_RELAY_PULSE_MS,
    .idle_read_time_sec = CONFIG_ACTUATION_DEFAULT_IDLE_READ_TIME_SEC,
    .status_active_level = CONFIG_ACTUATION_DEFAULT_STATUS_ACTIVE_LEVEL,
    .ramp_1_delay_sec = CONFIG_PUMP_RAMP_1_DELAY_MS / 1000U,
    .stage_1_delay_sec = CONFIG_PUMP_STAGE_1_DELAY_MS / 1000U,
    .ramp_2_delay_sec = CONFIG_PUMP_RAMP_2_DELAY_MS / 1000U,
    .stage_2_delay_sec = CONFIG_PUMP_STAGE_2_DELAY_MS / 1000U,
    .ramp_3_delay_sec = CONFIG_PUMP_RAMP_3_DELAY_MS / 1000U,
    .stage_3_delay_sec = CONFIG_PUMP_STAGE_3_DELAY_MS / 1000U,
    .ramp_4_delay_sec = CONFIG_PUMP_RAMP_4_DELAY_MS / 1000U,
    .stage_4_delay_sec = CONFIG_PUMP_STAGE_4_DELAY_MS / 1000U,
    .status_publish_time_min = CONFIG_ACTUATION_DEFAULT_STATUS_PUBLISH_MIN,
};

static void gpio_act_set_all_relays(uint32_t level)
{
    for (uint8_t channel = 0; channel < CONFIG_ACTUATION_CHANNEL_COUNT; channel++)
    {
        gpio_set_level(gpio_act_on_pins[channel], level);
        gpio_set_level(gpio_act_off_pins[channel], level);
    }
}

static void gpio_act_set_all_on_relays(uint32_t level)
{
    for (uint8_t channel = 0; channel < CONFIG_ACTUATION_CHANNEL_COUNT; channel++)
    {
        gpio_set_level(gpio_act_on_pins[channel], level);
    }
}

esp_err_t gpio_actuator_init(void)
{
    gpio_config_t io_conf_out = {};
    io_conf_out.intr_type = GPIO_INTR_DISABLE;
    io_conf_out.mode = GPIO_MODE_OUTPUT_OD;
    io_conf_out.pin_bit_mask = GPIO_ACT_OUTPUT_PIN_GROUP;
    io_conf_out.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf_out.pull_up_en = GPIO_PULLUP_DISABLE;

    esp_err_t err = gpio_config(&io_conf_out);
    if (err != ESP_OK)
    {
        LOG_ERROR(GPIO_ACT_TAG, "GPIO", "%s, failed to configure relay outputs: %d", __func__, err);
        return err;
    }

    gpio_act_set_all_relays(GPIO_ACT_RELAY_DISABLE);

    gpio_config_t io_conf_in = {};
    io_conf_in.intr_type = GPIO_INTR_DISABLE;
    io_conf_in.mode = GPIO_MODE_INPUT;
    io_conf_in.pin_bit_mask = GPIO_ACT_INPUT_PIN_GROUP;
    io_conf_in.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf_in.pull_up_en = GPIO_PULLUP_DISABLE;

    err = gpio_config(&io_conf_in);
    if (err != ESP_OK)
    {
        LOG_ERROR(GPIO_ACT_TAG, "GPIO", "%s, failed to configure status inputs: %d", __func__, err);
        return err;
    }

    LOG_SUCCESS(GPIO_ACT_TAG, "GPIO", "GPIO actuator initialized with 4 ON/OFF channels");
    return ESP_OK;
}

esp_err_t gpio_actuator_config(actuation_config config)
{
    if (config.relay_pulse_time_ms == 0)
    {
        config.relay_pulse_time_ms = CONFIG_ACTUATION_DEFAULT_RELAY_PULSE_MS;
    }

    if (config.idle_read_time_sec == 0)
    {
        config.idle_read_time_sec = CONFIG_ACTUATION_DEFAULT_IDLE_READ_TIME_SEC;
    }

    if (config.ramp_1_delay_sec == 0)
    {
        config.ramp_1_delay_sec = CONFIG_PUMP_RAMP_1_DELAY_MS / 1000U;
    }

    if (config.stage_1_delay_sec == 0)
    {
        config.stage_1_delay_sec = CONFIG_PUMP_STAGE_1_DELAY_MS / 1000U;
    }

    if (config.ramp_2_delay_sec == 0)
    {
        config.ramp_2_delay_sec = CONFIG_PUMP_RAMP_2_DELAY_MS / 1000U;
    }

    if (config.stage_2_delay_sec == 0)
    {
        config.stage_2_delay_sec = CONFIG_PUMP_STAGE_2_DELAY_MS / 1000U;
    }

    if (config.ramp_3_delay_sec == 0)
    {
        config.ramp_3_delay_sec = CONFIG_PUMP_RAMP_3_DELAY_MS / 1000U;
    }

    if (config.stage_3_delay_sec == 0)
    {
        config.stage_3_delay_sec = CONFIG_PUMP_STAGE_3_DELAY_MS / 1000U;
    }

    if (config.ramp_4_delay_sec == 0)
    {
        config.ramp_4_delay_sec = CONFIG_PUMP_RAMP_4_DELAY_MS / 1000U;
    }

    if (config.status_publish_time_min == 0)
    {
        config.status_publish_time_min = CONFIG_ACTUATION_DEFAULT_STATUS_PUBLISH_MIN;
    }

    gpio_act_config = config;
    return ESP_OK;
}

esp_err_t gpio_actuator_set(actuation_actions actions)
{
    for (uint8_t channel = 0; channel < CONFIG_ACTUATION_CHANNEL_COUNT; channel++)
    {
        switch (actions.channels[channel])
        {
            case ACTUATION_COMMAND_ON:
            {
                gpio_set_level(gpio_act_off_pins[channel], GPIO_ACT_RELAY_DISABLE);
                gpio_set_level(gpio_act_on_pins[channel], GPIO_ACT_RELAY_ENABLE);
                LOG_PROCESSING(GPIO_ACT_TAG, "Channel %u ON relay enabled", (unsigned int)(channel + 1));
                break;
            }

            case ACTUATION_COMMAND_OFF:
            {
                gpio_set_level(gpio_act_on_pins[channel], GPIO_ACT_RELAY_DISABLE);
                gpio_set_level(gpio_act_off_pins[channel], GPIO_ACT_RELAY_DISABLE);
                LOG_PROCESSING(GPIO_ACT_TAG,
                               "Channel %u ON relay disabled (one-wire stop)",
                               (unsigned int)(channel + 1));
                break;
            }

            case ACTUATION_COMMAND_NONE:
            default:
            {
                break;
            }
        }
    }

    return ESP_OK;
}

esp_err_t gpio_actuator_enable_on_relay(uint8_t channel)
{
    if (channel >= CONFIG_ACTUATION_CHANNEL_COUNT)
    {
        return ESP_ERR_INVALID_ARG;
    }

    gpio_set_level(gpio_act_off_pins[channel], GPIO_ACT_RELAY_DISABLE);
    gpio_set_level(gpio_act_on_pins[channel], GPIO_ACT_RELAY_ENABLE);

    LOG_PROCESSING(GPIO_ACT_TAG, "Channel %u ON relay enabled", (unsigned int)(channel + 1));
    return ESP_OK;
}

esp_err_t gpio_actuator_disable_on_relay(uint8_t channel)
{
    if (channel >= CONFIG_ACTUATION_CHANNEL_COUNT)
    {
        return ESP_ERR_INVALID_ARG;
    }

    gpio_set_level(gpio_act_on_pins[channel], GPIO_ACT_RELAY_DISABLE);
    gpio_set_level(gpio_act_off_pins[channel], GPIO_ACT_RELAY_DISABLE);

    LOG_PROCESSING(GPIO_ACT_TAG,
                   "Channel %u ON relay disabled (one-wire stop)",
                   (unsigned int)(channel + 1));
    return ESP_OK;
}

void gpio_actuator_disable_all_on_relays(void)
{
    gpio_act_set_all_on_relays(GPIO_ACT_RELAY_DISABLE);
}

actuation_status gpio_actuator_get(void)
{
    actuation_status status = {};
    int active_level = gpio_act_config.status_active_level ? 1 : 0;

    for (uint8_t channel = 0; channel < CONFIG_ACTUATION_CHANNEL_COUNT; channel++)
    {
        status.channels[channel] = (gpio_get_level(gpio_act_status_pins[channel]) == active_level)
            ? ACTUATION_STATUS_ON
            : ACTUATION_STATUS_OFF;
    }

    return status;
}

void gpio_actuator_get_status(actuation_status *status_out)
{
    if (status_out == NULL)
    {
        return;
    }

    *status_out = gpio_actuator_get();
}

void gpio_actuator_shutdown(void)
{
    gpio_act_set_all_relays(GPIO_ACT_RELAY_DISABLE);
}
