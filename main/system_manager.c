/**
 * @file system_manager.c
 * @brief System bootstrap for the new four-channel actuation product.
 */

#include "system_manager.h"

#include "actuation_app.h"
#include "comm_app.h"
#include "data_app.h"
#include "FreeRTOS_defines.h"
#include "idp_parser.h"
#include "log.h"
#include "project_config.h"
#include "rtc_app.h"
#include "scheduling.h"
#include "system_monitoring.h"

#include "esp_err.h"
#include "esp_system.h"

#include <stdio.h>
#include <string.h>

/**
 * @brief Log tag for the system manager module.
 */
#define SYSTEM_MANAGER_TAG "system_manager"
#define SYSTEM_MANAGER_RX_BUFFER_SIZE (512)
#define SYSTEM_MANAGER_PACKET_BUFFER_SIZE (256)
#define SYSTEM_MANAGER_ACTUATION_CONFIG_KEY "act_config"
#define SYSTEM_MANAGER_SHUTDOWN_REASON_KEY "reason_hangup"
#define SYSTEM_MANAGER_UNIX_TIMESTAMP_THRESHOLD (1000000000LL)

uint16_t global_pressure = 0;
comm_type comm_main_mode = COMM_RF;
char system_id[50] = "newproduct_1";

static bool system_manager_comm_ready = false;
static char system_manager_rf_rx_buffer[SYSTEM_MANAGER_RX_BUFFER_SIZE] = {};
static size_t system_manager_rf_rx_len = 0;
static char system_manager_mqtt_rx_buffer[SYSTEM_MANAGER_RX_BUFFER_SIZE] = {};
static size_t system_manager_mqtt_rx_len = 0;
static uint32_t system_manager_saved_shutdown_sequence = 0;

static bool system_manager_actions_request_start(const actuation_actions *actions);

static void system_manager_log_status(const actuation_status *status)
{
    if (status == NULL)
    {
        return;
    }

    LOG_DEFAULT(SYSTEM_MANAGER_TAG,
                "STATUS",
                "Actuation status: C1=%u C2=%u C3=%u C4=%u",
                (unsigned int)status->channels[0],
                (unsigned int)status->channels[1],
                (unsigned int)status->channels[2],
                (unsigned int)status->channels[3]);
}

static bool system_manager_device_matches(const char *device_id)
{
    if (device_id == NULL || device_id[0] == '\0')
    {
        return true;
    }

    return (strcmp(device_id, system_id) == 0 ||
            strcmp(device_id, "broadcast") == 0 ||
            strcmp(device_id, "all") == 0);
}

static bool system_manager_start_is_busy(void)
{
    const char *state = actuation_app_get_state_name();

    return (strcmp(state, "STARTING") == 0 ||
            strcmp(state, "RUNNING") == 0 ||
            strcmp(state, "STOPPING") == 0);
}

static void system_manager_send_packet(const char *packet, comm_type communication)
{
    if (packet == NULL || system_manager_comm_ready == false)
    {
        return;
    }

    comm_app_send_idp_pack(packet, communication);
}

static void system_manager_send_error(const char *error, comm_type communication)
{
    char packet[160] = {};

    if (error == NULL)
    {
        error = "unknown_error";
    }

    snprintf(packet, sizeof(packet), "#99-%s-%s$", system_id, error);
    system_manager_send_packet(packet, communication);
}

static void system_manager_send_status(comm_type communication)
{
    actuation_status status = {};
    char packet[SYSTEM_MANAGER_PACKET_BUFFER_SIZE] = {};
    time_t timestamp = rtc_app_get_timestamp(false);

    actuation_app_get_status(&status, sizeof(status));

    snprintf(packet,
             sizeof(packet),
             "#00-%s-%s-%u-%u-%u-%u-%u-%lld$",
             system_id,
             actuation_app_get_state_name(),
             (unsigned int)status.channels[0],
             (unsigned int)status.channels[1],
             (unsigned int)status.channels[2],
             (unsigned int)status.channels[3],
             (unsigned int)actuation_app_get_last_fault_channel(),
             (long long)timestamp);

    system_manager_send_packet(packet, communication);
}

static void system_manager_send_comm_mode(comm_type communication)
{
    char packet[80] = {};
    const char *mode = (comm_main_mode == COMM_RF) ? "RF" : "MQTT";

    snprintf(packet, sizeof(packet), "#31-%s-%s$", system_id, mode);
    system_manager_send_packet(packet, communication);
}

static const char *system_manager_reset_reason_to_string(esp_reset_reason_t reset_reason)
{
    switch (reset_reason)
    {
        case ESP_RST_POWERON:
            return "poweron";
        case ESP_RST_EXT:
            return "external";
        case ESP_RST_SW:
            return "software";
        case ESP_RST_PANIC:
            return "panic";
        case ESP_RST_INT_WDT:
            return "int_wdt";
        case ESP_RST_TASK_WDT:
            return "task_wdt";
        case ESP_RST_WDT:
            return "wdt";
        case ESP_RST_DEEPSLEEP:
            return "deepsleep";
        case ESP_RST_BROWNOUT:
            return "brownout";
        case ESP_RST_SDIO:
            return "sdio";
        default:
            return "unknown";
    }
}

