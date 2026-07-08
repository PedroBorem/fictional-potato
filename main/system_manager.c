/**
 * @file system_manager.c
 * @brief System bootstrap for the new four-channel actuation product.
 */

#include "system_manager.h"

#include "actuation_app.h"
#include "data_app.h"
#include "log.h"
#include "project_config.h"
#include "rtc_app.h"

#include "esp_err.h"

/**
 * @brief Log tag for the system manager module.
 */
#define SYSTEM_MANAGER_TAG "system_manager"

uint16_t global_pressure = 0;
uint16_t global_angle = CONFIG_ACTIONS_UNDEF_VALUE;
comm_type comm_main_mode = COMM_MQTT;
char system_id[50] = "new_product";
uint32_t counter_reading_panel_off = NO_MANUAL_READING;

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

    actuation_config config = {};
    bool config_changed = false;

    if (data_app_load(DATA_TYPE_ACTUATION_CONFIG, &config) != ESP_OK)
    {
        config.relay_pulse_time_ms = CONFIG_ACTUATION_DEFAULT_RELAY_PULSE_MS;
        config.read_time_sec = CONFIG_ACTUATION_DEFAULT_READ_TIME_SEC;
        config.status_active_level = CONFIG_ACTUATION_DEFAULT_STATUS_ACTIVE_LEVEL;
        config_changed = true;
    }

    if (config.status_active_level != CONFIG_ACTUATION_DEFAULT_STATUS_ACTIVE_LEVEL)
    {
        config.status_active_level = CONFIG_ACTUATION_DEFAULT_STATUS_ACTIVE_LEVEL;
        config_changed = true;
    }

    if (config_changed)
    {
        data_app_save(DATA_TYPE_ACTUATION_CONFIG, &config, sizeof(config));
    }

    ESP_ERROR_CHECK(actuation_app_set_config(config));
    ESP_ERROR_CHECK(actuation_app_init(NULL));

    actuation_status status = {};
    actuation_app_get_status(&status, sizeof(status));
    system_manager_log_status(&status);

    ESP_LOGI(SYSTEM_MANAGER_TAG, "New product initialized: 4 relay pairs and 4 ON/OFF status inputs");
}
