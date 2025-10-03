/**
 * @file pluviometer_app.c
 * @brief Pluviometer application implementation for managing rainfall data (new daily struct).
 */

/* Self include */
#include "pluviometer_app.h"
#include "data_app.h"
#include "rtc_app.h"
#include "gpio_actuator.h"

#include "FreeRTOS_defines.h"
#include "log.h"

#include <string.h>

/** @brief Log tag for this module. */
#define PLUVIOMETER_TAG "pluviometer_app"

/** @brief Spinlock/mutex for ISR-safe access to rain pulse counters. */
static portMUX_TYPE rain_sensor_mux = portMUX_INITIALIZER_UNLOCKED;

/** @brief Daily rainfall record; currently accumulates into rain_hour[0] as requested. */
static rain_per_day_data g_rain_day = {0};

/** @brief Millimeters per sensor pulse (mm/pulse). */
static float rain_per_pulse = 0.1f;

/** @brief When true, reloads rain_per_pulse from NVS on the next loop. */
static bool rain_per_pulse_flag = false;

/**
 * @brief Save the daily rainfall record to NVS.
 * @param data Pointer to the daily record to persist.
 * @return ESP_OK on success, error code otherwise.
 */
static esp_err_t save_rain_daily(const rain_per_day_data *data)
{
    return data_app_save(DATA_TYPE_RAINFALL_DAILY, data, sizeof(*data));
}

/**
 * @brief Load the daily rainfall record from NVS.
 * @param data Output pointer to receive the daily record.
 * @return ESP_OK on success, error code otherwise.
 */
static esp_err_t load_rain_daily(rain_per_day_data *data)
{
    return data_app_load(DATA_TYPE_RAINFALL_DAILY, data);
}

/**
 * @brief Request a refresh of the mm-per-pulse calibration from NVS.
 * @param flag Set true to refresh on next loop iteration; false to ignore.
 */
void set_rain_per_pulse_flag(bool flag)
{
    rain_per_pulse_flag = flag;
}

/**
 * @brief Initialize daily rainfall state from NVS.
 *        If not found, zeroes the structure and sets date_day to "DD/MM/YYYY".
 */
void system_monitoring_init_rainfall_data(void)
{
    // tenta carregar o dia salvo
    esp_err_t err = load_rain_daily(&g_rain_day);
    if (err != ESP_OK)
    {
        memset(&g_rain_day, 0, sizeof(g_rain_day));
        strncpy(g_rain_day.date_day, "DD/MM/YYYY", sizeof(g_rain_day.date_day) - 1);
        g_rain_day.date_day[sizeof(g_rain_day.date_day) - 1] = '\0';
        ESP_LOGI(PLUVIOMETER_TAG, "No daily rainfall found, starting fresh.");
    }

    // carrega mm por pulso
    float nvs_rain_per_pulse = 0.0f;
    err = data_app_load(DATA_TYPE_RAIN_PER_PULSE, &nvs_rain_per_pulse);
    if (err == ESP_OK && nvs_rain_per_pulse > 0.0f && nvs_rain_per_pulse <= 10.0f)
    {
        rain_per_pulse = nvs_rain_per_pulse;
    }
    else
    {
        rain_per_pulse = 0.1f;
        ESP_LOGW(PLUVIOMETER_TAG, "RAIN_PER_PULSE not found/invalid. Using default: %.2f", rain_per_pulse);
    }
}

/**
 * @brief FreeRTOS task: accumulates rainfall from pulses and periodically saves to NVS.
 *
 * Logic:
 * - Converts pulses to mm and accumulates a temporary total via get_rain_total()/set_rain_total().
 * - On save interval and if there was rain, adds the amount to g_rain_day.rain_hour[0],
 *   recomputes daily_total, and persists the record.
 * - Preserves existing shutdown-by-rain behavior.
 *
 * @param arg Unused.
 */
void system_monitoring_rainfall_task(void *arg)
{
    TickType_t last_wake_time = xTaskGetTickCount();
    TickType_t last_save_time = last_wake_time;
    const TickType_t save_interval = pdMS_TO_TICKS(RAINFALL_SAVE_INTERVAL_MS);

    pivot_actions actions = {};
    float rain_shutdown_value = 0.0f;

    system_monitoring_init_rainfall_data();

    while (1)
    {
        // atualização de calibração sob demanda
        if (rain_per_pulse_flag)
        {
            float nvs_rain_per_pulse = 0.1f;
            if (data_app_load(DATA_TYPE_RAIN_PER_PULSE, &nvs_rain_per_pulse) == ESP_OK &&
                nvs_rain_per_pulse > 0.0f && nvs_rain_per_pulse <= 10.0f)
            {
                rain_per_pulse = nvs_rain_per_pulse;
            }
            else
            {
                ESP_LOGW(PLUVIOMETER_TAG, "Failed to load RAIN_PER_PULSE. Keeping: %.2f", rain_per_pulse);
            }
            rain_per_pulse_flag = false;
        }

        /* Convert pulses to rainfall and accumulate into a temporary total */
        gpio_rain_sensor_calculate_rainfall();

        float rain_total = get_rain_total(); // accumulated since last reset

        /* Optional shutdown logic based on accumulated rain */
        if (rain_total > 0.0f)
        {
            data_app_load(DATA_TYPE_RAIN_SHUTDOWN_VALUE, &rain_shutdown_value);
            data_app_load(DATA_TYPE_ACTIONS, &actions);

            if (actions.power_state == PIVOT_ON &&
                actions.watering_state == PIVOT_WET &&
                rain_shutdown_value != 0.0f)
            {
                if (rain_total >= rain_shutdown_value)
                {
                    gpio_actuator_shutdown();
                }
            }
        }

        /* Periodic save to NVS */
        if ((xTaskGetTickCount() - last_save_time) >= save_interval)
        {
            if (rain_total > 0.0f)
            {
                // Accumulate into bin 0 as requested
                g_rain_day.rain_hour[0] += rain_total;

                // Recompute daily total
                float total = 0.0f;
                for (int i = 0; i < 24; ++i)
                    total += g_rain_day.rain_hour[i];
                g_rain_day.daily_total = total;

                // Persist
                if (save_rain_daily(&g_rain_day) != ESP_OK)
                {
                    ESP_LOGE(PLUVIOMETER_TAG, "Failed to save daily rainfall.");
                }
                else
                {
                    ESP_LOGI(PLUVIOMETER_TAG, "Daily rainfall saved (bin 0 += %.2f mm).", rain_total);
                }

                // Reset temporary accumulator
                set_rain_total(0.0f);
            }

            last_save_time = xTaskGetTickCount();
        }

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(500));
    }
}

/**
 * @brief Convert accumulated pulses to millimeters and add to the temporary total.
 *
 * Thread-safe with a critical section:
 * - Reads and clears the pulse count.
 * - Adds (pulses * rain_per_pulse) to the current temporary total.
 */
void gpio_rain_sensor_calculate_rainfall(void)
{
    taskENTER_CRITICAL(&rain_sensor_mux);

    float pulses = (float)get_rain_pulse();
    set_rain_pulse(0);

    taskEXIT_CRITICAL(&rain_sensor_mux);

    float interval_rain = pulses * rain_per_pulse;

    // accumulate into the temporary total
    float acc = get_rain_total();
    acc += interval_rain;
    set_rain_total(acc);
}