static const char *system_manager_shutdown_reason_to_string(uint8_t reason)
{
    switch ((actuation_shutdown_reason)reason)
    {
        case ACTUATION_SHUTDOWN_REASON_COMMAND_OFF:
            return "command_off";
        case ACTUATION_SHUTDOWN_REASON_STARTUP_FAULT:
            return "startup_fault";
        case ACTUATION_SHUTDOWN_REASON_RUNTIME_FAULT:
            return "runtime_fault";
        case ACTUATION_SHUTDOWN_REASON_BROWNOUT:
            return "brownout";
        case ACTUATION_SHUTDOWN_REASON_BOOT:
            return "boot";
        case ACTUATION_SHUTDOWN_REASON_NONE:
        default:
            return "none";
    }
}

static const char *system_manager_shutdown_origin_to_string(uint8_t reason)
{
    switch ((actuation_shutdown_reason)reason)
    {
        case ACTUATION_SHUTDOWN_REASON_COMMAND_OFF:
            return "command";
        case ACTUATION_SHUTDOWN_REASON_STARTUP_FAULT:
        case ACTUATION_SHUTDOWN_REASON_RUNTIME_FAULT:
            return "actuation_app";
        case ACTUATION_SHUTDOWN_REASON_BROWNOUT:
        case ACTUATION_SHUTDOWN_REASON_BOOT:
            return "boot";
        case ACTUATION_SHUTDOWN_REASON_NONE:
        default:
            return "unknown";
    }
}

static void system_manager_sanitize_packet_field(char *out,
                                                 size_t out_size,
                                                 const char *in,
                                                 const char *fallback)
{
    const char *source = (in != NULL && in[0] != '\0') ? in : fallback;
    size_t out_index = 0;

    if (out == NULL || out_size == 0)
    {
        return;
    }

    if (source == NULL || source[0] == '\0')
    {
        source = "unknown";
    }

    for (size_t in_index = 0; source[in_index] != '\0' && out_index < (out_size - 1U); in_index++)
    {
        char value = source[in_index];

        if (value == '-' || value == '#' || value == '$' || value == '\r' || value == '\n' || value == ' ')
        {
            value = '_';
        }

        out[out_index++] = value;
    }

    out[out_index] = '\0';
}

static void system_manager_build_shutdown_packet(char *packet,
                                                 size_t packet_size,
                                                 uint8_t reason,
                                                 const char *user,
                                                 const char *phase,
                                                 uint8_t motor,
                                                 const char *reset_reason)
{
    char safe_device[sizeof(system_id)] = {};
    char safe_user[50] = {};
    char safe_phase[CONFIG_ACTUATION_SHUTDOWN_TEXT_SIZE] = {};
    time_t timestamp = rtc_app_get_timestamp(false);

    if (packet == NULL || packet_size == 0)
    {
        return;
    }

    system_manager_sanitize_packet_field(safe_device, sizeof(safe_device), system_id, "newproduct_1");
    system_manager_sanitize_packet_field(safe_user, sizeof(safe_user), user, "unknown");
    system_manager_sanitize_packet_field(safe_phase, sizeof(safe_phase), phase, "unknown");

    snprintf(packet,
             packet_size,
             "#28-%s-%s-%s-%s-%s-%u-%s-%lld$",
             safe_device,
             system_manager_shutdown_reason_to_string(reason),
             system_manager_shutdown_origin_to_string(reason),
             safe_user,
             safe_phase,
             (unsigned int)motor,
             (reset_reason != NULL && reset_reason[0] != '\0') ? reset_reason : "none",
             (long long)timestamp);
}

static void system_manager_save_shutdown_packet(const char *packet)
{
    if (packet == NULL || packet[0] == '\0')
    {
        return;
    }

    data_app_save(DATA_TYPE_REASON_HANG_UP, packet, strlen(packet) + 1U);
}

static bool system_manager_load_shutdown_packet(char *packet, size_t packet_size)
{
    size_t stored_size = 0;

    if (packet == NULL || packet_size == 0)
    {
        return false;
    }

    memset(packet, 0, packet_size);
    stored_size = data_app_get_data_size(SYSTEM_MANAGER_SHUTDOWN_REASON_KEY);

    if (stored_size == 0 || stored_size >= packet_size)
    {
        return false;
    }

    if (data_app_load(DATA_TYPE_REASON_HANG_UP, packet) != ESP_OK)
    {
        packet[0] = '\0';
        return false;
    }

    packet[packet_size - 1U] = '\0';
    return (strncmp(packet, "#28-", 4) == 0);
}

static void system_manager_record_shutdown_info(const actuation_shutdown_info *info,
                                                const char *reset_reason)
{
    char packet[SYSTEM_MANAGER_PACKET_BUFFER_SIZE] = {};

    if (info == NULL)
    {
        return;
    }

    system_manager_build_shutdown_packet(packet,
                                         sizeof(packet),
                                         info->reason,
                                         info->user,
                                         info->phase,
                                         info->motor,
                                         reset_reason);
    system_manager_save_shutdown_packet(packet);
}

static void system_manager_record_pending_shutdown_event(void)
{
    actuation_shutdown_info info = {};
    uint32_t sequence = actuation_app_get_shutdown_info(&info, sizeof(info));

    if (sequence == 0 || sequence == system_manager_saved_shutdown_sequence)
    {
        return;
    }

    system_manager_record_shutdown_info(&info, "none");
    system_manager_saved_shutdown_sequence = sequence;
}

