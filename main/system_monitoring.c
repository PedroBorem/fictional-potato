/**
 * @file system_monitoring.c
 * @brief System monitoring functionality implementation.
 * @author soil-dev
 * @date 4 de ago. de 2023
 */

#include "system_monitoring.h"

#include "FreeRTOS_defines.h"
#include "log.h"

#include "actuation_app.h"
#include "data_app.h"
#include "comm_app.h"
#include "rtc_app.h"
#include "idp_parser.h"
#include "gpio_actuator.h"
#include "project_config.h"

#include <string.h>

/* Private definitions ------------------------------------------- */

#define SYSTEM_MONITORING_TAG	"system_monitoring"

#define SYSTEM_DELAY_ANALYSIS_ANGLE_MS	(6000) // 1 minute

/**
 * @brief Enumeration representing the possible states of the system monitoring.
 */
typedef enum {
    SYSTEM_PAUSE = 0,    /**< System is in a paused state. */
    SYSTEM_RUNNING,      /**< System is running normally. */
    SYSTEM_RETURN,       /**< System is in a return state. */
} system_monitoring_states;

/* Private variables  -------------------------------------------- */
static system_monitoring_states system_states = SYSTEM_PAUSE; /**< Current state of the system monitoring. */
bool return_back_flag = false;

static TaskHandle_t xTask_system_monitoring = NULL; /**< Task handle for the system monitoring task. */
static TimerHandle_t system_monitoring_timer_handle = NULL; /**< Timer handle for periodic actions. */
static app_callback system_monitoring_callback = NULL; /**< Callback function for system monitoring events. */

static uint8_t system_monitoring_delay = 10; /**< Time interval for system monitoring (in minutes). */
static pivot_virtual_config system_monitoring_virtual_config = {}; /**< Configuration for system monitoring. */
static pivot_physical_config system_monitoring_physical_config = {}; /**< Configuration for system monitoring. */
static barrier_status status_barrier = PIVOT_OUTSIDE_THE_BARRIER; /**< Current status of the barrier. */

static uint32_t panel_reading;

static uint16_t* system_monitoring_current_angle = &global_angle; /**< Pointer to the current angle variable. */

char pluviometro[MAX_RAINFALL_ENTRIES][30] = {};
float rain_per_pulse = 0.1; // Rainfall per pulse (mm)
bool rain_per_pulse_flag = false;

static uint8_t current_index = 0; // Índice para o próximo elemento no buffer


/* Private methods ----------------------------------- */

/**
 * @brief Task to calculate rainfall every second and save accumulated data every 10 minutes.
 *
 * @param arg Task argument (default NULL).
 */
void system_monitoring_rainfall_task(void *arg);

/**
 * @brief Executes the actuation process based on the system configuration.
 *
 * This function performs the actuation process based on the current system configuration.
 */
static void system_monitoring_actuation_virtual_barrier(void);

/**
 * @brief Task function for system monitoring.
 *
 * This task monitors the system state and triggers actuation as needed.
 *
 * @param arg Task argument (unused).
 */
static void system_monitoring_task(void* arg);

/**
 * @brief Timer callback for periodic system actions.
 *
 * This function is called periodically to execute specific actions.
 *
 * @param pxTimer Timer handle (unused).
 */
static void system_monitoring_timer(TimerHandle_t pxTimer);

/**
 * @brief Executes the automatic return process based on the pivot actions and system configuration.
 *
 * This function handles automatic pivot return based on configured settings.
 * Loads the barrier data,
 * and determines whether the pivot should be driven to its return state or paused the system.
 *
 * @param pivot_actions Structure containing the pivot actions to be performed.
 */
