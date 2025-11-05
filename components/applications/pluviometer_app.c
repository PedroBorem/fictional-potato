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
#include <time.h>

/** @brief Log tag for this module. */
#define PLUVIOMETER_TAG "pluviometer_app"

#define SECS_PER_HOUR 3600UL
#define SECS_PER_DAY 86400UL
#define HOURS_PER_DAY 24U

/** @brief Spinlock/mutex for ISR-safe access to rain pulse counters. */
static portMUX_TYPE rain_sensor_mux = portMUX_INITIALIZER_UNLOCKED;

/** @brief Mutex for hour accumulator (rain_total) snapshot/reset and updates. */
static portMUX_TYPE rain_total_mux = portMUX_INITIALIZER_UNLOCKED;

/** @brief Daily rainfall record; data is distributed by hour index (g_hour_idx). */
static rain_per_day_data g_rain_day = {0};

/** @brief Current hour bin index (0..23). Anchored to RTC hour (not timer-driven). */
static uint8_t g_hour_idx = 0;

/** @brief Millimeters per sensor pulse (mm/pulse). */
static float rain_per_pulse = 0.1f;

/** @brief When true, reloads rain_per_pulse from NVS on the next loop. */
static bool rain_per_pulse_flag = false;

/** @brief Último índice horário detectado via RTC (0..23). 0xFF = indefinido/inicial. */
static uint8_t s_last_hour_idx = 0xFF;

static void update_day_string_from_ts(time_t ts);

/**
 * @brief Get the rain calibration (mm per pulse).
 * @return Current calibration in mm/pulse.
 */
float get_rain_per_pulse(void)
{
    return rain_per_pulse;
}

/**
 * @brief Set the rain calibration value (mm per pulse).
 * @param value Calibration in mm/pulse.
 */
void set_rain_per_pulse(float value)
{
    rain_per_pulse = value;
}

/**
 * @brief Save the finished day (yesterday) to NVS for #41 packet.
 * @param data Pointer to the finalized daily record (the day that just ended).
 * @return ESP_OK on success, error code otherwise.
 */
