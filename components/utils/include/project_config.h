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
#include <inttypes.h>

#include <time.h>

/* Some scripts may not require all the parameters passed. Use this to avoid
 * compiler warnings about unused variables. */
#define UNUSED(x)               (void)(sizeof(x))

/**
 * @defgroup PROJECT_CONFIG Project Configuration
 * @{
 */

/**
 * @brief Firmware version.
 */
#define CONFIG_FW_VERSION           ("v2.9.3")

/**
 * @brief Number of controlled outputs in the new product.
 */
#define CONFIG_ACTUATION_CHANNEL_COUNT (4)

/**
 * @brief Maximum text length for shutdown reason fields.
 */
#define CONFIG_ACTUATION_SHUTDOWN_TEXT_SIZE (32)

/**
 * @brief Maximum text length for operation history fields.
 */
#define CONFIG_PUMP_HISTORY_TEXT_SIZE (32)

/**
 * @brief Default pulse duration for each ON/OFF relay command.
 */
#define CONFIG_ACTUATION_DEFAULT_RELAY_PULSE_MS (10000)

/**
 * @brief Default interval, in seconds, for idle status monitoring.
 */
#define CONFIG_ACTUATION_DEFAULT_IDLE_READ_TIME_SEC (10)

/**
 * @brief Default interval, in minutes, for periodic #00 status publish.
 */
#define CONFIG_ACTUATION_DEFAULT_STATUS_PUBLISH_MIN (1)

/**
 * @brief Default GPIO level interpreted as ON by status inputs.
 */
#define CONFIG_ACTUATION_DEFAULT_STATUS_ACTIVE_LEVEL (false)

/**
 * @brief Pump startup delay after channel 1 relay is enabled.
 */
#define CONFIG_PUMP_STAGE_1_DELAY_MS (10000)

/**
 * @brief Pump startup delay after channel 2 relay is enabled.
 */
#define CONFIG_PUMP_STAGE_2_DELAY_MS (30000)

/**
 * @brief Pump startup delay after channel 3 relay is enabled.
 */
#define CONFIG_PUMP_STAGE_3_DELAY_MS (30000)

/**
 * @brief Default motor ramp after channel 1 relay is enabled.
 */
#define CONFIG_PUMP_RAMP_1_DELAY_MS (5000)

/**
 * @brief Default motor ramp after channel 2 relay is enabled.
 */
#define CONFIG_PUMP_RAMP_2_DELAY_MS (5000)

/**
 * @brief Default motor ramp after channel 3 relay is enabled.
 */
#define CONFIG_PUMP_RAMP_3_DELAY_MS (5000)

/**
 * @brief Default motor ramp after channel 4 relay is enabled.
 */
#define CONFIG_PUMP_RAMP_4_DELAY_MS (5000)

/**
 * @brief Pump stabilization delay after channel 4 relay is enabled.
 */
#define CONFIG_PUMP_STAGE_4_DELAY_MS (0)

/**
 * @brief Pump runtime status monitoring interval.
 */
#define CONFIG_PUMP_MONITOR_INTERVAL_MS (500)

/**
 * @brief Interval used to log pump stage timer progress.
 */
#define CONFIG_PUMP_STAGE_LOG_INTERVAL_MS (10000)

/**
 * @brief Interval used to log pump stage heartbeat dots.
 */
#define CONFIG_PUMP_STAGE_HEARTBEAT_INTERVAL_MS (1000)

/**
 * @brief Minimum interval used to publish IDP 29 progress packets during timers.
 *
 * Phase start/end, RUNNING, STOPPING, FAULT and STOPPED events are still sent
 * immediately. This only limits periodic timer progress to reduce UART load.
 */
#define CONFIG_PUMP_PROGRESS_PUBLISH_INTERVAL_MS (5000)

/**
 * @brief Duration used to energize all OFF relays during shutdown.
 */
#define CONFIG_PUMP_STOP_RELAY_TIME_MS (10000)

/**
 * @brief Maximum number of scheduling values.
 *
 * This define specifies the maximum number of scheduling values allowed in the project.
 * It can be used to define the size of arrays or data structures related to scheduling.
 */
