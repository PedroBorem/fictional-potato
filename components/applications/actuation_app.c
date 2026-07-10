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
static comm_type actuation_app_progress_mode = COMM_RF;
static actuation_actions actuation_app_last_actions = {};
static actuation_pump_state actuation_app_state = ACTUATION_PUMP_STATE_STOPPED;
static uint8_t actuation_app_last_fault_channel = 0;
static bool actuation_app_on_relay_enabled[CONFIG_ACTUATION_CHANNEL_COUNT] = {};
static uint32_t actuation_app_shutdown_sequence = 0;
static actuation_shutdown_info actuation_app_last_shutdown_info = {};
static char actuation_app_phase_context[CONFIG_ACTUATION_SHUTDOWN_TEXT_SIZE] = "stopped";
static TickType_t actuation_app_last_status_publish_tick = 0;
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

const actuation_actions actuation_actions_none = {};

static uint32_t actuation_app_min_u32(uint32_t value_a, uint32_t value_b)
{
    return (value_a < value_b) ? value_a : value_b;
}

static uint32_t actuation_app_ms_to_sec_round_up(uint32_t value_ms)
{
    return (value_ms + 999U) / 1000U;
}

static actuation_config actuation_app_sanitize_config(actuation_config config)
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

    return config;
}

static uint32_t actuation_app_sec_to_ms_u32(uint16_t value_sec)
{
    return (uint32_t)value_sec * 1000U;
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

static const char *actuation_app_state_to_phase(actuation_pump_state state)
{
    switch (state)
    {
        case ACTUATION_PUMP_STATE_STOPPED:
            return "stopped";
        case ACTUATION_PUMP_STATE_STARTING:
            return "starting";
        case ACTUATION_PUMP_STATE_RUNNING:
            return "running";
        case ACTUATION_PUMP_STATE_STOPPING:
            return "stopping";
        case ACTUATION_PUMP_STATE_FAULT:
            return "fault";
        default:
            return "unknown";
    }
}

static const char *actuation_app_state_to_packet_state(actuation_pump_state state)
{
    switch (state)
    {
        case ACTUATION_PUMP_STATE_STOPPED:
            return "STOPPED";
        case ACTUATION_PUMP_STATE_STARTING:
            return "STARTING";
        case ACTUATION_PUMP_STATE_RUNNING:
            return "RUNNING";
        case ACTUATION_PUMP_STATE_STOPPING:
            return "STOPPING";
        case ACTUATION_PUMP_STATE_FAULT:
            return "FAULT";
        default:
            return "UNKNOWN";
    }
}

static void actuation_app_set_phase_context(const char *phase)
{
    const char *state_prefix = actuation_app_state_to_phase(actuation_app_state);

    if (phase == NULL || phase[0] == '\0')
    {
        phase = "unknown";
    }

    if (actuation_app_state == ACTUATION_PUMP_STATE_STARTING)
    {
        if (strcmp(phase, "RAMP") == 0)
        {
            snprintf(actuation_app_phase_context, sizeof(actuation_app_phase_context), "%s", "starting_ramp");
            return;
        }

        if (strcmp(phase, "STAGE") == 0)
        {
            snprintf(actuation_app_phase_context, sizeof(actuation_app_phase_context), "%s", "starting_stage");
            return;
        }

        if (strcmp(phase, "ON") == 0)
        {
            snprintf(actuation_app_phase_context, sizeof(actuation_app_phase_context), "%s", "starting_on");
            return;
        }
    }

    snprintf(actuation_app_phase_context,
             sizeof(actuation_app_phase_context),
             "%s",
             state_prefix);
}

static void actuation_app_record_shutdown_event(actuation_shutdown_reason reason,
                                                uint8_t motor,
                                                const char *user,
                                                const char *phase)
{
    actuation_app_shutdown_sequence++;
    if (actuation_app_shutdown_sequence == 0)
    {
        actuation_app_shutdown_sequence = 1;
    }

    actuation_app_last_shutdown_info = (actuation_shutdown_info){
        .sequence = actuation_app_shutdown_sequence,
        .reason = (uint8_t)reason,
        .motor = motor,
    };

    if (user != NULL && user[0] != '\0')
    {
        strncpy(actuation_app_last_shutdown_info.user,
                user,
                sizeof(actuation_app_last_shutdown_info.user) - 1U);
    }
    else
    {
        strncpy(actuation_app_last_shutdown_info.user,
                "unknown",
                sizeof(actuation_app_last_shutdown_info.user) - 1U);
    }

    if (phase != NULL && phase[0] != '\0')
    {
        strncpy(actuation_app_last_shutdown_info.phase,
                phase,
                sizeof(actuation_app_last_shutdown_info.phase) - 1U);
    }
    else
    {
        strncpy(actuation_app_last_shutdown_info.phase,
                "unknown",
                sizeof(actuation_app_last_shutdown_info.phase) - 1U);
    }
}

static void actuation_app_log_status(const actuation_status *status)
{
    if (status == NULL)
    {
        return;
    }

    LOG_DEFAULT(ACTUATION_APP_TAG,
                "STATUS",
                "Status C1=%u C2=%u C3=%u C4=%u",
                (unsigned int)status->channels[0],
                (unsigned int)status->channels[1],
                (unsigned int)status->channels[2],
                (unsigned int)status->channels[3]);
}

static void actuation_app_log_timer_progress(const char *stage_name,
                                             uint32_t elapsed_ms,
                                             uint32_t wait_ms,
                                             uint8_t monitored_count)
{
    actuation_status status = gpio_actuator_get();
    uint32_t remaining_ms = (elapsed_ms < wait_ms) ? (wait_ms - elapsed_ms) : 0;

    LOG_TIMER(ACTUATION_APP_TAG,
              "%s: %lu/%lu s elapsed, %lu s remaining, monitoring first %u channel(s), status C1=%u C2=%u C3=%u C4=%u",
              stage_name,
              (unsigned long)actuation_app_ms_to_sec_round_up(elapsed_ms),
              (unsigned long)actuation_app_ms_to_sec_round_up(wait_ms),
              (unsigned long)actuation_app_ms_to_sec_round_up(remaining_ms),
              (unsigned int)monitored_count,
              (unsigned int)status.channels[0],
              (unsigned int)status.channels[1],
              (unsigned int)status.channels[2],
              (unsigned int)status.channels[3]);
}

static void actuation_app_log_timer_heartbeat(const char *stage_name,
                                              uint32_t elapsed_ms,
                                              uint32_t wait_ms)
{
    LOG_TIMER(ACTUATION_APP_TAG,
              "%s heartbeat ...... %lu/%lu s",
              stage_name,
              (unsigned long)actuation_app_ms_to_sec_round_up(elapsed_ms),
              (unsigned long)actuation_app_ms_to_sec_round_up(wait_ms));
}

static void actuation_app_publish_progress(uint8_t motor,
                                           const char *phase,
                                           uint32_t elapsed_ms,
                                           uint32_t total_ms)
{
    actuation_status status = {};
    char packet[180] = {};

    actuation_app_set_phase_context(phase);

    if (actuation_app_callback == NULL)
    {
        return;
    }

    status = gpio_actuator_get();

    snprintf(packet,
             sizeof(packet),
             "#29-%s-%s-%u-%s-%lu-%lu-%u-%u-%u-%u$",
             system_id,
             actuation_app_state_to_packet_state(actuation_app_state),
             (unsigned int)motor,
             (phase != NULL && phase[0] != '\0') ? phase : "UNKNOWN",
             (unsigned long)actuation_app_ms_to_sec_round_up(elapsed_ms),
             (unsigned long)actuation_app_ms_to_sec_round_up(total_ms),
             (unsigned int)status.channels[0],
             (unsigned int)status.channels[1],
             (unsigned int)status.channels[2],
             (unsigned int)status.channels[3]);

    actuation_app_callback(packet, actuation_app_progress_mode);
}

static uint32_t actuation_app_get_status_publish_delay_ms(void)
{
    uint16_t publish_time_min = actuation_app_config.status_publish_time_min;

    if (publish_time_min == 0)
    {
        publish_time_min = CONFIG_ACTUATION_DEFAULT_STATUS_PUBLISH_MIN;
    }

    return (uint32_t)publish_time_min * 60U * 1000U;
}

static void actuation_app_publish_status(bool force)
{
    TickType_t now_tick = xTaskGetTickCount();
    TickType_t publish_delay_ticks = pdMS_TO_TICKS(actuation_app_get_status_publish_delay_ms());

    if (actuation_app_callback == NULL)
    {
        return;
    }

    if (force ||
        actuation_app_last_status_publish_tick == 0 ||
        (now_tick - actuation_app_last_status_publish_tick) >= publish_delay_ticks)
    {
        actuation_app_last_status_publish_tick = now_tick;
        actuation_app_callback("#00$", comm_main_mode);
    }
}

static actuation_status actuation_app_read_status(void)
{
    actuation_status current_status = gpio_actuator_get();

    if (actuation_app_status_changed(&current_status, &actuation_app_last_status))
    {
        actuation_app_last_status = current_status;
        actuation_app_log_status(&actuation_app_last_status);

        actuation_app_publish_status(true);
    }

    return current_status;
}

static uint32_t actuation_app_get_read_delay_ms(void)
{
    uint16_t read_time_sec = actuation_app_config.idle_read_time_sec;
    uint32_t read_delay_ms = 0;
    uint32_t publish_delay_ms = actuation_app_get_status_publish_delay_ms();

    if (read_time_sec == 0)
    {
        read_time_sec = CONFIG_ACTUATION_DEFAULT_IDLE_READ_TIME_SEC;
    }

    read_delay_ms = (uint32_t)read_time_sec * 1000U;
    return actuation_app_min_u32(read_delay_ms, publish_delay_ms);
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
            actuation_app_last_fault_channel = channel + 1;
            LOG_ERROR(ACTUATION_APP_TAG,
                      "ACTUATION",
                      "Pump fault: channel %u status is not ON",
                      (unsigned int)(channel + 1));
            return ACTUATION_SEQUENCE_FAULT;
        }
    }

    return ACTUATION_SEQUENCE_OK;
}