static void system_manager_send_shutdown_reason(comm_type communication)
{
    char packet[SYSTEM_MANAGER_PACKET_BUFFER_SIZE] = {};

    system_manager_record_pending_shutdown_event();

    if (system_manager_load_shutdown_packet(packet, sizeof(packet)) == false)
    {
        actuation_shutdown_info info = {
            .reason = ACTUATION_SHUTDOWN_REASON_NONE,
            .motor = 0,
        };

        strncpy(info.user, "unknown", sizeof(info.user) - 1U);
        strncpy(info.phase, "unknown", sizeof(info.phase) - 1U);
        system_manager_record_shutdown_info(&info, "none");
        system_manager_load_shutdown_packet(packet, sizeof(packet));
    }

    system_manager_send_packet(packet, communication);
}

static bool system_manager_sanitize_actuation_config(actuation_config *config)
{
    bool changed = false;

    if (config == NULL)
    {
        return false;
    }

    if (config->relay_pulse_time_ms == 0)
    {
        config->relay_pulse_time_ms = CONFIG_ACTUATION_DEFAULT_RELAY_PULSE_MS;
        changed = true;
    }

    if (config->idle_read_time_sec == 0)
    {
        config->idle_read_time_sec = CONFIG_ACTUATION_DEFAULT_IDLE_READ_TIME_SEC;
        changed = true;
    }

    if (config->ramp_1_delay_sec == 0)
    {
        config->ramp_1_delay_sec = CONFIG_PUMP_RAMP_1_DELAY_MS / 1000U;
        changed = true;
    }

    if (config->stage_1_delay_sec == 0)
    {
        config->stage_1_delay_sec = CONFIG_PUMP_STAGE_1_DELAY_MS / 1000U;
        changed = true;
    }

    if (config->ramp_2_delay_sec == 0)
    {
        config->ramp_2_delay_sec = CONFIG_PUMP_RAMP_2_DELAY_MS / 1000U;
        changed = true;
    }

    if (config->stage_2_delay_sec == 0)
    {
        config->stage_2_delay_sec = CONFIG_PUMP_STAGE_2_DELAY_MS / 1000U;
        changed = true;
    }

    if (config->ramp_3_delay_sec == 0)
    {
        config->ramp_3_delay_sec = CONFIG_PUMP_RAMP_3_DELAY_MS / 1000U;
        changed = true;
    }

    if (config->stage_3_delay_sec == 0)
    {
        config->stage_3_delay_sec = CONFIG_PUMP_STAGE_3_DELAY_MS / 1000U;
        changed = true;
    }

    if (config->ramp_4_delay_sec == 0)
    {
        config->ramp_4_delay_sec = CONFIG_PUMP_RAMP_4_DELAY_MS / 1000U;
        changed = true;
    }

    if (config->status_publish_time_min == 0)
    {
        config->status_publish_time_min = CONFIG_ACTUATION_DEFAULT_STATUS_PUBLISH_MIN;
        changed = true;
    }

    return changed;
}

static bool system_manager_load_actuation_config(actuation_config *config)
{
    if (config == NULL)
    {
        return false;
    }

    *config = (actuation_config){};

    if (data_app_get_data_size(SYSTEM_MANAGER_ACTUATION_CONFIG_KEY) != sizeof(*config))
    {
        return false;
    }

    return (data_app_load(DATA_TYPE_ACTUATION_CONFIG, config) == ESP_OK);
}

static void system_manager_load_comm_mode(void)
{
    comm_main_mode_config config = {};
    bool config_changed = false;

    if (data_app_load(DATA_TYPE_COMM_MAIN_MODE, &config) != ESP_OK ||
        (strcmp(config.comm_main_mode_config, "RF") != 0 &&
         strcmp(config.comm_main_mode_config, "MQTT") != 0))
    {
        strncpy(config.comm_main_mode_config, "RF", sizeof(config.comm_main_mode_config) - 1);
        config_changed = true;
    }

    if (comm_app_set_main_mode_config(config) != ESP_OK)
    {
        strncpy(config.comm_main_mode_config, "RF", sizeof(config.comm_main_mode_config) - 1);
        comm_app_set_main_mode_config(config);
        config_changed = true;
    }

    if (config_changed)
    {
        data_app_save(DATA_TYPE_COMM_MAIN_MODE, &config, sizeof(config));
    }
}

static time_t system_manager_resolve_schedule_time(time_t value, time_t now)
{
    if (value >= SYSTEM_MANAGER_UNIX_TIMESTAMP_THRESHOLD)
    {
        return value;
    }

    return now + value;
}

static void system_manager_send_schedule_error(esp_err_t error,
                                               const char *idp_error,
                                               comm_type communication)
{
    if (error == ESP_ERR_INVALID_STATE)
    {
        system_manager_send_error("schedule_conflict", communication);
    }
    else if (error == ESP_ERR_NO_MEM)
    {
        system_manager_send_error("schedule_full", communication);
    }
    else
    {
        system_manager_send_error(idp_error, communication);
    }
}

