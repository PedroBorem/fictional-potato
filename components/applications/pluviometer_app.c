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
#include "esp_err.h"

#include <string.h>
#include <time.h>

/** @brief Log tag for this module. */
#define PLUVIOMETER_TAG "pluviometer_app"

/* ===== Organization/consistency macros =================================== */
#ifndef MAYBE_UNUSED
#define MAYBE_UNUSED __attribute__((unused))
#endif

/* Calibration constraints (avoid magic numbers) */
#define RAIN_PER_PULSE_MIN_MM (0.0f)
#define RAIN_PER_PULSE_MAX_MM (10.0f)
#define RAIN_PER_PULSE_DEFAULT_MM (0.1f)

/* Time constants (keep only what is used here) */
#define SECS_PER_HOUR 3600UL
#define SECS_PER_DAY 86400UL
/* #define HOURS_PER_DAY 24U  // (unused here) */

/** @brief Optional callback for actuation/events integration. */
static MAYBE_UNUSED app_callback pluviometer_app_call = NULL;

/** @brief Spinlock/mutex for ISR-safe access to rain pulse counters. */
static portMUX_TYPE rain_sensor_mux = portMUX_INITIALIZER_UNLOCKED;

/** @brief Mutex for hour accumulator (rain_total) snapshot/reset and updates. */
static portMUX_TYPE rain_total_mux = portMUX_INITIALIZER_UNLOCKED;

/** @brief Spinlock to protect read/write access to g_rain_day structure. */
static portMUX_TYPE g_rain_day_mux = portMUX_INITIALIZER_UNLOCKED;

/** @brief Daily rainfall record; data is distributed by hour index (g_hour_idx). */
static rain_per_day_data g_rain_day = {0};

/** @brief Current hour bin index (0..23). Anchored to RTC hour (not timer-driven). */
static uint8_t g_hour_idx = 0;

/** @brief Millimeters per sensor pulse (mm/pulse). */
static float rain_per_pulse = RAIN_PER_PULSE_DEFAULT_MM;

/** @brief When true, reloads rain_per_pulse from NVS on the next loop. */
static bool rain_per_pulse_flag = false;

/** @brief Last hour index detected from RTC (0..23). 0xFF = undefined/uninitialized. */
static uint8_t s_last_hour_idx = 0xFF;

static void update_day_string_from_ts(time_t ts);

/** @brief Accumulator for the current hour's rainfall (mm). */
static float s_rain_total_mm = 0.0f;

/**
 * @brief Get the current hour's accumulated rainfall.
 * @note Must be called from within a 'rain_total_mux' critical section.
 * @return Rain total in mm.
 */
float get_rain_total(void)
{
    return s_rain_total_mm;
}

/**
 * @brief Set the current hour's accumulated rainfall.
 * @note Must be called from within a 'rain_total_mux' critical section.
 * @param value Rain total in mm.
 */
void set_rain_total(float value)
{
    s_rain_total_mm = value;
}

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
    /* DATA_TYPE_RAINFALL_DAILY_YESTERDAY must exist in data_app.h */
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
 * @brief Get a safe copy of the current day's data structure.
 * @note This function is thread-safe.
 * @param out_data Pointer to the structure where the data will be copied.
 * @return ESP_OK if successful.
 */
