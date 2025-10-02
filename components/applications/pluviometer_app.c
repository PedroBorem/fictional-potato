/**
 * @file pluviometer_app.c
 * @brief Pluviometer application implementation for managing rainfall data.
 */

 /* Self include */
#include "pluviometer_app.h"
#include "data_app.h"

#define PLUVIOMETER_TAG "pluviometer_app"

static rain_data pluviometer[MAX_RAINFALL_ENTRIES] = {0};
float rain_per_pulse = 0.1f; // Rainfall per pulse (mm)
bool rain_per_pulse_flag = false;

/**
 * @brief Sets the rain-per-pulse flag status.
 *
 * @param flag true to enable rain-per-pulse mode, false to disable.
 */
void set_rain_per_pulse_flag(bool flag)
{
    rain_per_pulse_flag = flag;
}

/**
 * @brief Returns a pointer to the rain data array.
 *
 * Allows read-only or mutable access to the internal rainfall data.
 *
 * @return Pointer to the pluviometer array.
 */
rain_data *get_rain_data_array()
{
    return pluviometer;
}

/**
 * @brief Sets a rain_data entry at the specified index.
 *
 * @param index Position in the pluviometer array.
 * @param data The rain_data value to set.
 */
void set_rain_data_entry(int index, rain_data data)
{
    if (index >= 0 && index < MAX_RAINFALL_ENTRIES)
    {
        pluviometer[index] = data;
    }
}

/**
 * @brief Initializes the rain gauge vector with NVS data.
 */
void system_monitoring_init_rainfall_data(void)
{
    esp_err_t err = data_app_load(DATA_TYPE_RAINFALL_ACCUMULATED, pluviometer);
    if (err != ESP_OK)
    {
        ESP_LOGE(SYSTEM_MONITORING_TAG, "Failed to load rainfall data. Initializing to empty.");
        memset(pluviometer, 0, sizeof(pluviometer));
    }

    current_index = 0;

    err = data_app_load(DATA_TYPE_RAIN_PER_PULSE, &rain_per_pulse);
    if (err != ESP_OK || rain_per_pulse <= 0.0 || rain_per_pulse > 10.0)
    {
        rain_per_pulse = 0.1f;
        ESP_LOGW(SYSTEM_MONITORING_TAG, "Failed to load RAIN_PER_PULSE. Using default: %.2f", rain_per_pulse);
    }
}

/**
 * @brief Task to calculate rainfall every second and save accumulated data every hour.
 *
 * This task calculates the rainfall based on sensor pulses and logs the rainfall interval every second.
 * It saves the accumulated rainfall data in persistent memory every hour.
 *
 * @param arg Task argument (default NULL).
 */
void system_monitoring_rainfall_task(void *arg) 
{
    TickType_t last_wake_time = xTaskGetTickCount();
    TickType_t last_save_time = last_wake_time;
    const TickType_t save_interval = pdMS_TO_TICKS(RAINFALL_SAVE_INTERVAL_MS);
    pivot_actions actions = {};
    float rain_shutdown_value;
    /*
    * - 1 minuto   = 60000 ms
    * - 2 minutos  = 120000 ms
    * - 3 minutos  = 180000 ms
    * - 5 minutos  = 300000 ms
    * - 10 minutos = 600000 ms
    * - 30 minutos = 1800000 ms
    * - 60 minutos = 3600000 ms
    */
    system_monitoring_init_rainfall_data();

    while (1) 
    {
        if (rain_per_pulse_flag)
        {
            float nvs_rain_per_pulse = 0.1;
            esp_err_t ret = data_app_load(DATA_TYPE_RAIN_PER_PULSE, &nvs_rain_per_pulse);
            if (ret == ESP_OK && nvs_rain_per_pulse > 0.0f && nvs_rain_per_pulse <= 10.0f)
            {
                rain_per_pulse = nvs_rain_per_pulse;
            }
            else
            {
                ESP_LOGW(SYSTEM_MONITORING_TAG,
                         "Failed to load RAIN_PER_PULSE. Using default: %.2f", 
                         rain_per_pulse);
            }
            rain_per_pulse_flag = false;
        }

        gpio_rain_sensor_calculate_rainfall(); 
        float rain_total = get_rain_total();
        if (rain_total > 0.0f) 
        {
            data_app_load(DATA_TYPE_RAIN_SHUTDOWN_VALUE, &rain_shutdown_value);
            data_app_load(DATA_TYPE_ACTIONS, &actions);

            if(actions.power_state == PIVOT_ON && actions.watering_state == PIVOT_WET && rain_shutdown_value != 0.0f)
            {
                if(rain_total >= rain_shutdown_value)
                {
                    gpio_actuator_shutdown();
                }    
            }
        }

        if ((xTaskGetTickCount() - last_save_time) >= save_interval) 
        {
            if (rain_total > 0.0f)
            {
                time_t timestamp = rtc_app_get_timestamp(false);

                char tmp_date_str[30];
                rtc_app_get_str_date_time(timestamp, tmp_date_str);

                int oldest_index = rtc_app_find_oldest_timestamp(
                                       pluviometer, 
                                       MAX_RAINFALL_ENTRIES
                                   );

                pluviometer[oldest_index].rain_per_hour = rain_total;
                strncpy(pluviometer[oldest_index].str_date_time, tmp_date_str, sizeof(pluviometer[oldest_index].str_date_time) - 1);

                ESP_LOGI(SYSTEM_MONITORING_TAG, 
                         "Saved rainfall data (%.2f) at index %d - %s", 
                         pluviometer[oldest_index].rain_per_hour, 
                         oldest_index,
                         pluviometer[oldest_index].str_date_time);

                if (data_app_save(DATA_TYPE_RAINFALL_ACCUMULATED, 
                                  pluviometer, 
                                  sizeof(pluviometer)) != ESP_OK) 
                {
                    ESP_LOGE(SYSTEM_MONITORING_TAG, "Failed to save rainfall data.");
                }
                else 
                {
                    ESP_LOGI(SYSTEM_MONITORING_TAG, "Rainfall data saved successfully.");
                }

                set_rain_total(0.0f);
            }

            last_save_time = xTaskGetTickCount();
        }

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(500));
    }
}