static void system_manager_schedule_event_callback(idp_type scheduling_idp,
                                                   const char *scheduling_id,
                                                   const char *event,
                                                   const char *user)
{
    char packet[SYSTEM_MANAGER_PACKET_BUFFER_SIZE] = {};
    char safe_user[50] = {};
    time_t timestamp = rtc_app_get_timestamp(false);

    system_manager_sanitize_packet_field(safe_user,
                                         sizeof(safe_user),
                                         user,
                                         "scheduling");

    snprintf(packet,
             sizeof(packet),
             "#18-%s-%u-%s-%s-%s-%lld$",
             system_id,
             (unsigned int)scheduling_idp,
             (scheduling_id != NULL && scheduling_id[0] != '\0') ? scheduling_id : "unknown",
             (event != NULL && event[0] != '\0') ? event : "UNKNOWN",
             safe_user,
             (long long)timestamp);
    system_manager_send_packet(packet, comm_main_mode);
}

static void system_manager_monitoring_send_callback(const char *packet,
                                                    comm_type communication)
{
    system_manager_send_packet(packet, communication);
}

static void system_manager_actuation_send_callback(const char *packet,
                                                   comm_type communication)
{
    if (packet == NULL)
    {
        return;
    }

    if (strcmp(packet, "#00$") == 0)
    {
        system_manager_send_status(communication);
        return;
    }

    if (strcmp(packet, "#28$") == 0)
    {
        system_manager_send_shutdown_reason(communication);
        return;
    }

    system_manager_send_packet(packet, communication);
}

static void system_manager_handle_idp_0(const char *packet, comm_type communication)
{
    UNUSED(packet);
    system_manager_send_status(communication);
}

static void system_manager_handle_idp_1(const char *packet, comm_type communication)
{
    uint8_t idp = 0;
    char device_id[50] = {};
    actuation_actions actions = {};
    char response[120] = {};

    arg_pair_t args[] = {
        {"uint8_t", &idp},
        {"string", device_id},
        {"uint8_t", &actions.channels[0]},
        {"uint8_t", &actions.channels[1]},
        {"uint8_t", &actions.channels[2]},
        {"uint8_t", &actions.channels[3]},
        {"string", actions.user},
        {NULL, NULL},
    };

    if (idp_parser_get_delimiter(packet) < 6)
    {
        system_manager_send_error("idp_1_invalid_format", communication);
        return;
    }

    idp_parser_get_packet_data(packet, args);

    if (system_manager_device_matches(device_id) == false)
    {
        return;
    }

    for (uint8_t channel = 0; channel < CONFIG_ACTUATION_CHANNEL_COUNT; channel++)
    {
        if (actions.channels[channel] != ACTUATION_COMMAND_NONE &&
            actions.channels[channel] != ACTUATION_COMMAND_ON &&
            actions.channels[channel] != ACTUATION_COMMAND_OFF)
        {
            system_manager_send_error("idp_1_invalid_command", communication);
            return;
        }
    }

    if (system_manager_actions_request_start(&actions) && system_manager_start_is_busy())
    {
        snprintf(response,
                 sizeof(response),
                 "#01-%s-BUSY-%s$",
                 system_id,
                 actuation_app_get_state_name());
        system_manager_send_packet(response, communication);
        system_manager_send_status(communication);
        return;
    }

    actuation_app_set_progress_mode(communication);
    actuation_app_set_actions(actions, true);

    snprintf(response, sizeof(response), "#01-%s-ACCEPTED$", system_id);
    system_manager_send_packet(response, communication);
    system_manager_send_status(communication);
}

static void system_manager_handle_idp_3(const char *packet, comm_type communication)
{
    uint8_t idp = 0;
    char device_id[50] = {};
    actuation_config config = {};
    char response[SYSTEM_MANAGER_PACKET_BUFFER_SIZE] = {};
    uint8_t status_active_level = 0;
    uint8_t delimiter_count = idp_parser_get_delimiter(packet);

    arg_pair_t full_args[] = {
        {"uint8_t", &idp},
        {"string", device_id},
        {"uint16_t", &config.relay_pulse_time_ms},
        {"uint16_t", &config.idle_read_time_sec},
        {"uint8_t", &status_active_level},
        {"uint16_t", &config.ramp_1_delay_sec},
        {"uint16_t", &config.stage_1_delay_sec},
        {"uint16_t", &config.ramp_2_delay_sec},
        {"uint16_t", &config.stage_2_delay_sec},
        {"uint16_t", &config.ramp_3_delay_sec},
        {"uint16_t", &config.stage_3_delay_sec},
        {"uint16_t", &config.ramp_4_delay_sec},
        {"uint16_t", &config.stage_4_delay_sec},
        {"uint16_t", &config.status_publish_time_min},
        {NULL, NULL},
    };

    arg_pair_t legacy_args[] = {
        {"uint8_t", &idp},
        {"string", device_id},
        {"uint16_t", &config.relay_pulse_time_ms},
        {"uint16_t", &config.idle_read_time_sec},
        {"uint8_t", &status_active_level},
        {NULL, NULL},
    };

    if (delimiter_count != 0 &&
        delimiter_count != 1 &&
        delimiter_count != 4 &&
        delimiter_count != 13)
    {
        system_manager_send_error("idp_3_invalid_format", communication);
        return;
    }

    idp_parser_get_packet_data(packet, (delimiter_count == 13) ? full_args : legacy_args);

    if (system_manager_device_matches(device_id) == false)
    {
        return;
    }

    if (delimiter_count == 4 || delimiter_count == 13)
    {
        if (config.relay_pulse_time_ms == 0 ||
            config.idle_read_time_sec == 0 ||
            status_active_level > 1 ||
            (delimiter_count == 13 && config.status_publish_time_min == 0))
        {
            system_manager_send_error("idp_3_invalid_config", communication);
            return;
        }

        config.status_active_level = (status_active_level != 0);
        system_manager_sanitize_actuation_config(&config);
        actuation_app_set_config(config);
        data_app_save(DATA_TYPE_ACTUATION_CONFIG, &config, sizeof(config));
    }
    else
    {
        if (system_manager_load_actuation_config(&config) == false)
        {
            config = (actuation_config){};
        }

        if (system_manager_sanitize_actuation_config(&config))
        {
            data_app_save(DATA_TYPE_ACTUATION_CONFIG, &config, sizeof(config));
        }
    }

    snprintf(response,
             sizeof(response),
             "#03-%s-%u-%u-%u-%u-%u-%u-%u-%u-%u-%u-%u-%u$",
             system_id,
             (unsigned int)config.relay_pulse_time_ms,
             (unsigned int)config.idle_read_time_sec,
             (unsigned int)(config.status_active_level ? 1 : 0),
             (unsigned int)config.ramp_1_delay_sec,
             (unsigned int)config.stage_1_delay_sec,
             (unsigned int)config.ramp_2_delay_sec,
             (unsigned int)config.stage_2_delay_sec,
             (unsigned int)config.ramp_3_delay_sec,
             (unsigned int)config.stage_3_delay_sec,
             (unsigned int)config.ramp_4_delay_sec,
             (unsigned int)config.stage_4_delay_sec,
             (unsigned int)config.status_publish_time_min);
    system_manager_send_packet(response, communication);
}