static void system_monitoring_automatic_return(pivot_actions pivot_actions, type_barrier barrier_type)
{
    uint8_t idp = IDP_INVALID;
    uint16_t dwp = 0;
    char str_out[50] = {};

    barrier_status status_barrier = PIVOT_IN_THE_BARRIER;

    if(barrier_type == VIRTUAL_BARRIER)
    {
        if(system_monitoring_virtual_config.automatic_return == true)
        {
            vTaskDelay(pdMS_TO_TICKS(5000)); // 5 seconds

            data_app_load(DATA_TYPE_BARRIER_STATUS, &return_back_flag);

            if(return_back_flag == false)
            {
                // act on the equipment - send IDP 01
                pivot_actions.power_state = PIVOT_ON;

                if(pivot_actions.rotation == PIVOT_CW)
                {
                    pivot_actions.rotation = PIVOT_CCW;
                }
                else
                {
                    pivot_actions.rotation = PIVOT_CW;
                }

                if(system_monitoring_virtual_config.water_return == true)
                {
                    pivot_actions.watering_state = PIVOT_WET;
                }
                else
                {
                    pivot_actions.watering_state = PIVOT_DRY;
                }

                idp = IDP_1;
                dwp = idp_parser_create_pwd(pivot_actions);
                uint16_t percent_return = pivot_actions.percentimeter;

                arg_pair_t arg_idp_02[] =
                {
                    { "uint8_t", &idp },
                    { "string", SYSTEM_MONITORING_TAG },
                    { "uint16_t", &dwp },
                    { "uint16_t", &percent_return },
                    { NULL, NULL }
                };

                memset(str_out, 0x00, sizeof(str_out));
                idp_parser_create_package(str_out, arg_idp_02);
                system_monitoring_callback(str_out, COMM_MQTT);

                return_back_flag = true;
                data_app_save(DATA_TYPE_BARRIER_STATUS, &return_back_flag, sizeof(return_back_flag));
                system_states = SYSTEM_RETURN;
            }
            else
            {
                return_back_flag = false;
                data_app_save(DATA_TYPE_BARRIER_STATUS, &return_back_flag, sizeof(return_back_flag));
                system_states = SYSTEM_PAUSE;
            }
        }
        else
        {
            system_states = SYSTEM_PAUSE;
        } 
    }
    else if(barrier_type == PHYSICAL_BARRIER)
    {
        data_app_load(DATA_TYPE_MANUAL_COUNTER, &panel_reading);
        if(system_monitoring_physical_config.automatic_return == true  && panel_reading < READING_LIMIT_FOR_RETURN)
        {
            vTaskDelay(pdMS_TO_TICKS(15000)); // 15   seconds

            data_app_load(DATA_TYPE_BARRIER_STATUS, &return_back_flag);

            if(return_back_flag == false)
            {
                // act on the equipment - send IDP 01
                pivot_actions.power_state = PIVOT_ON;

                status_barrier = PIVOT_LEAVING_THE_BARRIER;

                if(pivot_actions.rotation == PIVOT_CW)
                {
                    pivot_actions.rotation = PIVOT_CCW;
                }
                else
                {
                    pivot_actions.rotation = PIVOT_CW;
                }

                if(system_monitoring_physical_config.water_return == true)
                {
                    pivot_actions.watering_state = PIVOT_WET;
                }
                else
                {
                    pivot_actions.watering_state = PIVOT_DRY;
                }

                gpio_actuator_set_time_start(status_barrier);

                idp = IDP_1;
                dwp = idp_parser_create_pwd(pivot_actions);
                uint16_t percent_return = pivot_actions.percentimeter;

                arg_pair_t arg_idp_02[] =
                {
                    { "uint8_t", &idp },
                    { "string", SYSTEM_MONITORING_TAG },
                    { "uint16_t", &dwp },
                    { "uint16_t", &percent_return },
                    { NULL, NULL }
                };

                memset(str_out, 0x00, sizeof(str_out));
                idp_parser_create_package(str_out, arg_idp_02);
                system_monitoring_callback(str_out, COMM_MQTT);

                return_back_flag = true;
                data_app_save(DATA_TYPE_BARRIER_STATUS, &return_back_flag, sizeof(return_back_flag));
                system_states = SYSTEM_RETURN;
            }
            else
            {
                return_back_flag = false;
                data_app_save(DATA_TYPE_BARRIER_STATUS, &return_back_flag, sizeof(return_back_flag));
                system_states = SYSTEM_PAUSE;
            }
        }
        else
        {
            system_states = SYSTEM_PAUSE;
        } 
    }
}

