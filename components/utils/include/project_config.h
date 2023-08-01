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
 * @enum idp_type
 * @brief Enumerates different packet identifiers for communication.
 *
 * This enum defines packet identifiers used for communication. Each identifier represents
 * a specific action or configuration parameter.
 *
 * @var IDP_0 Read actions
 * @var IDP_1 Save actions
 * @var IDP_2 Network configurations
 * @var IDP_3 Pivot configurations
 * @var IDP_4 ECO mode configuration
 * @var IDP_5 Sector configurations
 * @var IDP_6 New modem configurations
 * @var IDP_7 Angle and timestamp configuration
 * @var IDP_8 RSSI status
 * @var IDP_9 Obtain Traceroute
 * @var IDP_10 Obtain Noise
 * @var IDP_11 GPRS connection status
 * @var IDP_12 Pivot history
 * @var IDP_13 Schedule delete
 * @var IDP_14 Scheduling type 1 (On by date and off by date)
 * @var IDP_15 Schedule type 2 (Turn on by date and turn off by angle)
 * @var IDP_16 Schedule type 3 (Only turns off by date)
 * @var IDP_INVALID Invalid packet identifier
 */
typedef enum
{
    IDP_0 = 0,
    IDP_1,
    IDP_2,
    IDP_3,
    IDP_4,
    IDP_5,
    IDP_6,
    IDP_7,
    IDP_8,
    IDP_9,
    IDP_10,
    IDP_11,
    IDP_12,
    IDP_13,
    IDP_14,
    IDP_15,
    IDP_16,
	IDP_30 = 30,
    IDP_INVALID = 255
} idp_type;

typedef enum
{
	COMM_HTTP_POST = 0,
	COMM_HTTP_GET,
	COMM_MQTT,
} comm_type;

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
 * @brief Configuration angles.
 *
 * Structure defining the configuration angles.
 */
typedef struct
{
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
    char contactor[30];       /*!< Contactor type */ //todo: usar isso nas classes
    char pressure[30];   /*!< Pressure switch type *///todo usar isso nas classes
    uint16_t pressurization_time;   /*!< Pressurization time */
    uint8_t on_off_time;            /*!< On/off time */
} pivot_config;

/**
 * @brief Configuration parameters.
 *
 * Structure defining the network configuration parameters.
 */
typedef struct __attribute__((__packed__))
{
    char gprs_id[30];           /*!< GPRS ID */
    char modem_apn[30]; //todo passar esse valor para o edu
    char wifi_ssid[30]; //todo trocar o wifi por esse valor
    char wifi_pass[30]; //todo trocar o wifi por esse valor
} network_config;

/**
 * @brief Configuration parameters.
 *
 * Structure defining the eco mode configuration parameters.
 */
typedef struct __attribute__((__packed__)) //todo: alterar as classes para esse padrão
{
	time_t start_time;              /*!< Start time */
	time_t end_time;                /*!< End time */
} eco_mode_config;

/**
 * @brief Configuration parameters.
 *
 * Structure defining the sectors configuration parameters.
 */
typedef struct __attribute__((__packed__)) //todo: alterar as classes para esse padrão
{
    uint8_t sector_number;	//todo olhar isso na implementação
    pivot_sectors sectors[CONFIG_SECTORS_MAX_VALUE];   /*!< Array of pivot sectors */
} sector_config;

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
	bool is_running;
    pivot_actions actions;          /*!< Pivot actions */
    uint16_t start_angle;           /*!< Start angle */
    uint16_t end_angle;             /*!< End angle */
    time_t start_date;              /*!< Start date */
    time_t end_date;                /*!< End date */
} pivot_history;

/**
 * @brief Application callback function.
 *
 * Function signature for the application callback function.
 *
 * @param state The state of the main callback request.
 * @param buffer The buffer containing the data associated with the request.
 */
typedef void (*app_callback)(const char* buffer_request, comm_type communication);


extern uint16_t global_angle;

#endif /* COMPONENTS_UTILS_INCLUDE_PROJECT_CONFIG_H_ */