static void system_manager_handle_idp_13(const char *packet, comm_type communication)
{
    uint8_t idp = 0;
    char device_id[50] = {};
    char scheduling_id[50] = {};
    char user[50] = {};
    char response[140] = {};
    uint8_t delimiter_count = idp_parser_get_delimiter(packet);

    arg_pair_t args[] = {
        {"uint8_t", &idp},
        {"string", device_id},
        {"string", scheduling_id},
        {"string", user},
        {NULL, NULL},
    };

    if (delimiter_count != 2 && delimiter_count != 3)
    {
        system_manager_send_error("idp_13_invalid_format", communication);
        return;
    }

    idp_parser_get_packet_data(packet, args);
    if (!system_manager_device_matches(device_id))
    {
        return;
    }

    esp_err_t result = scheduling_delete(scheduling_id);
    if (result != ESP_OK)
    {
        system_manager_send_schedule_error(result,
                                           "schedule_not_found",
                                           communication);
        return;
    }

    snprintf(response,
             sizeof(response),
             "#13-%s-%s-DELETED$",
             system_id,
             scheduling_id);
    system_manager_send_packet(response, communication);
}

static void system_manager_handle_idp_14(const char *packet, comm_type communication)
{
    uint8_t idp = 0;
    char device_id[50] = {};
    char user[50] = {};
    char response[SYSTEM_MANAGER_PACKET_BUFFER_SIZE] = {};
    time_t start_value = 0;
    time_t end_value = 0;
    uint8_t delimiter_count = idp_parser_get_delimiter(packet);

    arg_pair_t args[] = {
        {"uint8_t", &idp},
        {"string", device_id},
        {"time_t", &start_value},
        {"time_t", &end_value},
        {"string", user},
        {NULL, NULL},
    };

    if (delimiter_count != 0 && delimiter_count != 1 && delimiter_count != 4)
    {
        system_manager_send_error("idp_14_invalid_format", communication);
        return;
    }

    idp_parser_get_packet_data(packet, args);
    if (!system_manager_device_matches(device_id))
    {
        return;
    }

    if (delimiter_count == 4)
    {
        time_t now = rtc_app_get_timestamp(false);
        time_t start_date = system_manager_resolve_schedule_time(start_value, now);
        time_t end_date = system_manager_resolve_schedule_time(end_value, now);
        char scheduling_id[50] = {};

        if (start_value <= 0 || end_value <= 0 ||
            start_date <= now || end_date <= start_date ||
            user[0] == '\0')
        {
            system_manager_send_error("idp_14_invalid_schedule", communication);
            return;
        }

        esp_err_t result = scheduling_add_date(start_date,
                                               end_date,
                                               user,
                                               scheduling_id,
                                               sizeof(scheduling_id));
        if (result != ESP_OK)
        {
            system_manager_send_schedule_error(result,
                                               "idp_14_invalid_schedule",
                                               communication);
            return;
        }

        snprintf(response,
                 sizeof(response),
                 "#14-%s-%s-ACCEPTED$",
                 system_id,
                 scheduling_id);
        system_manager_send_packet(response, communication);
        return;
    }

    pump_scheduling_date schedules[CONFIG_SCHEDULING_MAX_VALUE] = {};
    size_t schedules_count = scheduling_get_dates(schedules,
                                                  CONFIG_SCHEDULING_MAX_VALUE);
    bool found = false;

    for (size_t index = 0; index < schedules_count; index++)
    {
        if (schedules[index].scheduling_id[0] == '\0')
        {
            continue;
        }

        snprintf(response,
                 sizeof(response),
                 "#14-%s-%s-%lld-%lld-%u-%s$",
                 system_id,
                 schedules[index].scheduling_id,
                 (long long)schedules[index].start_date,
                 (long long)schedules[index].end_date,
                 schedules[index].started ? 1U : 0U,
                 schedules[index].user);
        system_manager_send_packet(response, communication);
        found = true;
    }

    if (!found)
    {
        snprintf(response, sizeof(response), "#14-%s-NONE$", system_id);
        system_manager_send_packet(response, communication);
    }
}

