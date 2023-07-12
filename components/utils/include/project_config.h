/**
 * @file project_config.h
 * @brief Configuration and parameter definitions for the project.
 *
 * This file contains configuration and parameter definitions for the project.
 */

#ifndef COMPONENTS_UTILS_INCLUDE_PROJECT_CONFIG_H_
#define COMPONENTS_UTILS_INCLUDE_PROJECT_CONFIG_H_

/* C base */
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>


/* Some scripts may not require all the parameters passed. Use this to avoid
 * compiler warnings about unused variables. */
#define UNUSED(x)               (void)(sizeof(x))

/**
 * @def CONFIG_SCHEDULING_MAX_VALUE
 * @brief Maximum number of scheduling values.
 *
 * This define specifies the maximum number of scheduling values allowed in the project.
 * It can be used to define the size of arrays or data structures related to scheduling.
 */
#define CONFIG_SCHEDULING_MAX_VALUE    (10)

/**
 * @def CONFIG_HISTORY_MAX_VALUE
 * @brief Maximum number of history values.
 *
 * This define specifies the maximum number of history values allowed in the project.
 * It can be used to define the size of arrays or data structures related to history.
 */
#define CONFIG_HISTORY_MAX_VALUE       (20)

/**
 * @def CONFIG_SECTORS_MAX_VALUE
 * @brief Maximum number of sectors.
 *
 * This define specifies the maximum number of sectors allowed in the project.
 * It can be used to define the size of arrays or data structures related to sectors.
 */
#define CONFIG_SECTORS_MAX_VALUE       (04)

/* Public definitions ******************************************************/

/**
 * @brief Packet Identifier.
 *
 * Enumeration defining packet identifier values.
 */
typedef enum
{
    IDP_0 = 0,              /*!< Read actions */
    IDP_1 = 1,              /*!< Save actions */
    IDP_2 = 2,              /*!< Scheduling type 1 (On by date and off by date) */
    IDP_3 = 3,              /*!< Schedule type 2 (Turn on by date and turn off by angle) */
    IDP_4 = 4,              /*!< Schedule type 3 (Only turns off by date) */
    IDP_5 = 5,              /*!< Schedule type 4 (Only turns off by angle) */
    IDP_6 = 6,              /*!< Schedule delete */
    IDP_7 = 7,              /*!< Pivot off */
    IDP_INVALID = 255       /*!< Invalid packet identifier */
} idp_type;

/**
 * @brief Pivot actions states.
 *
 * Enumeration defining pivot actions states.
 */
typedef enum
{
    PIVOT_ON = 1,           /*!< Pivot on */
    PIVOT_OFF = 2,          /*!< Pivot off */
    PIVOT_CW = 3,           /*!< Pivot in ClockWise mode (advanced) */
    PIVOT_CCW = 4,          /*!< Pivot in Counter ClockWise mode (reverse) */
    PIVOT_DRY = 5,          /*!< Irrigation off */
    PIVOT_WET = 6,          /*!< Irrigation on */
    PIVOT_PRESSURIZING = 7, /*!< Pivot pressurizing */
    PIVOT_UNKNOWN = 0       /*!< Unknown pivot state */
} pivot_states;

/**
 * @brief Pivot actions parameters.
 *
 * Structure defining pivot actions parameters.
 */
typedef struct __attribute__((__packed__))
{
    uint8_t power_state;     /*!< Power state of the pivot (PIVOT_ON or PIVOT_OFF) */
    uint8_t rotation;        /*!< Rotation mode of the pivot (PIVOT_CW or PIVOT_CCW) */
    uint8_t watering_state;  /*!< Watering state of the pivot (PIVOT_DRY or PIVOT_WET) */
    uint8_t percentimeter;   /*!< Percentage value from 0 to 100 */
} pivot_actions;

/**
 * @brief Contactor type.
 *
 * Enumeration defining the contactor type.
 */
typedef enum
{
    CONTACTOR_NA = 1,       /*!< Normally open (NO) contactor */
    CONTACTOR_NF,           /*!< Normally closed (NC) contactor */
} contactor_type;

/**
 * @brief Pressure switch type.
 *
 * Enumeration defining the pressure switch type.
 */
typedef enum
{
    PRESSURE_SWITCH_NA = 1, /*!< Normally open (NO) pressure switch */
    PRESSURE_SWITCH_NF,     /*!< Normally closed (NC) pressure switch */
} pressure_switch_type;