/**
 * @brief Executes the actuation process based on the system configuration.
 *
 * This function performs the actuation process based on the current system configuration.
 */
static void system_monitoring_actuation_virtual_barrier(void)
{
    uint8_t idp = IDP_INVALID;
    uint16_t dwp = 0;
    char str_out[50] = {};
    type_barrier barrier_type = VIRTUAL_BARRIER;  

    pivot_actions pivot_actions = {};

    // act on the equipment (pivo_off) - send IDP 01
    actuation_app_get_actions(&pivot_actions, sizeof(pivot_actions));
    pivot_actions.power_state = PIVOT_OFF;

    idp = IDP_1;
    dwp = idp_parser_create_pwd(pivot_actions);
    uint16_t percent_off = 0;

    arg_pair_t arg_idp_01[] =
    {
        { "uint8_t", &idp },
        { "string", SYSTEM_MONITORING_TAG },
        { "uint16_t", &dwp },
        { "uint16_t", &percent_off },
        { NULL, NULL }
    };

    memset(str_out, 0x00, sizeof(str_out));
    idp_parser_create_package(str_out, arg_idp_01);
    system_monitoring_callback(str_out, COMM_MQTT);

    system_monitoring_automatic_return(pivot_actions, barrier_type);
}

/**
 * @brief Task function for system monitoring.
 *
 * This task monitors the system state and triggers actuation as needed.
 *
 * @param arg Task argument (unused).
 */
static void system_monitoring_task(void* arg)
{
    while(1)
    {
        if(*system_monitoring_current_angle != 655)
        {
            if((system_monitoring_virtual_config.start_angle_virtual_barrier > system_monitoring_virtual_config.end_angle_virtual_barrier))
            {
                if(*system_monitoring_current_angle  > system_monitoring_virtual_config.start_angle_virtual_barrier
                || *system_monitoring_current_angle < system_monitoring_virtual_config.end_angle_virtual_barrier)
                {
                    if(system_states != SYSTEM_PAUSE && status_barrier != PIVOT_LEAVING_THE_BARRIER  && status_barrier != PIVOT_LEAVING_THE_VIRTUAL_BARRIER)
                    {
                        system_monitoring_actuation_virtual_barrier();
                    }
                }
                else
                {
                    system_states = SYSTEM_RUNNING;
                    status_barrier = PIVOT_OUTSIDE_THE_BARRIER;
                }
            }
            else
            {
                if(*system_monitoring_current_angle > system_monitoring_virtual_config.start_angle_virtual_barrier
                && *system_monitoring_current_angle < system_monitoring_virtual_config.end_angle_virtual_barrier)
                {
                    if(system_states != SYSTEM_PAUSE && status_barrier != PIVOT_LEAVING_THE_BARRIER && status_barrier != PIVOT_LEAVING_THE_VIRTUAL_BARRIER)
                    {
                        system_monitoring_actuation_virtual_barrier();
                    }
                }
                else
                {
                    system_states = SYSTEM_RUNNING;
                    status_barrier = PIVOT_OUTSIDE_THE_BARRIER;
                }

            }            
        }

        if(system_states == SYSTEM_RETURN)
        {
            pivot_actions pivot_actions = {};
            actuation_app_get_actions(&pivot_actions, sizeof(pivot_actions));
            system_monitoring_barrier(pivot_actions, VIRTUAL_BARRIER);
        }        

        vTaskDelay(pdMS_TO_TICKS(SYSTEM_DELAY_ANALYSIS_ANGLE_MS));
    }
}

/**
 * @brief Initializes the rain gauge vector with NVS data.
 */