static void system_manager_handle_idp_16(const char *packet, comm_type communication)
{
    uint8_t idp = 0;
    char device_id[50] = {};
    char user[50] = {};
    char response[SYSTEM_MANAGER_PACKET_BUFFER_SIZE] = {};
    time_t end_value = 0;
    uint8_t delimiter_count = idp_parser_get_delimiter(packet);

    arg_pair_t args[] = {
        {"uint8_t", &idp},
        {"string", device_id},
        {"time_t", &end_value},
        {"string", user},
        {NULL, NULL},
    };

    if (delimiter_count != 0 && delimiter_count != 1 && delimiter_count != 3)
    {
        system_manager_send_error("idp_16_invalid_format", communication);
        return;
    }

    idp_parser_get_packet_data(packet, args);
    if (!system_manager_device_matches(device_id))
    {
        return;
    }

    if (delimiter_count == 3)
    {
        time_t now = rtc_app_get_timestamp(false);
        time_t end_date = system_manager_resolve_schedule_time(end_value, now);
        char scheduling_id[50] = {};

        if (end_value <= 0 || end_date <= now || user[0] == '\0')
        {
            system_manager_send_error("idp_16_invalid_schedule", communication);
            return;
        }

        esp_err_t result = scheduling_add_off_date(end_date,
                                                   user,
                                                   scheduling_id,
                                                   sizeof(scheduling_id));
        if (result != ESP_OK)
        {
            system_manager_send_schedule_error(result,
                                               "idp_16_invalid_schedule",
                                               communication);
            return;
        }

        snprintf(response,
                 sizeof(response),
                 "#16-%s-%s-ACCEPTED$",
                 system_id,
                 scheduling_id);
        system_manager_send_packet(response, communication);
        return;
    }

    pump_scheduling_off_date schedules[CONFIG_SCHEDULING_MAX_VALUE] = {};
    size_t schedules_count = scheduling_get_off_dates(schedules,
                                                      CONFIG_SCHEDULING_MAX_VALUE);
    bool found = false;

    for (size_t index = 0; index < schedules_count; index++)
    {
        if (schedules[index].scheduling_id[0] == '\0')
        {
            continue;
        }

        snprintf(response,
                 sizeof(response),
                 "#16-%s-%s-%lld-%s$",
                 system_id,
                 schedules[index].scheduling_id,
                 (long long)schedules[index].end_date,
                 schedules[index].user);
        system_manager_send_packet(response, communication);
        found = true;
    }

    if (!found)
    {
        snprintf(response, sizeof(response), "#16-%s-NONE$", system_id);
        system_manager_send_packet(response, communication);
    }
}

static void system_manager_handle_idp_21(const char *packet, comm_type communication)
{
    uint8_t idp = 0;
    char device_id[50] = {};
    time_t timestamp = 0;
    char response[100] = {};

    arg_pair_t args[] = {
        {"uint8_t", &idp},
        {"string", device_id},
        {"time_t", &timestamp},
        {NULL, NULL},
    };

    if (idp_parser_get_delimiter(packet) >= 2)
    {
        idp_parser_get_packet_data(packet, args);
        if (system_manager_device_matches(device_id) == false)
        {
            return;
        }
        rtc_app_set_timestamp(timestamp);
    }

    timestamp = rtc_app_get_timestamp(false);
    snprintf(response, sizeof(response), "#21-%s-%lld$", system_id, (long long)timestamp);
    system_manager_send_packet(response, communication);
}

static void system_manager_handle_idp_28(const char *packet, comm_type communication)
{
    uint8_t idp = 0;
    char device_id[50] = {};
    uint8_t delimiter_count = idp_parser_get_delimiter(packet);

    arg_pair_t args[] = {
        {"uint8_t", &idp},
        {"string", device_id},
        {NULL, NULL},
    };

    if (delimiter_count > 1)
    {
        system_manager_send_error("idp_28_invalid_format", communication);
        return;
    }

    idp_parser_get_packet_data(packet, args);

    if (system_manager_device_matches(device_id) == false)
    {
        return;
    }

    system_manager_send_shutdown_reason(communication);
}

static void system_manager_handle_idp_29(const char *packet, comm_type communication)
{
    UNUSED(packet);
    UNUSED(communication);
    LOG_WARNING(SYSTEM_MANAGER_TAG, "COMM", "Ignoring inbound IDP29: progress telemetry is output-only");
}

static void system_manager_handle_idp_31(const char *packet, comm_type communication)
{
    uint8_t idp = 0;
    char device_id[50] = {};
    comm_main_mode_config config = {};

    arg_pair_t args[] = {
        {"uint8_t", &idp},
        {"string", device_id},
        {"string", config.comm_main_mode_config},
        {NULL, NULL},
    };

    if (idp_parser_get_delimiter(packet) >= 2)
    {
        idp_parser_get_packet_data(packet, args);

        if (system_manager_device_matches(device_id) == false)
        {
            return;
        }

        if (comm_app_set_main_mode_config(config) != ESP_OK)
        {
            system_manager_send_error("idp_31_invalid_mode", communication);
            return;
        }

        data_app_save(DATA_TYPE_COMM_MAIN_MODE, &config, sizeof(config));
    }

    system_manager_send_comm_mode(communication);
}

