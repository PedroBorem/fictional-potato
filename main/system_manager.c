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

#include "esp_err.h"
#include "esp_system.h"

#include <stdio.h>
#include <string.h>

/**
 * @brief Log tag for the system manager module.
 */
#define SYSTEM_MANAGER_TAG "system_manager"
#define SYSTEM_MANAGER_RX_BUFFER_SIZE (512)
#define SYSTEM_MANAGER_PACKET_BUFFER_SIZE (220)

uint16_t global_pressure = 0;
uint16_t global_angle = CONFIG_ACTIONS_UNDEF_VALUE;
comm_type comm_main_mode = COMM_RF;
char system_id[50] = "new_product";
uint32_t counter_reading_panel_off = NO_MANUAL_READING;

static bool system_manager_comm_ready = false;
static char system_manager_rf_rx_buffer[SYSTEM_MANAGER_RX_BUFFER_SIZE] = {};
static size_t system_manager_rf_rx_len = 0;
static char system_manager_mqtt_rx_buffer[SYSTEM_MANAGER_RX_BUFFER_SIZE] = {};
static size_t system_manager_mqtt_rx_len = 0;

static void system_manager_log_status(const actuation_status *status)
{
    if (status == NULL)
    {
        return;
    }

    ESP_LOGI(SYSTEM_MANAGER_TAG,
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

    if (config->read_time_sec == 0)
    {
        config->read_time_sec = CONFIG_ACTUATION_DEFAULT_READ_TIME_SEC;
        changed = true;
    }

    if (config->status_active_level != CONFIG_ACTUATION_DEFAULT_STATUS_ACTIVE_LEVEL)
    {
        config->status_active_level = CONFIG_ACTUATION_DEFAULT_STATUS_ACTIVE_LEVEL;
        changed = true;
    }

    if (config->stage_1_delay_sec == 0)
    {
        config->stage_1_delay_sec = CONFIG_PUMP_STAGE_1_DELAY_MS / 1000U;
        changed = true;
    }

    if (config->stage_2_delay_sec == 0)
    {
        config->stage_2_delay_sec = CONFIG_PUMP_STAGE_2_DELAY_MS / 1000U;
        changed = true;
    }

    if (config->stage_3_delay_sec == 0)
    {
        config->stage_3_delay_sec = CONFIG_PUMP_STAGE_3_DELAY_MS / 1000U;
        changed = true;
    }

    if (config->status_publish_time_sec == 0)
    {
        config->status_publish_time_sec = CONFIG_ACTUATION_DEFAULT_READ_TIME_SEC;
        changed = true;
    }

    return changed;
}

static void system_manager_load_comm_mode(void)
{
    pivot_comm_main_mode_config config = {};
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
    char response[120] = {};
    uint8_t status_active_level = 0;

    arg_pair_t args[] = {
        {"uint8_t", &idp},
        {"string", device_id},
        {"uint16_t", &config.relay_pulse_time_ms},
        {"uint8_t", &config.read_time_sec},
        {"uint8_t", &status_active_level},
        {"uint16_t", &config.stage_1_delay_sec},
        {"uint16_t", &config.stage_2_delay_sec},
        {"uint16_t", &config.stage_3_delay_sec},
        {"uint16_t", &config.status_publish_time_sec},
        {NULL, NULL},
    };

    uint8_t delimiter_count = idp_parser_get_delimiter(packet);

    if (delimiter_count != 0 && delimiter_count != 1 && delimiter_count < 4)
    {
        system_manager_send_error("idp_3_invalid_format", communication);
        return;
    }

    idp_parser_get_packet_data(packet, args);

    if (system_manager_device_matches(device_id) == false)
    {
        return;
    }

    if (delimiter_count >= 4)
    {
        if (config.relay_pulse_time_ms == 0 || config.read_time_sec == 0 || status_active_level > 1)
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
        if (data_app_load(DATA_TYPE_ACTUATION_CONFIG, &config) != ESP_OK)
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
             "#03-%s-%u-%u-%u-%u-%u-%u-%u$",
             system_id,
             (unsigned int)config.relay_pulse_time_ms,
             (unsigned int)config.read_time_sec,
             (unsigned int)(config.status_active_level ? 1 : 0),
             (unsigned int)config.stage_1_delay_sec,
             (unsigned int)config.stage_2_delay_sec,
             (unsigned int)config.stage_3_delay_sec,
             (unsigned int)config.status_publish_time_sec);
    system_manager_send_packet(response, communication);
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

static void system_manager_handle_idp_31(const char *packet, comm_type communication)
{
    uint8_t idp = 0;
    char device_id[50] = {};
    pivot_comm_main_mode_config config = {};

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

static void system_manager_handle_idp_92(comm_type communication)
{
    char packet[80] = {};

    snprintf(packet, sizeof(packet), "#92-%s$", system_id);
    system_manager_send_packet(packet, communication);
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

    ESP_LOGI(SYSTEM_MANAGER_TAG, "RX %s", packet);

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
        case IDP_21:
            system_manager_handle_idp_21(packet, communication);
            break;
        case IDP_31:
            system_manager_handle_idp_31(packet, communication);
            break;
        case IDP_90:
            system_manager_handle_idp_90(communication);
            break;
        case IDP_91:
            system_manager_handle_idp_91(communication);
            break;
        case IDP_92:
            system_manager_handle_idp_92(communication);
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

    if (data_app_load(DATA_TYPE_ACTUATION_CONFIG, &config) != ESP_OK)
    {
        config_changed = true;
    }

    config_changed |= system_manager_sanitize_actuation_config(&config);

    if (config_changed)
    {
        data_app_save(DATA_TYPE_ACTUATION_CONFIG, &config, sizeof(config));
    }

    ESP_ERROR_CHECK(actuation_app_set_config(config));
    ESP_ERROR_CHECK(actuation_app_init(system_manager_comm_callback));
    ESP_ERROR_CHECK(comm_app_init(system_manager_comm_callback));
    system_manager_comm_ready = true;

    actuation_status status = {};
    actuation_app_get_status(&status, sizeof(status));
    system_manager_log_status(&status);

    ESP_LOGI(SYSTEM_MANAGER_TAG, "New product initialized: 4 relay pairs and 4 ON/OFF status inputs");
}