void system_monitoring_init_rainfall_data(void) 
{
    esp_err_t err = data_app_load(DATA_TYPE_RAINFALL_ACCUMULATED, pluviometro);
    if (err == ESP_OK) 
    {
        current_index = 0; 
    } 
    else 
    {
        ESP_LOGW(SYSTEM_MONITORING_TAG, "Failed to load rainfall data. Initializing to empty.");
        memset(pluviometro, 0, sizeof(pluviometro));
        current_index = 0;
    }

    err = data_app_load(DATA_TYPE_RAIN_PER_PULSE, &rain_per_pulse);
    if (err != ESP_OK || rain_per_pulse <= 0.0 || rain_per_pulse > 10.0) 
    {
        rain_per_pulse = 0.1;
        ESP_LOGW(SYSTEM_MONITORING_TAG, "Failed to load RAIN_PER_PULSE. Using default: %.2f", rain_per_pulse);
    }
    else
    {
        ESP_LOGI(SYSTEM_MONITORING_TAG, "Loaded RAIN_PER_PULSE from NVS: %.2f", rain_per_pulse);
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
    const TickType_t save_interval = pdMS_TO_TICKS(3600000);

    char str_date_time[20] = {};    

    system_monitoring_init_rainfall_data();

    while (1) 
    {
        if (rain_per_pulse_flag)
        {
            float nvs_rain_per_pulse = 0.1;
            esp_err_t ret = data_app_load(DATA_TYPE_RAIN_PER_PULSE, &nvs_rain_per_pulse);
            if (ret == ESP_OK && nvs_rain_per_pulse > 0.0 && nvs_rain_per_pulse <= 10.0)
            {
                rain_per_pulse = nvs_rain_per_pulse;
            }
            else
            {
                ESP_LOGW(SYSTEM_MONITORING_TAG, "Failed to load RAIN_PER_PULSE. Using default: %.2f", rain_per_pulse);
            }
            rain_per_pulse_flag = false;
        }

        gpio_rain_sensor_calculate_rainfall(); 

        if ((xTaskGetTickCount() - last_save_time) >= save_interval) 
        {
            if (rain_total > 0.0f)
            {
                time_t timestamp = rtc_app_get_timestamp(false);
                rtc_app_get_str_date_time(timestamp, str_date_time);

                snprintf(pluviometro[current_index], sizeof(pluviometro[current_index]), "%.2f-%s", rain_total, str_date_time);

                ESP_LOGI(SYSTEM_MONITORING_TAG, "Saved rainfall data: %s", pluviometro[current_index]);

                current_index = (current_index + 1) % 10;

                if (data_app_save(DATA_TYPE_RAINFALL_ACCUMULATED, pluviometro, sizeof(pluviometro)) != ESP_OK) 
                {
                    ESP_LOGE(SYSTEM_MONITORING_TAG, "Failed to save rainfall data.");
                }
                else 
                {
                    ESP_LOGI(SYSTEM_MONITORING_TAG, "Rainfall data saved successfully.");
                }

                rain_total = 0.0f;
            }

            last_save_time = xTaskGetTickCount();
        }

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(500));
    }
}


/**
 * @brief Determines and triggers actuation based on the barrier status.
 *
 * This function evaluates the current angle of the pivot and determines the barrier status
 * based on the angle configuration. It then triggers actuation accordingly to handle the
 * pivot's movement in relation to the barrier.
 *
 * @param[in] current_pivot_actions The current actions and configuration of the pivot.
 */