esp_err_t pluviometer_app_get_current_day(rain_per_day_data *out_data)
{
    if (out_data == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    taskENTER_CRITICAL(&g_rain_day_mux);
    *out_data = g_rain_day; /* fast memory copy */
    taskEXIT_CRITICAL(&g_rain_day_mux);

    return ESP_OK;
}

/**
 * @brief Get the index of the last closed hour (0-23).
 * @return Hour index (0-23), or 0xFF if not yet initialized.
 */
uint8_t pluviometer_app_get_last_hour_idx(void)
{
    return s_last_hour_idx;
}

/**
 * @brief Initialize daily rainfall state from NVS.
 *        If not found, zeroes the structure and sets date_day to "DD/MM/YYYY".
 *        Also sets the hour index from RTC or leaves it uninitialized (0xFF).
 */
void system_monitoring_init_rainfall_data(const app_callback callback)
{
    /* Try to load the persisted day */
    esp_err_t err = load_rain_daily(&g_rain_day);
    if (err != ESP_OK)
    {
        memset(&g_rain_day, 0, sizeof(g_rain_day));
        strncpy(g_rain_day.date_day, "DD/MM/YYYY", sizeof(g_rain_day.date_day) - 1);
        g_rain_day.date_day[sizeof(g_rain_day.date_day) - 1] = '\0';
        ESP_LOGI(PLUVIOMETER_TAG, "No daily rainfall found, starting fresh.");
    }

    /* Initialize hour state from local RTC time */
    time_t now_ts = rtc_app_get_timestamp(true); /* true: local time */
    if (now_ts > 0)
    {
        uint32_t sod = (uint32_t)(now_ts % SECS_PER_DAY);
        g_hour_idx = (uint8_t)(sod / SECS_PER_HOUR); /* 0..23 */
        s_last_hour_idx = g_hour_idx;                /* start tracking from current hour */
        update_day_string_from_ts(now_ts);
    }
    else
    {
        g_hour_idx = 0;
        s_last_hour_idx = 0xFF; /* first valid timestamp will set this */
    }

    /* Load mm per pulse */
    float nvs_rain_per_pulse = 0.0f;
    err = data_app_load(DATA_TYPE_RAIN_PER_PULSE, &nvs_rain_per_pulse);
    if ((err == ESP_OK) &&
        (nvs_rain_per_pulse > RAIN_PER_PULSE_MIN_MM) &&
        (nvs_rain_per_pulse <= RAIN_PER_PULSE_MAX_MM))
    {
        rain_per_pulse = nvs_rain_per_pulse;
    }
    else
    {
        rain_per_pulse = RAIN_PER_PULSE_DEFAULT_MM;
        ESP_LOGW(PLUVIOMETER_TAG,
                 "RAIN_PER_PULSE not found/invalid. Using default: %.2f",
                 rain_per_pulse);
    }

    pluviometer_app_call = callback; /* may be unused now; kept for future hooks */
}

/**
 * @brief Update g_rain_day.date_day (DD/MM/YYYY) from a timestamp (local time).
 */
static void update_day_string_from_ts(time_t ts)
{
    struct tm lt;
    localtime_r(&ts, &lt);
    (void)strftime(g_rain_day.date_day, sizeof(g_rain_day.date_day), "%d/%m/%Y", &lt);
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
 * @brief Refreshes the rain-per-pulse factor from NVS when the flag is set.
 *
 * Loads DATA_TYPE_RAIN_PER_PULSE from NVS into @c rain_per_pulse if the cached
 * value is valid (0.0 < mm/pulse <= 10.0). Resets @c rain_per_pulse_flag.
 * Logs both success and failure paths.
 */
static void update_rain_per_pulse_if_needed(void)
{
    if (!rain_per_pulse_flag)
    {
        return;
    }

    float nvs_rain_per_pulse = 0.1f;
    esp_err_t err = data_app_load(DATA_TYPE_RAIN_PER_PULSE, &nvs_rain_per_pulse);
    bool valid = (nvs_rain_per_pulse > 0.0f) && (nvs_rain_per_pulse <= 10.0f);

    if ((err == ESP_OK) && valid)
    {
        ESP_LOGI(PLUVIOMETER_TAG,
                 "Updated RAIN_PER_PULSE from NVS: %.3f mm/pulse (prev: %.3f).",
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

/**
 * @brief Applies automatic shutdown if cumulative hourly rain exceeds the configured threshold.
 *
 * Reads @c DATA_TYPE_RAIN_SHUTDOWN_VALUE and @c DATA_TYPE_ACTIONS. If pivot is ON and WET,
 * and the threshold is configured (>0), and @c s_rain_total_mm >= threshold, then it calls
 * @c gpio_actuator_shutdown() and logs the event.
 */
static void maybe_trigger_rain_shutdown(void)
{
    if (s_rain_total_mm <= 0.0f)
    {
        return;
    }

    float shutdown_value = 0.0f;
    pivot_actions acts = {};
    (void)data_app_load(DATA_TYPE_RAIN_SHUTDOWN_VALUE, &shutdown_value);
    (void)data_app_load(DATA_TYPE_ACTIONS, &acts);

    bool pivot_on = (acts.power_state == PIVOT_ON);
    bool pivot_wet = (acts.watering_state == PIVOT_WET);
    bool threshold_configured = (shutdown_value > 0.0f);
    bool reached_threshold = (s_rain_total_mm >= shutdown_value);

    if (pivot_on && pivot_wet && threshold_configured && reached_threshold)
    {
        ESP_LOGW(PLUVIOMETER_TAG,
                 "Rain shutdown triggered: rain_total=%.2f mm >= threshold=%.2f mm. Requesting actuator OFF.",
                 s_rain_total_mm, shutdown_value);
        gpio_actuator_shutdown();
    }
}

/**
 * @brief Reads the current hour index (0..23) from RTC and seconds-of-day.
 *
 * @param[out] out_hour  Current hour index (0..23) when RTC is valid.
 * @param[out] out_sod   Seconds-of-day (0..86399) when RTC is valid.
 * @param[out] out_now   The current timestamp (UTC seconds) when RTC is valid.
 * @return true if RTC returned a valid timestamp (>0), false otherwise.
 */
static bool rtc_get_current_hour(uint8_t *out_hour, uint32_t *out_sod, time_t *out_now)
{
    if (out_hour == NULL || out_sod == NULL || out_now == NULL)
    {
        return false;
    }

    time_t now_ts = rtc_app_get_timestamp(true);
    if (now_ts <= 0)
    {
        return false;
    }

    uint32_t sod = (uint32_t)(now_ts % SECS_PER_DAY);
    uint8_t hour = (uint8_t)(sod / SECS_PER_HOUR);

    *out_hour = hour;
    *out_sod = sod;
    *out_now = now_ts;
    return true;
}

/**
 * @brief Initializes the hour index tracking the first time RTC becomes valid.
 *
 * Sets @c s_last_hour_idx and @c g_hour_idx to the current hour and logs the initialization.
 *
 * @param curr_hour Current hour index (0..23).
 * @param sod       Seconds-of-day used only for logging.
 */
static void init_hour_index_from_rtc(uint8_t curr_hour, uint32_t sod)
{
    s_last_hour_idx = curr_hour;
    g_hour_idx = curr_hour;
    ESP_LOGI(PLUVIOMETER_TAG,
             "Initialized hour index from RTC: hour=%u (sod=%u).",
             (unsigned)curr_hour, (unsigned)sod);
}

/**
 * @brief Atomically snapshots and resets the hourly rainfall accumulator.
 *
 * Acquires @c rain_total_mux to read @c get_rain_total(), writes its value into @p out_snapshot,
 * and resets the accumulator to zero via @c set_rain_total(0.0f).
 *
 * @param[out] out_snapshot The snapshot value of the accumulator in mm.
 */
static void snapshot_and_reset_hour_accumulator(float *out_snapshot)
{
    if (out_snapshot == NULL)
    {
        return;
    }

    float snapshot = 0.0f;
    taskENTER_CRITICAL(&rain_total_mux);
    snapshot = get_rain_total();
    set_rain_total(0.0f);
    taskEXIT_CRITICAL(&rain_total_mux);

    *out_snapshot = snapshot;
}

/**
 * @brief Formats the current local date into DD/MM/YYYY string.
 *
 * @param now_ts      Current epoch seconds.
 * @param out_buf     Destination buffer to receive the date.
 * @param out_buf_len Size of @p out_buf.
 */
static void format_local_date_ddmmyyyy(time_t now_ts, char *out_buf, size_t out_buf_len)
{
    if (out_buf == NULL || out_buf_len == 0)
    {
        return;
    }

    struct tm lt;
    localtime_r(&now_ts, &lt);
    (void)strftime(out_buf, out_buf_len, "%d/%m/%Y", &lt);
}

/**
 * @brief Fills missed hourly bins inside the same day with 0.0, and the tail of the day on rollover.
 *
 * Must be called with @c g_rain_day_mux held by the caller.
 *
 * @param curr_hour     Current hour index (0..23).
 * @param last_hour     Previous hour index (0..23).
 * @param step          Forward distance from @p last_hour to @p curr_hour modulo 24.
 * @param rolled_day    True if the clock wrapped to a new day (curr_hour < last_hour).
 */
static void fill_missed_bins_zeros(uint8_t curr_hour, uint8_t last_hour, uint8_t step, bool rolled_day)
{
    if (rolled_day)
    {
        /* When day rolled and more than one hour advanced: zero the remaining hours of the previous day. */
        if (step > 1U)
        {
            uint8_t start = (uint8_t)(last_hour + 1U);
            uint8_t h = start;
            while (h <= 23U)
            {
                g_rain_day.rain_per_hour[h] = 0.0f;
                h++;
            }
        }
        return;
    }

    /* Same day, jumped multiple hours: zero each missed hour except the current one. */
    if (step > 1U)
    {
        uint8_t k = 1U;
        while (k < step)
        {
            uint8_t missed = (uint8_t)((last_hour + k) % 24U);
            if (missed != curr_hour)
            {
                g_rain_day.rain_per_hour[missed] = 0.0f;
            }
            k++;
        }
    }
}

/**
 * @brief Writes the snapshot to the specified hourly bin and recomputes daily total.
 *
 * Must be called with @c g_rain_day_mux held by the caller.
 *
 * @param bin_to_write Hourly bin index (0..23).
 * @param snapshot     Hourly rainfall in mm to write.
 */
static void write_bin_and_recompute_total(uint8_t bin_to_write, float snapshot)
{
    g_rain_day.rain_per_hour[bin_to_write] = snapshot;

    float total = 0.0f;
    int i = 0;
    while (i < 24)
    {
        total += g_rain_day.rain_per_hour[i];
        i++;
    }
    g_rain_day.daily_total = total;
}

/**
 * @brief Handles the optional "archive yesterday" step during a day rollover.
 *
 * Must be called with @c g_rain_day_mux held by the caller.
 * Copies @c g_rain_day into @p out_yesterday, then clears the current structure for the new day
 * and updates its date string.
 *
 * @param new_date_str     Date string (DD/MM/YYYY) to assign to the new day.
 * @param out_yesterday    Output copy of the previous day for archival.
 */
static void prepare_day_rollover_locked(const char *new_date_str, rain_per_day_data *out_yesterday)
{
    if (out_yesterday == NULL)
    {
        return;
    }

    *out_yesterday = g_rain_day;

    /* Clear current day for the new date. */
    memset(g_rain_day.rain_per_hour, 0, sizeof(g_rain_day.rain_per_hour));
    g_rain_day.daily_total = 0.0f;

    if (new_date_str != NULL)
    {
        size_t len = sizeof(g_rain_day.date_day);
        memcpy(g_rain_day.date_day, new_date_str, len);
    }
}

/**
 * @brief Saves daily structures and publishes packets according to rollover and snapshot.
 *
 * Persists @p yesterday_to_save when provided (rollover case) and always persists
 * @p current_day_to_save. Publishes packet #41 for daily summary when yesterday is archived,
 * and packet #40 for the hourly bin when @p snapshot > 0.0.
 *
 * @param must_archive_yesterday  True if there was a day rollover.
 * @param yesterday_to_save       Pointer to "yesterday" structure (valid if @p must_archive_yesterday is true).
 * @param current_day_to_save     Pointer to the current day structure to persist.
 * @param bin_written             The hour index that was written.
 * @param snapshot                The rainfall snapshot stored in @p bin_written.
 */
static void persist_and_publish(bool must_archive_yesterday,
                                const rain_per_day_data *yesterday_to_save,
                                const rain_per_day_data *current_day_to_save,
                                uint8_t bin_written,
                                float snapshot)
{
    if (must_archive_yesterday && (yesterday_to_save != NULL))
    {
        esp_err_t aerr = save_rain_yesterday(yesterday_to_save);
        if (aerr != ESP_OK)
        {
            ESP_LOGE(PLUVIOMETER_TAG,
                     "Failed to archive yesterday rainfall (date=%s). err=%d",
                     yesterday_to_save->date_day, (int)aerr);
        }
        else
        {
            ESP_LOGI(PLUVIOMETER_TAG,
                     "Archived yesterday rainfall (date=%s, total=%.2f mm).",
                     yesterday_to_save->date_day, yesterday_to_save->daily_total);

            ESP_LOGI(PLUVIOMETER_TAG, "Publishing daily summary packet (#41).");
            pluviometer_app_call("#41$", NULL);
        }
        ESP_LOGI(PLUVIOMETER_TAG, "New day rollover detected. Clearing daily structure.");
    }

    if (current_day_to_save != NULL)
    {
        esp_err_t err = save_rain_daily(current_day_to_save);
        if (err != ESP_OK)
        {
            ESP_LOGE(PLUVIOMETER_TAG,
                     "Failed to save daily rainfall (hour=%u, value=%.2f mm). err=%d",
                     (unsigned)bin_written, snapshot, (int)err);
        }
        else
        {
            ESP_LOGI(PLUVIOMETER_TAG,
                     "Saved hour %u = %.2f mm. Daily total now: %.2f mm.",
                     (unsigned)bin_written, snapshot, current_day_to_save->daily_total);

            if (snapshot > 0.0f)
            {
                ESP_LOGI(PLUVIOMETER_TAG, "Publishing hourly packet (#40) for hour=%u.", (unsigned)bin_written);
                pluviometer_app_call("#40$", NULL);
            }
        }
    }
}

/**
 * @brief Processes an hour change boundary: snapshot, zero missed bins, rollover if needed, persist and publish.
 *
 * This function encapsulates the full hour-boundary transition while keeping critical sections minimal.
 * It also logs hour jumps (step != 1).
 *
 * @param curr_hour_idx  Current hour index (0..23).
 * @param now_ts         Current epoch seconds.
 * @param sod            Seconds-of-day (for logging only).
 */
static void handle_hour_change(uint8_t curr_hour_idx, time_t now_ts, uint32_t sod)
{
    /* Step and rollover detection */
    uint8_t step = (uint8_t)((curr_hour_idx + 24U - s_last_hour_idx) % 24U);
    bool rolled_day = (curr_hour_idx < s_last_hour_idx);
    bool new_day_boundary = (curr_hour_idx == 0U) && (s_last_hour_idx == 23U);
    bool must_archive_yesterday = false;

    if (new_day_boundary)
    {
        must_archive_yesterday = true;
    }
    else
    {
        if (rolled_day)
        {
            must_archive_yesterday = true;
        }
    }

    /* Snapshot accumulator */
    float snapshot = 0.0f;
    snapshot_and_reset_hour_accumulator(&snapshot);

    uint8_t bin_to_write = s_last_hour_idx;

    /* If rolling over, prepare the new date string */
    char new_date_str[sizeof(g_rain_day.date_day)] = {0};
    if (must_archive_yesterday)
    {
        format_local_date_ddmmyyyy(now_ts, new_date_str, sizeof(new_date_str));
    }

    /* Update daily structure under lock */
    rain_per_day_data current_day_to_save;
    rain_per_day_data yesterday_to_save;
    memset(&current_day_to_save, 0, sizeof(current_day_to_save));
    memset(&yesterday_to_save, 0, sizeof(yesterday_to_save));

    taskENTER_CRITICAL(&g_rain_day_mux);

    /* Fill missed hourly bins with zeros (same day or tail of previous day). */
    fill_missed_bins_zeros(curr_hour_idx, s_last_hour_idx, step, rolled_day);

    /* Write this hour's snapshot and recompute total. */
    write_bin_and_recompute_total(bin_to_write, snapshot);

    /* Handle day rollover: copy yesterday, clear current and set new date. */
    if (must_archive_yesterday)
    {
        prepare_day_rollover_locked(new_date_str, &yesterday_to_save);
    }

    /* Keep a copy of the current day for persistence. */
    current_day_to_save = g_rain_day;

    /* Advance global hour tracking. */
    s_last_hour_idx = curr_hour_idx;
    g_hour_idx = curr_hour_idx;

    taskEXIT_CRITICAL(&g_rain_day_mux);

    if (step != 1U)
    {
        ESP_LOGW(PLUVIOMETER_TAG,
                 "Hour jump detected: last=%u -> current=%u (step=%u). Missed bins handled.",
                 (unsigned)bin_to_write, (unsigned)curr_hour_idx, (unsigned)step);
    }

    /* Persist and publish according to the actions taken. */
    persist_and_publish(must_archive_yesterday,
                        must_archive_yesterday ? &yesterday_to_save : NULL,
                        &current_day_to_save,
                        bin_to_write,
                        snapshot);

    ESP_LOGI(PLUVIOMETER_TAG,
             "Reset hourly accumulator; new active hour=%u (sod=%u).",
             (unsigned)curr_hour_idx, (unsigned)sod);
}

/**
 * @brief FreeRTOS task: accumulates rainfall from pulses and writes hourly bins anchored to RTC.
 *
 * - Pulses → mm are accumulated in a temporary “current-hour” bucket.
 * - On RTC hour change (00..23), we atomically snapshot+reset the accumulator
 * and store the value into g_rain_day.rain_per_hour[hour_just_finished].
 * - We always persist to NVS (including 0.0 mm) and recompute the daily total.
 * - If hours were skipped within the same day, we fill missed bins with 0.0 mm.
 * - On day rollover (23→0 or any midnight crossing), we finalize the previous day
 * and clear the structure for the new day (date string updated from RTC).
 */
void system_monitoring_rainfall_task(void *arg)
{
    TickType_t last_wake_time = xTaskGetTickCount();
    static TickType_t s_last_rtc_warn_tick = 0;

    ESP_LOGI(PLUVIOMETER_TAG, "Starting rainfall monitoring task (hourly RTC-anchored windows).");

    system_monitoring_init_rainfall_data();

    while (1)
    {
        update_rain_per_pulse_if_needed();

        gpio_rain_sensor_calculate_rainfall();
        s_rain_total_mm = get_rain_total();

        maybe_trigger_rain_shutdown();

        uint8_t curr_hour_idx = 0;
        uint32_t sod = 0;
        time_t now_ts = 0;

        if (rtc_get_current_hour(&curr_hour_idx, &sod, &now_ts))
        {
            if (s_last_hour_idx == 0xFFU)
            {
                init_hour_index_from_rtc(curr_hour_idx, sod);
            }
            else
            {
                if (curr_hour_idx != s_last_hour_idx)
                {
                    handle_hour_change(curr_hour_idx, now_ts, sod);
                }
            }
        }
        else
        {
            TickType_t now_tick = xTaskGetTickCount();
            TickType_t elapsed = now_tick - s_last_rtc_warn_tick;
            if (elapsed > pdMS_TO_TICKS(60000))
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

    taskENTER_CRITICAL(&rain_total_mux);
    set_rain_total(get_rain_total() + interval_rain);
    taskEXIT_CRITICAL(&rain_total_mux);
}