/**
 * @brief Configuration angles.
 *
 * Structure defining the configuration angles.
 */
typedef struct
{
    uint8_t id;             /*!< ID of the configuration angle */
    uint16_t start_angle;   /*!< Start angle of the configuration */
    uint16_t end_angle;     /*!< End angle of the configuration */
} pivot_sectors;

/**
 * @brief Configuration parameters.
 *
 * Structure defining the configuration parameters.
 */
typedef struct __attribute__((__packed__))
{
    char pivot_id[30];                          /*!< Pivot ID */
    char gprs_id[30];               /*!< GPRS ID */
    contactor_type contactor;       /*!< Contactor type */
    pressure_switch_type pressure_switch;   /*!< Pressure switch type */
    uint16_t pressurization_time;   /*!< Pressurization time */
    uint8_t on_off_time;            /*!< On/off time */
    bool eco_mode;                  /*!< Eco mode enabled flag */
    time_t start_time;              /*!< Start time */
    time_t end_time;                /*!< End time */
    bool sector_enabled;            /*!< Sector enabled flag */
    pivot_sectors sectors[CONFIG_SECTORS_MAX_VALUE];   /*!< Array of pivot sectors */
} pivot_config;

/**
 * @brief Scheduling date parameters.
 *
 * Structure defining the scheduling parameters based on date.
 */
typedef struct __attribute__((__packed__))
{
    char scheduling_id[30];         /*!< Scheduling ID */
    bool is_running;                /*!< Flag indicating if the scheduling is running */
    time_t start_date;              /*!< Start date */
    time_t end_date;                /*!< End date */
    pivot_actions actions;          /*!< Pivot actions */
} pivot_scheduling_date;

/**
 * @brief Scheduling angle parameters.
 *
 * Structure defining the scheduling parameters based on angle.
 */
typedef struct __attribute__((__packed__))
{
    char scheduling_id[30];         /*!< Scheduling ID */
    bool is_running;                /*!< Flag indicating if the scheduling is running */
    time_t start_date;              /*!< Start date */
    uint16_t end_angle;             /*!< End angle */
    pivot_actions actions;          /*!< Pivot actions */
} pivot_scheduling_angle;

/**
 * @brief History parameters.
 *
 * Structure defining the history parameters.
 */
typedef struct __attribute__((__packed__))
{
    bool is_running;                /*!< Flag indicating if the history is running */
    time_t start_date;              /*!< Start date */
    time_t end_date;                /*!< End date */
    uint16_t start_angle;           /*!< Start angle */
    uint16_t end_angle;             /*!< End angle */
    pivot_actions actions;          /*!< Pivot actions */
} pivot_history;

/**
 * @brief Main callback request states.
 *
 * Enumeration defining the states for the main callback request.
 */
typedef enum
{
    CALL_LOAD_ACTION = 1,               /*!< Load configuration read request */
    CALL_SAVE_ACTION,                   /*!< Save configuration read request */
    CALL_READ_ACTION,                   /*!< Read configuration read request */
    CALL_SAVE_CONFIG,                   /*!< Save configuration request */
    CALL_READ_CONFIG,                   /*!< Read configuration request */
    CALL_SAVE_SCHEDULE_DATE,            /*!< Save scheduling based on date request */
    CALL_LOAD_SCHEDULE_DATE,            /*!< Load scheduling based on date request */
    CALL_DELETE_SCHEDULE_DATE,          /*!< Delete scheduling based on date request */
    CALL_SAVE_SCHEDULE_ANGLE,           /*!< Save scheduling based on angle request */
    CALL_LOAD_SCHEDULE_ANGLE,           /*!< Load scheduling based on angle request */
    CALL_DELETE_SCHEDULE_ANGLE,         /*!< Delete scheduling based on angle request */
    CALL_LOAD_HISTORY,                  /*!< Load history request */
    CALL_MANUAL_PIVOT,                  /*!< Manual pivot request */
    CALL_OFF_PIVOT                      /*!< Pivot off request */
} app_call_states;

/**
 * @brief Application callback function.
 *
 * Function signature for the application callback function.
 *
 * @param state The state of the main callback request.
 * @param buffer The buffer containing the data associated with the request.
 */
typedef void (*app_callback)(app_call_states state, void* buffer);

#endif /* COMPONENTS_UTILS_INCLUDE_PROJECT_CONFIG_H_ */