static esp_err_t save_rain_yesterday(const rain_per_day_data *data)
{
    // DATA_TYPE_RAINFALL_DAILY_YESTERDAY deve existir no seu data_app.h
    return data_app_save(DATA_TYPE_RAINFALL_DAILY_YESTERDAY, data, sizeof(*data));
}

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
 *        Also resets the hour index to 0.
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

    // índice horário inicia pela hora 'real' local (via RTC)
    time_t now_ts = rtc_app_get_timestamp(true); // true: horário local
    if (now_ts > 0)
    {
        uint32_t sod = (uint32_t)(now_ts % SECS_PER_DAY);
        g_hour_idx = (uint8_t)(sod / SECS_PER_HOUR); // 0..23
        s_last_hour_idx = g_hour_idx;                // inicializa o acompanhamento
        update_day_string_from_ts(now_ts);
    }
    else
    {
        g_hour_idx = 0;
        s_last_hour_idx = 0xFF; // primeiro timestamp válido ajustará isso
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
 * @brief Update g_rain_day.date_day (DD/MM/YYYY) from a timestamp (local time).
 */
static void update_day_string_from_ts(time_t ts)
{
    struct tm lt;
    localtime_r(&ts, &lt);
    strftime(g_rain_day.date_day, sizeof(g_rain_day.date_day), "%d/%m/%Y", &lt);
}

/**
 * @brief FreeRTOS task: accumulates rainfall from pulses and writes hourly bins anchored to RTC.
 *
 * - Pulses → mm are accumulated in a temporary “current-hour” bucket.
 * - On RTC hour change (00..23), we atomically snapshot+reset the accumulator
 *   and store the value into g_rain_day.rain_per_hour[hour_just_finished].
 * - We always persist to NVS (including 0.0 mm) and recompute the daily total.
 * - If hours were skipped within the same day, we fill missed bins with 0.0 mm.
 * - On day rollover (23→0 or any midnight crossing), we finalize the previous day
 *   and clear the structure for the new day (date string updated from RTC).
 */

void system_monitoring_rainfall_task(void *arg)
{
    // Periodic loop ticks; used with vTaskDelayUntil for stable cadence.
    TickType_t last_wake_time = xTaskGetTickCount();

    // Optional rain-based shutdown parameters
    pivot_actions actions = {};
    float rain_shutdown_value = 0.0f;

    // Throttle for repeated warnings when RTC is invalid
    static TickType_t s_last_rtc_warn_tick = 0;

    ESP_LOGI(PLUVIOMETER_TAG, "Starting rainfall monitoring task (hourly RTC-anchored windows).");

    system_monitoring_init_rainfall_data();

    while (1)
    {
        if (rain_per_pulse_flag)
        {
            float nvs_rain_per_pulse = 0.1f;
            esp_err_t err = data_app_load(DATA_TYPE_RAIN_PER_PULSE, &nvs_rain_per_pulse);
            if (err == ESP_OK && nvs_rain_per_pulse > 0.0f && nvs_rain_per_pulse <= 10.0f)
            {
                ESP_LOGI(PLUVIOMETER_TAG, "Updated RAIN_PER_PULSE from NVS: %.3f mm/pulse (prev: %.3f).",
                         nvs_rain_per_pulse, rain_per_pulse);
                rain_per_pulse = nvs_rain_per_pulse;
            }
            else
            {
                ESP_LOGW(PLUVIOMETER_TAG,
                         "Failed to load RAIN_PER_PULSE (err=%d or invalid %.3f). Keeping current: %.3f.",
                         (int)err, nvs_rain_per_pulse, rain_per_pulse);
            }
            rain_per_pulse_flag = false;
        }

        gpio_rain_sensor_calculate_rainfall();
        float rain_total = get_rain_total();

        if (rain_total > 0.0f)
        {
            (void)data_app_load(DATA_TYPE_RAIN_SHUTDOWN_VALUE, &rain_shutdown_value);
            (void)data_app_load(DATA_TYPE_ACTIONS, &actions);

            if (actions.power_state == PIVOT_ON &&
                actions.watering_state == PIVOT_WET &&
                rain_shutdown_value > 0.0f &&
                rain_total >= rain_shutdown_value)
            {
                ESP_LOGW(PLUVIOMETER_TAG,
                         "Rain shutdown triggered: rain_total=%.2f mm >= threshold=%.2f mm. Requesting actuator OFF.",
                         rain_total, rain_shutdown_value);
                gpio_actuator_shutdown();
            }
        }

        time_t now_ts = rtc_app_get_timestamp(true);
        if (now_ts > 0)
        {
            uint32_t sod = (uint32_t)(now_ts % SECS_PER_DAY);
            uint8_t curr_hour_idx = (uint8_t)(sod / SECS_PER_HOUR);

            if (s_last_hour_idx == 0xFF)
            {
                s_last_hour_idx = curr_hour_idx;
                g_hour_idx = curr_hour_idx;
                ESP_LOGI(PLUVIOMETER_TAG,
                         "Initialized hour index from RTC: hour=%u (sod=%u).",
                         (unsigned)curr_hour_idx, (unsigned)sod);
            }

            else if (curr_hour_idx != s_last_hour_idx)
            {
                uint8_t step = (uint8_t)((curr_hour_idx + 24 - s_last_hour_idx) % 24);
                bool rolled_day = (curr_hour_idx < s_last_hour_idx); // crossed midnight if true

                if (rolled_day && step > 1)
                {
                    uint8_t start = (uint8_t)(s_last_hour_idx + 1); // next hour in the previous day
                    for (uint8_t h = start; h <= 23; ++h)
                    {
                        g_rain_day.rain_per_hour[h] = 0.0f;
                    }
                }

                if (step != 1)
                {
                    ESP_LOGW(PLUVIOMETER_TAG,
                             "Hour jump detected: last=%u -> current=%u (step=%u). "
                             "Hour jump detected; same-day missed bins set to 0.0 mm (previous-day remainder handled if midnight crossed).",
                             (unsigned)s_last_hour_idx, (unsigned)curr_hour_idx, (unsigned)step);
                }
                if (step > 1 && !rolled_day)
                {
                    for (uint8_t k = 1; k < step; ++k)
                    {
                        uint8_t missed = (uint8_t)((s_last_hour_idx + k) % 24);
                        if (missed != curr_hour_idx)
                        {
                            g_rain_day.rain_per_hour[missed] = 0.0f;
                        }
                    }
                }

                bool new_day = (curr_hour_idx == 0 && s_last_hour_idx == 23);

                // Atomic snapshot + reset of the hourly accumulator
                float snapshot = 0.0f;
                taskENTER_CRITICAL(&rain_total_mux);
                snapshot = get_rain_total();
                set_rain_total(0.0f);
                taskEXIT_CRITICAL(&rain_total_mux);

                uint8_t bin_to_write = s_last_hour_idx;
                g_rain_day.rain_per_hour[bin_to_write] = snapshot;

                float total = 0.0f;
                for (int i = 0; i < 24; ++i)
                    total += g_rain_day.rain_per_hour[i];
                g_rain_day.daily_total = total;

                esp_err_t err = save_rain_daily(&g_rain_day);
                if (err != ESP_OK)
                {
                    ESP_LOGE(PLUVIOMETER_TAG,
                             "Failed to save daily rainfall (hour=%u, value=%.2f mm). err=%d",
                             (unsigned)bin_to_write, snapshot, (int)err);
                }
                else
                {
                    ESP_LOGI(PLUVIOMETER_TAG,
                             "Saved hour %u = %.2f mm. Daily total now: %.2f mm.",
                             (unsigned)bin_to_write, snapshot, g_rain_day.daily_total);
                }

                ESP_LOGI(PLUVIOMETER_TAG,
                         "Reset hourly accumulator; new active hour=%u.",
                         (unsigned)curr_hour_idx);

                if (new_day || rolled_day)
                {
                    {
                        rain_per_day_data yesterday = g_rain_day; 
                        esp_err_t aerr = save_rain_yesterday(&yesterday);
                        if (aerr != ESP_OK)
                        {
                            ESP_LOGE(PLUVIOMETER_TAG,
                                    "Failed to archive yesterday rainfall (date=%s). err=%d",
                                    yesterday.date_day, (int)aerr);
                        }
                        else
                        {
                            ESP_LOGI(PLUVIOMETER_TAG,
                                    "Archived yesterday rainfall (date=%s, total=%.2f mm).",
                                    yesterday.date_day, yesterday.daily_total);
                        }
                    }

                    ESP_LOGI(PLUVIOMETER_TAG, "New day rollover detected. Clearing daily structure.");
                    memset(g_rain_day.rain_per_hour, 0, sizeof(g_rain_day.rain_per_hour));
                    g_rain_day.daily_total = 0.0f;

                    update_day_string_from_ts(now_ts);

                    (void)save_rain_daily(&g_rain_day);
                }


                s_last_hour_idx = curr_hour_idx;
                g_hour_idx = curr_hour_idx;
            }
        }
        else
        {
            TickType_t now_tick = xTaskGetTickCount();
            if ((now_tick - s_last_rtc_warn_tick) > pdMS_TO_TICKS(60000))
            {
                s_last_rtc_warn_tick = now_tick;
                ESP_LOGW(PLUVIOMETER_TAG,
                         "RTC timestamp invalid (<= 0). Skipping hour closure until RTC becomes valid.");
            }
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

    // accumulate into the current hour bucket (atomic with respect to hour rollover)
    taskENTER_CRITICAL(&rain_total_mux);
    float acc = get_rain_total();
    acc += interval_rain;
    set_rain_total(acc);
    taskEXIT_CRITICAL(&rain_total_mux);
}