void system_monitoring_barrier(const pivot_actions current_pivot_actions, type_barrier barrier_type)
{
    pivot_actions pivot_actions = {};

    if(barrier_type == PHYSICAL_BARRIER)
    {
        if(*system_monitoring_current_angle != 655)
        {
            if((system_monitoring_physical_config.start_angle_physical_barrier < system_monitoring_physical_config.end_angle_physical_barrier
            || system_monitoring_physical_config.start_angle_physical_barrier > system_monitoring_physical_config.end_angle_physical_barrier))
            {
                if(*system_monitoring_current_angle >= system_monitoring_physical_config.start_angle_physical_barrier - 5
                && *system_monitoring_current_angle <= system_monitoring_physical_config.start_angle_physical_barrier + 5)
                {
                    if(current_pivot_actions.rotation == PIVOT_CW)
                    {
                        status_barrier = PIVOT_LEAVING_THE_BARRIER;

                    }
                    else if(current_pivot_actions.rotation == PIVOT_CCW) /* If rotation was sent COUNTERCLOCKWISE - REVERSE */
                    {
                        status_barrier = PIVOT_IN_THE_BARRIER;
                    }

                    if(current_pivot_actions.power_state == PIVOT_OFF)
                    {
                        actuation_app_get_actions(&pivot_actions, sizeof(pivot_actions));
                        pivot_actions.rotation = PIVOT_CCW;
                        pivot_actions.watering_state = current_pivot_actions.watering_state;
                        pivot_actions.power_state = PIVOT_ON;

                        pivot_actions.percentimeter = current_pivot_actions.percentimeter;

                        system_monitoring_automatic_return(pivot_actions, barrier_type);
                    }
                }
                else if (*system_monitoring_current_angle >= system_monitoring_physical_config.end_angle_physical_barrier - 5
                && *system_monitoring_current_angle <= system_monitoring_physical_config.end_angle_physical_barrier + 5)
                {
                    if(current_pivot_actions.rotation == PIVOT_CW)
                    {
                        status_barrier = PIVOT_IN_THE_BARRIER;

                    }
                    else if(current_pivot_actions.rotation == PIVOT_CCW) /* If rotation was sent COUNTERCLOCKWISE - REVERSE */
                    {
                        status_barrier = PIVOT_LEAVING_THE_BARRIER;
                    }

                    if(current_pivot_actions.power_state == PIVOT_OFF)
                    {
                        actuation_app_get_actions(&pivot_actions, sizeof(pivot_actions));
                        pivot_actions.rotation = PIVOT_CW;
                        pivot_actions.watering_state = current_pivot_actions.watering_state;
                        pivot_actions.power_state = PIVOT_ON;

                        pivot_actions.percentimeter = current_pivot_actions.percentimeter;

                        system_monitoring_automatic_return(pivot_actions, barrier_type);
                    }
                }
                else
                {
                    status_barrier = PIVOT_OUTSIDE_THE_BARRIER;
                }
            }
        }       
    }
    else if(barrier_type == VIRTUAL_BARRIER)
    {
        if((system_monitoring_virtual_config.start_angle_virtual_barrier < system_monitoring_virtual_config.end_angle_virtual_barrier
        || system_monitoring_virtual_config.start_angle_virtual_barrier > system_monitoring_virtual_config.end_angle_virtual_barrier))
        {
            if(*system_monitoring_current_angle >= system_monitoring_virtual_config.start_angle_virtual_barrier - 5
            && *system_monitoring_current_angle <= system_monitoring_virtual_config.start_angle_virtual_barrier + 5)
            {
                if(current_pivot_actions.rotation == PIVOT_CW)
                {
                    status_barrier = PIVOT_LEAVING_THE_VIRTUAL_BARRIER;

                }
                else if(current_pivot_actions.rotation == PIVOT_CCW) /* If rotation was sent COUNTERCLOCKWISE - REVERSE */
                {
                    status_barrier = PIVOT_IN_THE_VIRTUAL_BARRIER;
                }
            }
            else if (*system_monitoring_current_angle >= system_monitoring_virtual_config.end_angle_virtual_barrier - 5
            && *system_monitoring_current_angle <= system_monitoring_virtual_config.end_angle_virtual_barrier + 5)
            {
                if(current_pivot_actions.rotation == PIVOT_CW)
                {
                    status_barrier = PIVOT_IN_THE_VIRTUAL_BARRIER;

                }
                else if(current_pivot_actions.rotation == PIVOT_CCW) /* If rotation was sent COUNTERCLOCKWISE - REVERSE */
                {
                    status_barrier = PIVOT_LEAVING_THE_VIRTUAL_BARRIER;
                }
            }
            else
            {
                status_barrier = PIVOT_OUTSIDE_THE_VIRTUAL_BARRIER;
            }
        }
    }

    gpio_actuator_set_time_start(status_barrier);
}

/**
 * @brief Timer callback for periodic system actions.
 *
 * This function is called periodically to execute specific actions.
 *
 * @param pxTimer Timer handle (unused).
 */