static actuation_sequence_result actuation_app_wait_and_monitor(uint32_t wait_ms,
                                                               uint8_t monitored_count,
                                                               const char *stage_name,
                                                               bool log_progress,
                                                               uint8_t progress_motor,
                                                               const char *progress_phase)
{
    uint32_t elapsed_ms = 0;
    uint32_t next_log_ms = CONFIG_PUMP_STAGE_LOG_INTERVAL_MS;
    uint32_t next_heartbeat_ms = CONFIG_PUMP_STAGE_HEARTBEAT_INTERVAL_MS;
    uint32_t next_progress_publish_ms = CONFIG_PUMP_PROGRESS_PUBLISH_INTERVAL_MS;

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
        actuation_app_publish_status(false);

        if (log_progress &&
            stage_name != NULL &&
            elapsed_ms >= next_heartbeat_ms &&
            elapsed_ms < wait_ms)
        {
            actuation_app_log_timer_heartbeat(stage_name, elapsed_ms, wait_ms);

            while (next_heartbeat_ms <= elapsed_ms)
            {
                next_heartbeat_ms += CONFIG_PUMP_STAGE_HEARTBEAT_INTERVAL_MS;
            }
        }

        if (log_progress &&
            progress_phase != NULL &&
            elapsed_ms >= next_progress_publish_ms &&
            elapsed_ms < wait_ms)
        {
            actuation_app_publish_progress(progress_motor, progress_phase, elapsed_ms, wait_ms);

            while (next_progress_publish_ms <= elapsed_ms)
            {
                next_progress_publish_ms += CONFIG_PUMP_PROGRESS_PUBLISH_INTERVAL_MS;
            }
        }

        if (log_progress &&
            stage_name != NULL &&
            (elapsed_ms >= next_log_ms || elapsed_ms >= wait_ms))
        {
            actuation_app_log_timer_progress(stage_name, elapsed_ms, wait_ms, monitored_count);

            while (next_log_ms <= elapsed_ms)
            {
                next_log_ms += CONFIG_PUMP_STAGE_LOG_INTERVAL_MS;
            }
        }
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

static uint32_t actuation_app_get_channel_ramp_ms(uint8_t channel)
{
    switch (channel)
    {
        case 0:
            return actuation_app_sec_to_ms_u32(actuation_app_config.ramp_1_delay_sec);
        case 1:
            return actuation_app_sec_to_ms_u32(actuation_app_config.ramp_2_delay_sec);
        case 2:
            return actuation_app_sec_to_ms_u32(actuation_app_config.ramp_3_delay_sec);
        case 3:
            return actuation_app_sec_to_ms_u32(actuation_app_config.ramp_4_delay_sec);
        default:
            return 0;
    }
}

static void actuation_app_monitor_shutdown_status(uint8_t ramp_channel,
                                                  bool final_validation,
                                                  uint8_t *warned_channels)
{
    actuation_status status = actuation_app_read_status();

    for (uint8_t channel = 0; channel < CONFIG_ACTUATION_CHANNEL_COUNT; channel++)
    {
        const uint8_t expected = actuation_app_on_relay_enabled[channel]
            ? ACTUATION_STATUS_ON
            : ACTUATION_STATUS_OFF;

        if (status.channels[channel] == expected)
        {
            continue;
        }

        if (channel == ramp_channel && !final_validation)
        {
            continue;
        }

        const uint8_t channel_mask = (uint8_t)(1U << channel);
        if (warned_channels != NULL && ((*warned_channels & channel_mask) != 0U))
        {
            continue;
        }

        LOG_WARNING(ACTUATION_APP_TAG,
                    "ACTUATION",
                    "Sequential stop feedback mismatch: motor %u expected %s, read %s",
                    (unsigned int)(channel + 1U),
                    expected == ACTUATION_STATUS_ON ? "ON" : "OFF",
                    status.channels[channel] == ACTUATION_STATUS_ON ? "ON" : "OFF");

        if (warned_channels != NULL)
        {
            *warned_channels |= channel_mask;
        }
    }
}

static void actuation_app_stop_channel_with_ramp(uint8_t channel, bool fault)
{
    const uint32_t ramp_ms = actuation_app_get_channel_ramp_ms(channel);
    uint32_t elapsed_ms = 0;
    uint32_t next_heartbeat_ms = CONFIG_PUMP_STAGE_HEARTBEAT_INTERVAL_MS;
    uint32_t next_progress_ms = CONFIG_PUMP_PROGRESS_PUBLISH_INTERVAL_MS;
    uint8_t warned_channels = 0;
    char stage_name[40] = {};
    const char *progress_phase = fault ? "FAULT_STOP_RAMP" : "STOP_RAMP";

    if (!actuation_app_on_relay_enabled[channel])
    {
        return;
    }

    snprintf(stage_name, sizeof(stage_name), "Stop motor %u ramp", (unsigned int)(channel + 1U));
    LOG_PROCESSING(ACTUATION_APP_TAG,
                   "Sequential stop: disabling motor %u ON relay",
                   (unsigned int)(channel + 1U));

    gpio_actuator_disable_on_relay(channel);
    actuation_app_on_relay_enabled[channel] = false;
    actuation_app_publish_progress(channel + 1U, progress_phase, 0, ramp_ms);

    while (elapsed_ms < ramp_ms)
    {
        const uint32_t step_ms = actuation_app_min_u32(CONFIG_PUMP_MONITOR_INTERVAL_MS,
                                                       ramp_ms - elapsed_ms);
        vTaskDelay(pdMS_TO_TICKS(step_ms));
        elapsed_ms += step_ms;

        actuation_app_monitor_shutdown_status(channel, false, &warned_channels);
        actuation_app_publish_status(false);

        if (elapsed_ms >= next_heartbeat_ms && elapsed_ms < ramp_ms)
        {
            actuation_app_log_timer_heartbeat(stage_name, elapsed_ms, ramp_ms);
            while (next_heartbeat_ms <= elapsed_ms)
            {
                next_heartbeat_ms += CONFIG_PUMP_STAGE_HEARTBEAT_INTERVAL_MS;
            }
        }

        if (elapsed_ms >= next_progress_ms && elapsed_ms < ramp_ms)
        {
            actuation_app_publish_progress(channel + 1U, progress_phase, elapsed_ms, ramp_ms);
            while (next_progress_ms <= elapsed_ms)
            {
                next_progress_ms += CONFIG_PUMP_PROGRESS_PUBLISH_INTERVAL_MS;
            }
        }
    }

    actuation_app_monitor_shutdown_status(channel, true, &warned_channels);
    actuation_app_publish_progress(channel + 1U, progress_phase, ramp_ms, ramp_ms);
    LOG_PROCESSING(ACTUATION_APP_TAG,
                   "Sequential stop: motor %u ramp completed",
                   (unsigned int)(channel + 1U));
}

static void actuation_app_stop_pump_sequence(bool fault)
{
    LOG_PROCESSING(ACTUATION_APP_TAG,
                   "Sequential stop started: M4 -> M3 -> M2 -> M1, OFF relays disabled");

    for (int channel = CONFIG_ACTUATION_CHANNEL_COUNT - 1; channel >= 0; channel--)
    {
        actuation_app_stop_channel_with_ramp((uint8_t)channel, fault);
    }

    gpio_actuator_disable_all_on_relays();
    memset(actuation_app_on_relay_enabled, 0, sizeof(actuation_app_on_relay_enabled));
}

static void actuation_app_stop_pump(bool fault)
{
    actuation_pump_state previous_state = actuation_app_state;
    actuation_shutdown_reason shutdown_reason = ACTUATION_SHUTDOWN_REASON_COMMAND_OFF;
    uint8_t shutdown_motor = fault ? actuation_app_last_fault_channel : 0;
    char shutdown_user[sizeof(actuation_app_last_actions.user)] = {};
    char shutdown_phase[CONFIG_ACTUATION_SHUTDOWN_TEXT_SIZE] = {};

    strncpy(shutdown_user, actuation_app_last_actions.user, sizeof(shutdown_user) - 1U);
    strncpy(shutdown_phase,
            actuation_app_phase_context,
            sizeof(shutdown_phase) - 1U);

    actuation_app_state = fault ? ACTUATION_PUMP_STATE_FAULT : ACTUATION_PUMP_STATE_STOPPING;
    actuation_app_publish_progress(shutdown_motor, fault ? "FAULT" : "STOPPING", 0, 0);

    if (fault)
    {
        shutdown_reason = (previous_state == ACTUATION_PUMP_STATE_RUNNING)
            ? ACTUATION_SHUTDOWN_REASON_RUNTIME_FAULT
            : ACTUATION_SHUTDOWN_REASON_STARTUP_FAULT;
        LOG_ERROR(ACTUATION_APP_TAG, "ACTUATION", "Pump fault detected. Starting sequential stop");
    }
    else
    {
        actuation_app_last_fault_channel = 0;
        LOG_PROCESSING(ACTUATION_APP_TAG, "Pump stop requested");
    }

    actuation_app_stop_pump_sequence(fault);
    actuation_app_store_stop_actions();
    actuation_app_read_status();
    actuation_app_record_shutdown_event(shutdown_reason, shutdown_motor, shutdown_user, shutdown_phase);

    if (fault == false)
    {
        actuation_app_state = ACTUATION_PUMP_STATE_STOPPED;
        actuation_app_publish_progress(0, "STOPPED", 0, 0);
    }
    else
    {
        actuation_app_publish_progress(shutdown_motor, "FAULT_STOPPED", 0, 0);
    }

    if (actuation_app_callback != NULL)
    {
        actuation_app_callback("#28$", actuation_app_progress_mode);

        if (actuation_app_progress_mode != comm_main_mode)
        {
            actuation_app_callback("#28$", comm_main_mode);
        }
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

static bool actuation_app_wait_phase_and_validate(const char *phase_name,
                                                 uint32_t wait_ms,
                                                 uint8_t monitored_count,
                                                 uint8_t validate_count,
                                                 uint8_t progress_motor,
                                                 const char *progress_phase)
{
    if (wait_ms > 0)
    {
        actuation_app_publish_progress(progress_motor, progress_phase, 0, wait_ms);

        LOG_TIMER(ACTUATION_APP_TAG,
                  "%s: waiting %lu s, monitoring first %u channel(s), then validating first %u channel(s)",
                  phase_name,
                  (unsigned long)actuation_app_ms_to_sec_round_up(wait_ms),
                  (unsigned int)monitored_count,
                  (unsigned int)validate_count);

        if (actuation_app_handle_sequence_result(
                actuation_app_wait_and_monitor(wait_ms,
                                               monitored_count,
                                               phase_name,
                                               true,
                                               progress_motor,
                                               progress_phase)) == false)
        {
            return false;
        }
    }
    else
    {
        LOG_WARNING(ACTUATION_APP_TAG,
                    "TIMER",
                    "%s: no wait configured, validating first %u channel(s)",
                    phase_name,
                    (unsigned int)validate_count);
        actuation_app_publish_progress(progress_motor, progress_phase, 0, 0);
    }

    if (actuation_app_handle_sequence_result(actuation_app_validate_channels(validate_count)) == false)
    {
        return false;
    }

    LOG_SUCCESS(ACTUATION_APP_TAG,
                "ACTUATION",
                "%s: validation OK for first %u channel(s)",
                phase_name,
                (unsigned int)validate_count);

    actuation_app_publish_progress(progress_motor, progress_phase, wait_ms, wait_ms);

    return true;
}

static bool actuation_app_start_channel_and_validate(uint8_t channel,
                                                    uint32_t ramp_ms,
                                                    uint32_t stage_ms,
                                                    uint8_t previous_count,
                                                    uint8_t enabled_count,
                                                    const char *stage_name)
{
    char ramp_name[40] = {};
    char interval_name[40] = {};

    LOG_PROCESSING(ACTUATION_APP_TAG,
                   "%s: enabling channel %u ON relay",
                   stage_name,
                   (unsigned int)(channel + 1));

    if (gpio_actuator_enable_on_relay(channel) != ESP_OK)
    {
        actuation_app_stop_pump(true);
        return false;
    }

    actuation_app_on_relay_enabled[channel] = true;

    actuation_app_publish_progress(channel + 1U, "ON", 0, 0);

    snprintf(ramp_name, sizeof(ramp_name), "%s ramp", stage_name);
    if (actuation_app_wait_phase_and_validate(ramp_name,
                                              ramp_ms,
                                              previous_count,
                                              enabled_count,
                                              channel + 1U,
                                              "RAMP") == false)
    {
        return false;
    }

    snprintf(interval_name, sizeof(interval_name), "%s interval", stage_name);
    return actuation_app_wait_phase_and_validate(interval_name,
                                                 stage_ms,
                                                 enabled_count,
                                                 enabled_count,
                                                 channel + 1U,
                                                 "STAGE");
}

static void actuation_app_start_pump_sequence(void)
{
    if (actuation_app_state == ACTUATION_PUMP_STATE_STARTING ||
        actuation_app_state == ACTUATION_PUMP_STATE_RUNNING)
    {
        LOG_WARNING(ACTUATION_APP_TAG, "ACTUATION", "Pump start ignored: already active");
        return;
    }

    LOG_PROCESSING(ACTUATION_APP_TAG, "Pump start sequence requested");
    LOG_TIMER(ACTUATION_APP_TAG,
              "Configuration: ramp1=%u s stage1=%u s ramp2=%u s stage2=%u s ramp3=%u s stage3=%u s ramp4=%u s stage4=%u s periodic_status=%u min",
              (unsigned int)actuation_app_config.ramp_1_delay_sec,
              (unsigned int)actuation_app_config.stage_1_delay_sec,
              (unsigned int)actuation_app_config.ramp_2_delay_sec,
              (unsigned int)actuation_app_config.stage_2_delay_sec,
              (unsigned int)actuation_app_config.ramp_3_delay_sec,
              (unsigned int)actuation_app_config.stage_3_delay_sec,
              (unsigned int)actuation_app_config.ramp_4_delay_sec,
              (unsigned int)actuation_app_config.stage_4_delay_sec,
              (unsigned int)actuation_app_config.status_publish_time_min);
    actuation_app_last_fault_channel = 0;
    memset(actuation_app_on_relay_enabled, 0, sizeof(actuation_app_on_relay_enabled));
    actuation_app_state = ACTUATION_PUMP_STATE_STARTING;
    actuation_app_publish_progress(0, "START_REQUESTED", 0, 0);

    if (actuation_app_start_channel_and_validate(0,
                                                 actuation_app_sec_to_ms_u32(actuation_app_config.ramp_1_delay_sec),
                                                 actuation_app_sec_to_ms_u32(actuation_app_config.stage_1_delay_sec),
                                                 0,
                                                 1,
                                                 "Stage 1/4") == false)
    {
        return;
    }

    if (actuation_app_start_channel_and_validate(1,
                                                 actuation_app_sec_to_ms_u32(actuation_app_config.ramp_2_delay_sec),
                                                 actuation_app_sec_to_ms_u32(actuation_app_config.stage_2_delay_sec),
                                                 1,
                                                 2,
                                                 "Stage 2/4") == false)
    {
        return;
    }

    if (actuation_app_start_channel_and_validate(2,
                                                 actuation_app_sec_to_ms_u32(actuation_app_config.ramp_3_delay_sec),
                                                 actuation_app_sec_to_ms_u32(actuation_app_config.stage_3_delay_sec),
                                                 2,
                                                 3,
                                                 "Stage 3/4") == false)
    {
        return;
    }

    if (actuation_app_start_channel_and_validate(3,
                                                 actuation_app_sec_to_ms_u32(actuation_app_config.ramp_4_delay_sec),
                                                 actuation_app_sec_to_ms_u32(actuation_app_config.stage_4_delay_sec),
                                                 3,
                                                 4,
                                                 "Stage 4/4") == false)
    {
        return;
    }

    actuation_app_state = ACTUATION_PUMP_STATE_RUNNING;
    actuation_app_publish_progress(4, "RUNNING", 0, 0);
    LOG_SUCCESS(ACTUATION_APP_TAG, "ACTUATION", "Stage 4/4: validation OK for all channels");
    LOG_DEFAULT(ACTUATION_APP_TAG,
                "MONITORING",
                "Pump running: monitoring all channels every %lu ms and publishing #00 every %u s",
                (unsigned long)CONFIG_PUMP_MONITOR_INTERVAL_MS,
                (unsigned int)(actuation_app_config.status_publish_time_min * 60U));
    actuation_app_publish_status(true);

    while (actuation_app_state == ACTUATION_PUMP_STATE_RUNNING)
    {
        if (actuation_app_handle_sequence_result(
                actuation_app_wait_and_monitor(CONFIG_PUMP_MONITOR_INTERVAL_MS,
                                               CONFIG_ACTUATION_CHANNEL_COUNT,
                                               "Running",
                                               false,
                                               0,
                                               "RUNNING")) == false)
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
            actuation_app_publish_status(false);
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
            LOG_ERROR(ACTUATION_APP_TAG, "ACTUATION", "%s, failed to create command queue", __func__);
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
            LOG_ERROR(ACTUATION_APP_TAG, "ACTUATION", "%s, failed to create task: %s", __func__, ACTUATION_APP_TASK_NAME);
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}

esp_err_t actuation_app_set_config(actuation_config config)
{
    actuation_app_config = actuation_app_sanitize_config(config);
    LOG_TIMER(ACTUATION_APP_TAG,
              "Actuation config: off_relay_ms=%u idle_read=%u s active_level=%u ramp1=%u s stage1=%u s ramp2=%u s stage2=%u s ramp3=%u s stage3=%u s ramp4=%u s stage4=%u s publish=%u min",
              (unsigned int)actuation_app_config.relay_pulse_time_ms,
              (unsigned int)actuation_app_config.idle_read_time_sec,
              (unsigned int)(actuation_app_config.status_active_level ? 1 : 0),
              (unsigned int)actuation_app_config.ramp_1_delay_sec,
              (unsigned int)actuation_app_config.stage_1_delay_sec,
              (unsigned int)actuation_app_config.ramp_2_delay_sec,
              (unsigned int)actuation_app_config.stage_2_delay_sec,
              (unsigned int)actuation_app_config.ramp_3_delay_sec,
              (unsigned int)actuation_app_config.stage_3_delay_sec,
              (unsigned int)actuation_app_config.ramp_4_delay_sec,
              (unsigned int)actuation_app_config.stage_4_delay_sec,
              (unsigned int)actuation_app_config.status_publish_time_min);

    return gpio_actuator_config(actuation_app_config);
}

void actuation_app_set_actions(const actuation_actions actions, bool alert_change)
{
    UNUSED(alert_change);
    actuation_pump_command command = ACTUATION_PUMP_COMMAND_NONE;

    if (actuation_app_command_is_valid(&actions) != true)
    {
        LOG_ERROR(ACTUATION_APP_TAG, "ACTUATION", "Invalid actuation command");
        return;
    }

    actuation_app_last_actions = actions;
    data_app_save(DATA_TYPE_ACTUATION_ACTIONS, &actuation_app_last_actions, sizeof(actuation_app_last_actions));

    if (actuation_app_actions_request_stop(&actions))
    {
        command = ACTUATION_PUMP_COMMAND_STOP;
        LOG_PROCESSING(ACTUATION_APP_TAG,
                       "Pump STOP command received: user=%s",
                       actions.user);
    }
    else if (actuation_app_actions_request_start(&actions))
    {
        command = ACTUATION_PUMP_COMMAND_START;
        LOG_PROCESSING(ACTUATION_APP_TAG,
                       "Pump START command received: user=%s, commands C1=%u C2=%u C3=%u C4=%u",
                       actions.user,
                       (unsigned int)actions.channels[0],
                       (unsigned int)actions.channels[1],
                       (unsigned int)actions.channels[2],
                       (unsigned int)actions.channels[3]);
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

void actuation_app_set_progress_mode(comm_type communication)
{
    if (communication == COMM_RF ||
        communication == COMM_MQTT ||
        communication == COMM_HTTP_GET ||
        communication == COMM_HTTP_POST)
    {
        actuation_app_progress_mode = communication;
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

const char *actuation_app_get_state_name(void)
{
    switch (actuation_app_state)
    {
        case ACTUATION_PUMP_STATE_STOPPED:
            return "STOPPED";
        case ACTUATION_PUMP_STATE_STARTING:
            return "STARTING";
        case ACTUATION_PUMP_STATE_RUNNING:
            return "RUNNING";
        case ACTUATION_PUMP_STATE_STOPPING:
            return "STOPPING";
        case ACTUATION_PUMP_STATE_FAULT:
            return "FAULT";
        default:
            return "UNKNOWN";
    }
}

uint8_t actuation_app_get_last_fault_channel(void)
{
    return actuation_app_last_fault_channel;
}

uint32_t actuation_app_get_shutdown_info(actuation_shutdown_info *info_out, size_t info_size)
{
    if (info_out != NULL && info_size > 0)
    {
        size_t copy_size = (info_size < sizeof(actuation_app_last_shutdown_info))
            ? info_size
            : sizeof(actuation_app_last_shutdown_info);

        memcpy(info_out, &actuation_app_last_shutdown_info, copy_size);
    }

    return actuation_app_shutdown_sequence;
}

void actuation_app_shutdown(void)
{
    gpio_actuator_shutdown();
}
