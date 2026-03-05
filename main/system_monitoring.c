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

/* Private methods ----------------------------------- */

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


void system_monitoring_start(const pivot_physical_config physical_config, const pivot_virtual_config virtual_config, uint8_t monitoring_time);

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
                system_monitoring_callback(str_out, comm_main_mode);

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
                system_monitoring_callback(str_out, comm_main_mode);

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
    system_monitoring_callback(str_out, comm_main_mode);
    system_monitoring_pivot_shutdown(TYPE_HANGS_UP_VIRTUAL_BARRIER, idp, "0", SYSTEM_MONITORING_TAG);

    system_monitoring_automatic_return(pivot_actions, barrier_type);
}

/**
 * @brief Task function for system monitoring.
 *
 * This task monitors the system state and triggers actuation as needed.
 *
/**
 * @brief TODO: Describe `argument`.
 *
 * @param arg TODO.
 * @return TODO.
 */
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
 * @brief Checks if the current angle is within the specified range barrier.
 *
 * This function evaluates the pivot's current angle and determines if it falls
 * within the defined range around the physical barrier's start and end angles.
 * It returns true if the angle is within the range, allowing for boundary
 * transitions, and false otherwise.
 *
/**
 * @brief TODO: Describe `range`.
 *
 * @param range TODO.
 * @param range_barrier TODO.
 * @return TODO.
 */
 * @param[in] range_barrier The tolerance range (in degrees) around the barrier's start and end angles.
 * 
 * @return bool True if the current angle is within the specified range, false otherwise.
 */
bool system_monitoring_range_barrier(uint8_t range_barrier)
{
    pivot_physical_config physical_barrier_config = {};
    data_app_load(DATA_TYPE_PHYSICAL_BARRIER, &physical_barrier_config);

    if (*system_monitoring_current_angle != 655) 
    {
        uint16_t margins[2][2] = {
            {(physical_barrier_config.start_angle_physical_barrier + 360 - range_barrier) % 360,
            (physical_barrier_config.start_angle_physical_barrier + range_barrier) % 360},
            {(physical_barrier_config.end_angle_physical_barrier + 360 - range_barrier) % 360,
            (physical_barrier_config.end_angle_physical_barrier + range_barrier) % 360}
        };

        for (int i = 0; i < 2; ++i) 
        {
            uint16_t margin_low = margins[i][0];
            uint16_t margin_high = margins[i][1];

            if (margin_low > margin_high) 
            {
                if (*system_monitoring_current_angle >= margin_low || *system_monitoring_current_angle <= margin_high) 
                {
                    return true;
                }
            } 
            else 
            {
                if (*system_monitoring_current_angle >= margin_low && *system_monitoring_current_angle <= margin_high) 
                {
                    return true;
                }
            }
        }
    }

    return false;
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
/**
 * @brief TODO: Describe `handle`.
 *
 * @param pxTimer TODO.
 * @return TODO.
 */
 * @param pxTimer Timer handle (unused).
 */
static void system_monitoring_timer(TimerHandle_t pxTimer)
{
    // send IDP 0 (current status)
    system_monitoring_callback("#00$", comm_main_mode);

    // save current Timestamp
    time_t timestamp_now = rtc_app_get_timestamp(false);
    data_app_save(DATA_TYPE_TIMESTAMP, &timestamp_now, sizeof(timestamp_now));
}

/**
 * @brief Starts the system monitoring task with the provided configuration.
 *
 * This function initializes the system monitoring task with the specified configuration.
 *
 * @param virtual_config Configuration for system monitoring.
/**
 * @brief TODO: Describe `monitoring`.
 *
 * @param monitoring_time TODO.
 * @return TODO.
 */
 * @param monitoring_time Time interval for system monitoring (in minutes).
 */
void system_monitoring_start(const pivot_physical_config physical_config, const pivot_virtual_config virtual_config, uint8_t monitoring_time)
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

    if((physical_config.start_angle_physical_barrier == 0 && physical_config.end_angle_physical_barrier == 0)
    && (virtual_config.start_angle_virtual_barrier == 0 && virtual_config.end_angle_virtual_barrier == 0))
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
 * @brief TODO: Describe `system_monitoring_pivot_shutdown`.
 *
 * @param shutdown_reason TODO.
 * @param idp TODO.
 * @param scheduling_id TODO.
 * @param origin TODO.
 */
void system_monitoring_pivot_shutdown(hangs_up_status shutdown_reason, idp_type idp, char *scheduling_id, char *origin)
{
    char str_out[200] = {};

    uint8_t idp_28 = IDP_28;
    time_t timestamp = 0;
    char str_date_time[70] = {};

    bool pivot_is_on_barrier = false;

    timestamp = rtc_app_get_timestamp(false);
    rtc_app_get_str_date_time(timestamp, str_date_time);

    char *reason_str = NULL;
    bool is_external_agent = false;

    switch (shutdown_reason)
    {
        case TYPE_HANGS_UP_VIRTUAL_BARRIER:
        {
            reason_str = "virtual_barrier";
            break;
        }
        case TYPE_HANGS_UP_MANUAL:
        {
            reason_str = "manual";
            break;
        }
        case TYPE_HANGS_UP_SCHEDULE_14:
        {
            reason_str = "scheduling_14";
            break;
        }
        case TYPE_HANGS_UP_SCHEDULE_15:
        {
            reason_str = "scheduling_15";
            break;
        }
        case TYPE_HANGS_UP_SCHEDULE_16:
        {
            reason_str = "scheduling_16";
            break;
        }
        case TYPE_HANGS_UP_SCHEDULE_17:
        {
            reason_str = "scheduling_17";
            break;
        }
        case TYPE_HANGS_UP_BROWNOUT:
        {
            reason_str = "brownout";
            break;
        }
        case TYPE_HANGS_UP_PIVOT_WITHOUT_WATER:
        {
            reason_str = "pivot_without_water";
            break;
        }
        case TYPE_HANGS_UP_SOIL_APP:
        {
            reason_str = "soil_app";
            is_external_agent = true;
            break;
        }
        case TYPE_HANGS_UP_IRRIGABRAS_APP:
        {
            reason_str = "nimbus_app";
            is_external_agent = true;
            break;
        }
        default:
        {
            break;
        }
    }

    if (reason_str != NULL)
    {
        arg_pair_t arg_pair[10]; 
        idp_parser_build_arg_pairs_pivot_shutdown(
            arg_pair,
            reason_str,
            &idp_28,
            system_id,
            origin,
            &idp,
            scheduling_id,
            &pivot_is_on_barrier,
            &global_angle,
            str_date_time,
            is_external_agent
        );

        idp_parser_create_package(str_out, arg_pair);   

        data_app_save(DATA_TYPE_REASON_HANG_UP, &str_out, strlen(str_out));
        
        vTaskDelay(pdMS_TO_TICKS(1000));
        system_monitoring_callback("#28$", comm_main_mode);
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