#define CONFIG_SCHEDULING_MAX_VALUE     (10)

/**
 * @brief Maximum number of history values.
 *
 * This define specifies the maximum number of history values allowed in the project.
 * It can be used to define the size of arrays or data structures related to history.
 */
#define CONFIG_HISTORY_MAX_VALUE        (20)

/**
 * @brief Undefined value for actions.
 */
#define CONFIG_ACTIONS_UNDEF_VALUE      (655)

/**
 * @brief HTTP error response code.
 */
#define CONFIG_HTTP_ERROR               ("400")

/**
 * @brief HTTP OK response code.
 */
#define CONFIG_HTTP_OK                  ("200")

/**
 * @brief Maximum delay accepted when executing a pending start schedule.
 */
#define CONFIG_SCHEDULING_START_GRACE_SEC (1800)

/**
 * @brief Factory recovery identifier used only when no valid NVS identity exists.
 */
#define CONFIG_DEFAULT_DEVICE_ID "newproductteste_1"


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
 * @var IDP_3 Pump actuation configurations
 * @var IDP_4 Rush mode configuration
 * @var IDP_5 Sector configurations
 * @var IDP_6 New modem configurations
 * @var IDP_7 Angle and timestamp configuration
 * @var IDP_8 RSSI status
 * @var IDP_9 Obtain Traceroute
 * @var IDP_10 Obtain Noise
 * @var IDP_11 GPRS connection status
 * @var IDP_12 Pump history
 * @var IDP_13 Schedule delete
 * @var IDP_14 Scheduling type 1 (On by date and off by date)
 * @var IDP_15 Removed angle schedule
 * @var IDP_16 Schedule type 3 (Only turns off by date)
 * @var IDP_29 Pump startup progress event
 * @var IDP_42 Heartbeat with the connectivity ESP over GPRS UART
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
    IDP_17,
    IDP_18,
    IDP_21 = 21,
    IDP_22,
    IDP_23,
    IDP_26 = 26,
    IDP_27,
    IDP_28,
    IDP_29,
    IDP_30 = 30,
    IDP_31,
    IDP_32,
    IDP_42 = 42,
    IDP_90 = 90,
    IDP_91,
    IDP_INVALID = 255
} idp_type;

/**
 * @enum comm_type
 * @brief Enumerates different communication types.
 *
 * This enum defines communication types used for packet transmission.
 *
 * @var COMM_HTTP_POST HTTP POST communication
 * @var COMM_HTTP_GET HTTP GET communication
 * @var COMM_MQTT MQTT communication
 * @var COMM_RF RadioLoraMesh communication
 */
typedef enum
{
    COMM_HTTP_POST = 0,
    COMM_HTTP_GET,
    COMM_MQTT,
    COMM_RF,
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
    PIVOT_UNKNOWN = 8,     /*!< Unknown pivot state */
    PIVOT_SUSPENDED = 9    /*!< Pivot suspended inside the configured Rush Mode window */
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
    uint16_t percentimeter;   /*!< Percentage value from 0 to 100 */
    char user[50];
} pivot_actions;

/**
 * @brief Command for one new-product actuation channel.
 */
typedef enum
{
    ACTUATION_COMMAND_NONE = 0,     /*!< No relay pulse requested. */
    ACTUATION_COMMAND_ON = 1,       /*!< Pulse the ON relay for the channel. */
    ACTUATION_COMMAND_OFF = 2,      /*!< Pulse the OFF relay for the channel. */
} actuation_channel_command;

/**
 * @brief Live status read from one new-product actuation input.
 */
typedef enum
{
    ACTUATION_STATUS_OFF = 0,       /*!< Status input indicates OFF. */
    ACTUATION_STATUS_ON = 1,        /*!< Status input indicates ON. */
    ACTUATION_STATUS_UNKNOWN = 2,   /*!< Status is not available. */
} actuation_channel_status;

/**
 * @brief Commands for the four ON/OFF actuation channels.
 */
typedef struct __attribute__((__packed__))
{
    uint8_t channels[CONFIG_ACTUATION_CHANNEL_COUNT]; /*!< actuation_channel_command per channel. */
    char user[50];                                    /*!< Origin reserved for the future communication layer. */
} actuation_actions;