static void system_manager_handle_idp_42(const char *packet, comm_type communication)
{
    uint8_t idp = 0;
    char device_id[50] = {};
    char heartbeat_state[20] = {};
    char response[100] = {};
    uint8_t delimiter_count = idp_parser_get_delimiter(packet);

    arg_pair_t args[] = {
        {"uint8_t", &idp},
        {"string", device_id},
        {"string", heartbeat_state},
        {NULL, NULL},
    };

    if (delimiter_count > 2)
    {
        system_manager_send_error("idp_42_invalid_format", communication);
        return;
    }

    idp_parser_get_packet_data(packet, args);
    if (!system_manager_device_matches(device_id))
    {
        return;
    }

    if (heartbeat_state[0] == '\0')
    {
        snprintf(response,
                 sizeof(response),
                 "#42-%s-%s$",
                 system_id,
                 system_monitoring_connectivity_alive() ? "ALIVE" : "NO_HEARTBEAT");
        system_manager_send_packet(response, communication);
        return;
    }

    if (strcmp(heartbeat_state, "PING") == 0)
    {
        if (communication == COMM_MQTT)
        {
            system_monitoring_heartbeat_feed("PING");
        }

        snprintf(response, sizeof(response), "#42-%s-PONG$", system_id);
        system_manager_send_packet(response, communication);
    }
    else if (strcmp(heartbeat_state, "PONG") == 0)
    {
        if (communication == COMM_MQTT)
        {
            system_monitoring_heartbeat_feed("PONG");
        }
    }
    else
    {
        system_manager_send_error("idp_42_invalid_state", communication);
    }
}

static void system_manager_handle_idp_90(comm_type communication)
{
    char packet[100] = {};

    snprintf(packet, sizeof(packet), "#90-%s-%s$", system_id, CONFIG_FW_VERSION);
    system_manager_send_packet(packet, communication);
}

static void system_manager_handle_idp_91(comm_type communication)
{
    char packet[80] = {};

    snprintf(packet, sizeof(packet), "#91-%s$", system_id);
    system_manager_send_packet(packet, communication);
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_restart();
}

static void system_manager_process_packet(const char *packet_in, comm_type communication)
{
    char packet[SYSTEM_MANAGER_PACKET_BUFFER_SIZE] = {};
    idp_type idp = IDP_INVALID;

    if (packet_in == NULL)
    {
        return;
    }

    idp = idp_parser_get(packet_in, packet);

    if (communication == COMM_MQTT)
    {
        LOG_UART_GPRS(SYSTEM_MANAGER_TAG, "RX %s", packet);
    }
    else if (communication == COMM_RF)
    {
        LOG_UART_RF(SYSTEM_MANAGER_TAG, "RX %s", packet);
    }
    else
    {
        LOG_DEFAULT(SYSTEM_MANAGER_TAG, "COMM", "RX %s", packet);
    }

    switch (idp)
    {
        case IDP_0:
            system_manager_handle_idp_0(packet, communication);
            break;
        case IDP_1:
            system_manager_handle_idp_1(packet, communication);
            break;
        case IDP_3:
            system_manager_handle_idp_3(packet, communication);
            break;
        case IDP_13:
            system_manager_handle_idp_13(packet, communication);
            break;
        case IDP_14:
            system_manager_handle_idp_14(packet, communication);
            break;
        case IDP_16:
            system_manager_handle_idp_16(packet, communication);
            break;
        case IDP_21:
            system_manager_handle_idp_21(packet, communication);
            break;
        case IDP_28:
            system_manager_handle_idp_28(packet, communication);
            break;
        case IDP_29:
            system_manager_handle_idp_29(packet, communication);
            break;
        case IDP_31:
            system_manager_handle_idp_31(packet, communication);
            break;
        case IDP_42:
            system_manager_handle_idp_42(packet, communication);
            break;
        case IDP_90:
            system_manager_handle_idp_90(communication);
            break;
        case IDP_91:
            system_manager_handle_idp_91(communication);
            break;
        default:
            system_manager_send_error("unsupported_idp", communication);
            break;
    }
}