static void system_monitoring_timer(TimerHandle_t pxTimer)
{
    // send IDP 0 (current status)
    system_monitoring_callback("#00$", COMM_MQTT);

    // save current Timestamp
    time_t timestamp_now = rtc_app_get_timestamp(false);
    data_app_save(DATA_TYPE_TIMESTAMP, &timestamp_now, sizeof(timestamp_now));

    // send IDP 19 (current pressure status)
    vTaskDelay(pdMS_TO_TICKS(5000));
    system_monitoring_callback("#19$", COMM_MQTT);
}

/**
 * @brief Starts the system monitoring task with the provided configuration.
 *
 * This function initializes the system monitoring task with the specified configuration.
 *
 * @param virtual_config Configuration for system monitoring.
 * @param monitoring_time Time interval for system monitoring (in minutes).
 */
void system_monitoring_start(const pivot_physical_config physical_config, const pivot_virtual_config virtual_config, uint8_t monitoring_time)
{
    system_monitoring_stop();

    if (monitoring_time > 0)
    {
        system_monitoring_delay = monitoring_time;

        system_monitoring_timer_handle = xTimerCreate(
            "system_timer", /* name */
            pdMS_TO_TICKS(system_monitoring_delay * 60000), /* period/time */
            pdTRUE, /* auto reload */
            (void *)0, /* timer ID */
            system_monitoring_timer); /* callback */

        xTimerStart(system_monitoring_timer_handle, 1000);
    }

    if ((physical_config.start_angle_physical_barrier == 0 && physical_config.end_angle_physical_barrier == 0) &&
        (virtual_config.start_angle_virtual_barrier == 0 && virtual_config.end_angle_virtual_barrier == 0))
    {
        ESP_LOGI(SYSTEM_MONITORING_TAG, "Pivot configured from 0° to 360°, without barrier");
    }
    else
    {

        memcpy(&system_monitoring_virtual_config, &virtual_config, sizeof(system_monitoring_virtual_config));
        memcpy(&system_monitoring_physical_config, &physical_config, sizeof(system_monitoring_physical_config));

        xTaskCreate(&system_monitoring_task,
                    SYSTEM_MONITORING_TASK_NAME,
                    SYSTEM_MONITORING_TASK_SIZE,
                    NULL,
                    SYSTEM_MONITORING_TASK_PRIORITY,
                    &xTask_system_monitoring);
    }

    BaseType_t rainfall_task_created = xTaskCreate(system_monitoring_rainfall_task, 
                                       PLUV_TASK_NAME, 
                                       PLUV_STACK_SIZE, 
                                       NULL, 
                                       PLUV_TASK_PRIORITY, 
                                       NULL);
    if (rainfall_task_created != pdPASS)
    {
        ESP_LOGE(SYSTEM_MONITORING_TAG, "%s: Failed to create rainfall task", __func__);
    }
    else
    {
        ESP_LOGI(SYSTEM_MONITORING_TAG, "%s: Rainfall monitoring initialized successfully", __func__);
    }
}

/**
 * @brief Stops the system monitoring task.
 *
 * This function stops the system monitoring task.
 */
void system_monitoring_stop(void)
{
	if(xTask_system_monitoring != NULL)
	{
		vTaskDelete(xTask_system_monitoring);
		xTask_system_monitoring = NULL;
	}

	if(system_monitoring_timer_handle != NULL)
	{
		xTimerStop(system_monitoring_timer_handle,portMAX_DELAY);
		xTimerDelete(system_monitoring_timer_handle,portMAX_DELAY);
		system_monitoring_timer_handle = NULL;
	}
}

/**
 * @brief Registers a callback function for system monitoring.
 *
 * This function allows the registration of a callback function to be used in the system monitoring module.
 * The registered callback will be invoked by the system monitoring module to perform specific actions.
 *
 * @param[in] callback A function pointer to the callback function.
 */
void system_monitoring_register_callback(const app_callback callback)
{
	system_monitoring_callback = callback;
}