/**
 * @brief Live ON/OFF status for the four actuation channels.
 */
typedef struct __attribute__((__packed__))
{
    uint8_t channels[CONFIG_ACTUATION_CHANNEL_COUNT]; /*!< actuation_channel_status per channel. */
} actuation_status;

/**
 * @brief Last shutdown reason categories for the new product.
 */
typedef enum
{
    ACTUATION_SHUTDOWN_REASON_NONE = 0,
    ACTUATION_SHUTDOWN_REASON_COMMAND_OFF,
    ACTUATION_SHUTDOWN_REASON_STARTUP_FAULT,
    ACTUATION_SHUTDOWN_REASON_RUNTIME_FAULT,
    ACTUATION_SHUTDOWN_REASON_BROWNOUT,
    ACTUATION_SHUTDOWN_REASON_BOOT,
} actuation_shutdown_reason;

/**
 * @brief Runtime shutdown event produced by actuation_app and persisted by system_manager.
 */
typedef struct
{
    uint32_t sequence;                                /*!< Monotonic runtime event counter. */
    uint8_t reason;                                  /*!< actuation_shutdown_reason value. */
    uint8_t motor;                                   /*!< Failed motor/channel, or 0 when not applicable. */
    char user[50];                                   /*!< Command user or last known operator. */
    char phase[CONFIG_ACTUATION_SHUTDOWN_TEXT_SIZE]; /*!< Pump phase when the shutdown happened. */
} actuation_shutdown_info;

/**
 * @brief Configuration for the new-product actuation layer.
 */
typedef struct __attribute__((__packed__))
{
    uint16_t relay_pulse_time_ms;     /*!< Pulse duration used by OFF relays during stop. */
    uint16_t idle_read_time_sec;      /*!< Internal status read cadence while stopped. */
    bool status_active_level;         /*!< GPIO level interpreted as ON by status inputs. */
    uint16_t ramp_1_delay_sec;        /*!< Motor ramp delay after channel 1 ON relay is enabled. */
    uint16_t stage_1_delay_sec;       /*!< Delay after channel 1 ON relay is enabled. */
    uint16_t ramp_2_delay_sec;        /*!< Motor ramp delay after channel 2 ON relay is enabled. */
    uint16_t stage_2_delay_sec;       /*!< Delay after channel 2 ON relay is enabled. */
    uint16_t ramp_3_delay_sec;        /*!< Motor ramp delay after channel 3 ON relay is enabled. */
    uint16_t stage_3_delay_sec;       /*!< Delay after channel 3 ON relay is enabled. */
    uint16_t ramp_4_delay_sec;        /*!< Motor ramp delay after channel 4 ON relay is enabled. */
    uint16_t stage_4_delay_sec;       /*!< Delay after channel 4 ON relay is enabled. */
    uint16_t status_publish_time_min; /*!< Periodic #00 publish interval, independent from idle reads. */
} actuation_config;

/**
 * @brief Number of configuration fields in actuation_config.
 */
#define ACTUATION_CONFIG_VAR_COUNT (12)

/**
 * @brief Configuration parameters.
 *
 * Structure defining the network configuration parameters.
 */
typedef struct __attribute__((__packed__))
{
    char gprs_id[50];           /*!< GPRS ID */
    char modem_apn[50];
    char wifi_ssid[50];
    char wifi_pass[50];
} network_config;

/**
 * @brief Configuration parameters.
 *
 * How many configuration parameters in network_config struct.
 */
#define NETWORK_CONFIG_VAR_COUNT    (4)

/**
 * @brief Configuration parameters.
 *
 * Structure defining the rush mode configuration parameters.
 */
typedef struct __attribute__((__packed__)) //todo: alterar as classes para esse padrão
{
    time_t start_time;              /*!< Start time in seconds since 00:00. */
    time_t end_time;                /*!< End time in seconds since 00:00. */
    bool enable;                     /*!< Enable or Disable */
} rush_mode_config;