static void system_manager_process_rx_buffer(char *rx_buffer,
                                             size_t *rx_buffer_len,
                                             const char *chunk,
                                             comm_type communication)
{
    size_t chunk_len = 0;

    if (rx_buffer == NULL || rx_buffer_len == NULL || chunk == NULL)
    {
        return;
    }

    chunk_len = strlen(chunk);
    if (chunk_len == 0)
    {
        return;
    }

    if ((*rx_buffer_len + chunk_len) >= SYSTEM_MANAGER_RX_BUFFER_SIZE)
    {
        *rx_buffer_len = 0;
        rx_buffer[0] = '\0';
        system_manager_send_error("rx_buffer_overflow", communication);
        return;
    }

    memcpy(&rx_buffer[*rx_buffer_len], chunk, chunk_len);
    *rx_buffer_len += chunk_len;
    rx_buffer[*rx_buffer_len] = '\0';

    while (*rx_buffer_len > 0)
    {
        char *start = memchr(rx_buffer, '#', *rx_buffer_len);
        char *end = NULL;
        size_t packet_len = 0;
        size_t remaining_len = 0;

        if (start == NULL)
        {
            *rx_buffer_len = 0;
            rx_buffer[0] = '\0';
            return;
        }

        if (start != rx_buffer)
        {
            size_t discard_len = (size_t)(start - rx_buffer);
            *rx_buffer_len -= discard_len;
            memmove(rx_buffer, start, *rx_buffer_len);
            rx_buffer[*rx_buffer_len] = '\0';
        }

        end = memchr(rx_buffer, '$', *rx_buffer_len);
        if (end == NULL)
        {
            return;
        }

        packet_len = (size_t)(end - rx_buffer) + 1U;
        if (packet_len < SYSTEM_MANAGER_PACKET_BUFFER_SIZE)
        {
            char packet[SYSTEM_MANAGER_PACKET_BUFFER_SIZE] = {};
            memcpy(packet, rx_buffer, packet_len);
            packet[packet_len] = '\0';
            system_manager_process_packet(packet, communication);
        }
        else
        {
            system_manager_send_error("packet_too_long", communication);
        }

        remaining_len = *rx_buffer_len - packet_len;
        memmove(rx_buffer, &rx_buffer[packet_len], remaining_len);
        *rx_buffer_len = remaining_len;
        rx_buffer[*rx_buffer_len] = '\0';
    }
}

static void system_manager_comm_callback(const char *buffer_request, comm_type communication)
{
    if (communication == COMM_RF)
    {
        system_manager_process_rx_buffer(system_manager_rf_rx_buffer,
                                         &system_manager_rf_rx_len,
                                         buffer_request,
                                         communication);
    }
    else
    {
        system_manager_process_rx_buffer(system_manager_mqtt_rx_buffer,
                                         &system_manager_mqtt_rx_len,
                                         buffer_request,
                                         communication);
    }
}

static bool system_manager_actions_request_start(const actuation_actions *actions)
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

static void system_manager_publish_boot_shutdown_reason(void)
{
    esp_reset_reason_t reset_reason = esp_reset_reason();
    actuation_actions actions = {};
    bool actions_loaded = (data_app_load(DATA_TYPE_ACTUATION_ACTIONS, &actions) == ESP_OK);
    bool was_commanded_on = actions_loaded && system_manager_actions_request_start(&actions);

    if (reset_reason == ESP_RST_BROWNOUT || was_commanded_on)
    {
        actuation_shutdown_info info = {
            .reason = (reset_reason == ESP_RST_BROWNOUT)
                ? ACTUATION_SHUTDOWN_REASON_BROWNOUT
                : ACTUATION_SHUTDOWN_REASON_BOOT,
            .motor = 0,
        };

        if (actions_loaded && actions.user[0] != '\0')
        {
            strncpy(info.user, actions.user, sizeof(info.user) - 1U);
        }
        else
        {
            strncpy(info.user, "unknown", sizeof(info.user) - 1U);
        }

        strncpy(info.phase,
                was_commanded_on ? "was_commanded_on" : "boot",
                sizeof(info.phase) - 1U);

        system_manager_record_shutdown_info(&info, system_manager_reset_reason_to_string(reset_reason));
    }
    else if (data_app_get_data_size(SYSTEM_MANAGER_SHUTDOWN_REASON_KEY) == 0)
    {
        actuation_shutdown_info info = {
            .reason = ACTUATION_SHUTDOWN_REASON_BOOT,
            .motor = 0,
        };

        strncpy(info.user, "unknown", sizeof(info.user) - 1U);
        strncpy(info.phase, "boot", sizeof(info.phase) - 1U);
        system_manager_record_shutdown_info(&info, system_manager_reset_reason_to_string(reset_reason));
    }

    system_manager_send_shutdown_reason(comm_main_mode);
}

/**
 * @brief Initializes only the local services required by the new product.
 *
 * Communication with the HTTP/app layer and the old pivot business rules are
 * intentionally not started in this phase.
 */
void system_manager_init(void)
{
    ESP_ERROR_CHECK(rtc_app_init());
    ESP_ERROR_CHECK(data_app_init());
    system_manager_load_comm_mode();

    actuation_config config = {};
    bool config_changed = false;

    if (system_manager_load_actuation_config(&config) == false)
    {
        config_changed = true;
    }

    config_changed |= system_manager_sanitize_actuation_config(&config);

    if (config_changed)
    {
        data_app_save(DATA_TYPE_ACTUATION_CONFIG, &config, sizeof(config));
    }

    ESP_ERROR_CHECK(actuation_app_set_config(config));
    actuation_app_set_progress_mode(comm_main_mode);
    ESP_ERROR_CHECK(actuation_app_init(system_manager_actuation_send_callback));
    ESP_ERROR_CHECK(comm_app_init(system_manager_comm_callback));
    system_manager_comm_ready = true;
    ESP_ERROR_CHECK(scheduling_init(system_manager_schedule_event_callback));
    ESP_ERROR_CHECK(system_monitoring_init(system_manager_monitoring_send_callback));
    system_manager_publish_boot_shutdown_reason();

    actuation_status status = {};
    actuation_app_get_status(&status, sizeof(status));
    system_manager_log_status(&status);

    LOG_SUCCESS(SYSTEM_MANAGER_TAG,
                "SYSTEM",
                "New product initialized: actuation, date scheduling and UART heartbeat active");
}
