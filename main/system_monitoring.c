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
static pivot_return_config system_monitoring_config = {}; /**< Configuration for system monitoring. */
static barrier_status status_barrier = PIVOT_OUTSIDE_THE_BARRIER; /**< Current status of the barrier. */


static uint16_t* system_monitoring_current_angle  = &global_angle; /**< Pointer to the current angle variable. */

/* Private methods ----------------------------------- */

/**
 * @brief Executes the actuation process based on the system configuration.
 *
 * This function performs the actuation process based on the current system configuration.
 */
static void system_monitoring_actuation(void);

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

static void system_monitoring_automatic_return()
{
    if(system_monitoring_config.automatic_return == true)
    {
        vTaskDelay(pdMS_TO_TICKS(5000)); // 5 seconds

        data_app_load(DATA_TYPE_BARRIER, &return_back_flag);

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

            if(system_monitoring_config.water_return == true)
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
            data_app_save(DATA_TYPE_BARRIER, &return_back_flag, sizeof(return_back_flag));
            system_states = SYSTEM_RETURN;
        }
        else
        {
        	return_back_flag = false;
            data_app_save(DATA_TYPE_BARRIER, &return_back_flag, sizeof(return_back_flag));
            system_states = SYSTEM_PAUSE;
        }
    }
    else
    {
        system_states = SYSTEM_PAUSE;
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

    pivot_actions pivot_actions = {};

    act on the equipment (pivo_off) - send IDP 01
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

    system_monitoring_automatic_return();
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
            if((system_monitoring_config.start_angle_virtual_barrier > system_monitoring_config.end_angle_virtual_barrier))
            {
                if(*system_monitoring_current_angle  > system_monitoring_config.start_angle_virtual_barrier
                || *system_monitoring_current_angle < system_monitoring_config.end_angle_virtual_barrier)
                {
                    if(system_states != SYSTEM_PAUSE && status_barrier != PIVOT_LEAVING_THE_BARRIER)
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
                if(*system_monitoring_current_angle > system_monitoring_config.start_angle_virtual_barrier
                && *system_monitoring_current_angle < system_monitoring_config.end_angle_virtual_barrier)
                {
                    if(system_states != SYSTEM_PAUSE && status_barrier != PIVOT_LEAVING_THE_BARRIER)
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
            system_monitoring_barrier(pivot_actions);
        }        

        vTaskDelay(pdMS_TO_TICKS(SYSTEM_DELAY_ANALYSIS_ANGLE_MS));
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
void system_monitoring_barrier(const pivot_actions current_pivot_actions)
{
    if(*system_monitoring_current_angle != 655)
    {
        if((system_monitoring_config.start_angle_physical_barrier < system_monitoring_config.end_angle_physical_barrier
        || system_monitoring_config.start_angle_physical_barrier > system_monitoring_config.end_angle_physical_barrier))
        {
            if(*system_monitoring_current_angle >= system_monitoring_config.start_angle_physical_barrier - 5
            && *system_monitoring_current_angle <= system_monitoring_config.start_angle_physical_barrier + 5)
            {
                if(current_pivot_actions.rotation == PIVOT_CW)
                {
                    status_barrier = PIVOT_LEAVING_THE_BARRIER;

                }
                else if(current_pivot_actions.rotation == PIVOT_CCW) /* If rotation was sent COUNTERCLOCKWISE - REVERSE */
                {
                    status_barrier = PIVOT_IN_THE_BARRIER;
                }

                system_monitoring_automatic_return();          
            }
            else if (*system_monitoring_current_angle >= system_monitoring_config.end_angle_physical_barrier - 5
            && *system_monitoring_current_angle <= system_monitoring_config.end_angle_physical_barrier + 5)
            {
                if(current_pivot_actions.rotation == PIVOT_CW)
                {
                    status_barrier = PIVOT_IN_THE_BARRIER;
                }
                else if(current_pivot_actions.rotation == PIVOT_CCW)
                {
                    status_barrier = PIVOT_LEAVING_THE_BARRIER;
                }

                system_monitoring_automatic_return();
            }
            else
            {
                status_barrier = PIVOT_OUTSIDE_THE_BARRIER;
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
}

/**
 * @brief Starts the system monitoring task with the provided configuration.
 *
 * This function initializes the system monitoring task with the specified configuration.
 *
 * @param return_config Configuration for system monitoring.
 * @param monitoring_time Time interval for system monitoring (in minutes).
 */
void system_monitoring_start(const pivot_return_config return_config, uint8_t monitoring_time)
{
    system_monitoring_stop();

    if(monitoring_time > 0)
    {
        system_monitoring_delay = monitoring_time;

        system_monitoring_timer_handle = xTimerCreate(
                              "system_timer", /* name */
                              pdMS_TO_TICKS(system_monitoring_delay * 60000), /* period/time */
                              pdTRUE, /* auto reload */
                              (void*)0, /* timer ID */
                              system_monitoring_timer); /* callback */

        xTimerStart(system_monitoring_timer_handle, 1000);
    }

    if(return_config.start_angle == 0 && return_config.end_angle == 0)
    {
        ESP_LOGI(SYSTEM_MONITORING_TAG, "Pivot configured from 0° to 360°, without barrier");
    }
    else
    {
        memcpy(&system_monitoring_config, &return_config, sizeof(system_monitoring_config));
        xTaskCreate(&system_monitoring_task,
                    SYSTEM_MONITORING_TASK_NAME,
                    SYSTEM_MONITORING_TASK_SIZE,
                    NULL,
                    SYSTEM_MONITORING_TASK_PRIORITY,
                    &xTask_system_monitoring);
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