/**
 * @brief Persisted Rush Mode window state.
 *
 * Structure used to keep the current active Rush Mode window context.
 * When `valid` is true, `actions` stores the pivot state captured before
 * Rush Mode turns the pivot off and that state must be restored when the
 * current window ends. When `valid` is false and `window_end_time` is
 * non-zero, the current window was overridden by command and must stay
 * suspended until `window_end_time`.
 */
typedef struct __attribute__((__packed__))
{
    bool valid;                     /*!< Indicates whether `actions` contains a state to be restored. */
    time_t window_start_time;       /*!< Absolute start timestamp of the current protected window. */
    time_t window_end_time;         /*!< Absolute end timestamp of the current protected window. */
    pivot_actions actions;          /*!< Pivot state to be restored when `valid` is true. */
} rush_mode_saved_state;

/**
 * @brief Configuration parameters.
 *
 * How many configuration parameters in rush_mode_config struct.
 */
#define RUSH_MODE_CONFIG_VAR_COUNT   (3)

/**
 * @brief Configuration parameters.
 * 
 * Defines the principal communication mode of the system for periodic comm.
 */
typedef struct __attribute__((__packed__))
{
    char comm_main_mode_config[50];
} comm_main_mode_config;

/**
 * @brief Configuration parameters.
 *
 * How many configuration parameters in comm_main_mode_config struct.
 */
#define COMM_MAIN_MODE_CONFIG_VAR_COUNT   (1)

/**
 * @brief Pump schedule that starts and later stops the complete sequence.
 */
typedef struct __attribute__((__packed__))
{
    char scheduling_id[50];         /*!< Generated schedule identifier. */
    time_t start_date;              /*!< Absolute Unix timestamp for pump start. */
    time_t end_date;                /*!< Absolute Unix timestamp for pump stop. */
    char user[50];                  /*!< User that created the schedule. */
    bool started;                   /*!< True after the start command has been issued. */
} pump_scheduling_date;

/**
 * @brief Pump schedule that only requests a stop.
 */
typedef struct __attribute__((__packed__))
{
    char scheduling_id[50];         /*!< Generated schedule identifier. */
    time_t end_date;                /*!< Absolute Unix timestamp for pump stop. */
    char user[50];                  /*!< User that created the schedule. */
} pump_scheduling_off_date;

/**
 * @brief Persisted operation history for the pump product.
 */
typedef struct __attribute__((__packed__))
{
    bool valid;                                             /*!< True when this slot contains a history item. */
    bool active;                                            /*!< True while the operation is running or starting. */
    time_t start_date;                                      /*!< Timestamp when the start command was accepted. */
    time_t end_date;                                        /*!< Timestamp when the operation ended, or 0 while active. */
    time_t event_date;                                      /*!< Timestamp of the stop/fault event. */
    uint8_t stop_motor;                                     /*!< Motor related to the stop/fault, or 0. */
    char user[50];                                         /*!< User that requested the start. */
    char start_reason[CONFIG_PUMP_HISTORY_TEXT_SIZE];       /*!< command, schedule, boot recovery, etc. */
    char stop_reason[CONFIG_PUMP_HISTORY_TEXT_SIZE];        /*!< command_off, startup_fault, runtime_fault, etc. */
    char stop_phase[CONFIG_PUMP_HISTORY_TEXT_SIZE];         /*!< starting_ramp, running, boot, etc. */
    char reset_reason[CONFIG_PUMP_HISTORY_TEXT_SIZE];       /*!< brownout, panic, none, etc. */
} pump_history;


/**
 * @brief Application callback function.
 *
 * Function signature for the application callback function.
 *
 * @param buffer_request The buffer containing the data associated with the request.
 * @param communication The type of communication (HTTP POST, HTTP GET, MQTT).
 */
typedef void (*app_callback)(const char* buffer_request, comm_type communication);


/** @var system_id
 *  @brief Local variable for the system ID.
 */
extern char system_id[50];

/**
 * @brief Global comm_main_mode variable.
 */
extern comm_type comm_main_mode;

/** @} */ // end of PROJECT_CONFIG group

#endif /* COMPONENTS_UTILS_INCLUDE_PROJECT_CONFIG_H_ */
