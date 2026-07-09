/**
 * @file system_monitoring.c
 * @brief Monitors the companion connectivity ESP through IDP 42.
 */

#include "system_monitoring.h"

#include "FreeRTOS_defines.h"
#include "log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

#define SYSTEM_MONITORING_TAG "system_monitoring"
#define SYSTEM_MONITORING_HEARTBEAT_INTERVAL_MS (30000U)
#define SYSTEM_MONITORING_HEARTBEAT_TIMEOUT_MS (90000U)

static TaskHandle_t system_monitoring_task_handle = NULL;
static app_callback system_monitoring_send_callback = NULL;
static portMUX_TYPE system_monitoring_mux = portMUX_INITIALIZER_UNLOCKED;
static TickType_t system_monitoring_last_rx_tick = 0;
static bool system_monitoring_alive = false;
static bool system_monitoring_timeout_logged = false;

static void system_monitoring_send_ping(void)
{
    char packet[100] = {};

    if (system_monitoring_send_callback == NULL)
    {
        return;
    }

    snprintf(packet, sizeof(packet), "#42-%s-PING$", system_id);
    system_monitoring_send_callback(packet, COMM_MQTT);
}

static void system_monitoring_task(void *arg)
{
    TickType_t last_ping_tick = xTaskGetTickCount();

    UNUSED(arg);

    while (true)
    {
        TickType_t now = xTaskGetTickCount();
        TickType_t last_rx;
        bool alive;
        bool timeout_logged;

        taskENTER_CRITICAL(&system_monitoring_mux);
        last_rx = system_monitoring_last_rx_tick;
        alive = system_monitoring_alive;
        timeout_logged = system_monitoring_timeout_logged;
        taskEXIT_CRITICAL(&system_monitoring_mux);

        if ((now - last_ping_tick) >= pdMS_TO_TICKS(SYSTEM_MONITORING_HEARTBEAT_INTERVAL_MS))
        {
            system_monitoring_send_ping();
            last_ping_tick = now;
        }

        if ((now - last_rx) >= pdMS_TO_TICKS(SYSTEM_MONITORING_HEARTBEAT_TIMEOUT_MS))
        {
            if (!timeout_logged)
            {
                LOG_WARNING(SYSTEM_MONITORING_TAG,
                            "HEARTBEAT",
                            "Connectivity ESP heartbeat timeout; no reset is performed by this board");
            }

            taskENTER_CRITICAL(&system_monitoring_mux);
            system_monitoring_alive = false;
            system_monitoring_timeout_logged = true;
            taskEXIT_CRITICAL(&system_monitoring_mux);
        }
        else if (alive)
        {
            taskENTER_CRITICAL(&system_monitoring_mux);
            system_monitoring_timeout_logged = false;
            taskEXIT_CRITICAL(&system_monitoring_mux);
        }

        vTaskDelay(pdMS_TO_TICKS(1000U));
    }
}

esp_err_t system_monitoring_init(const app_callback send_callback)
{
    if (send_callback == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    system_monitoring_send_callback = send_callback;

    taskENTER_CRITICAL(&system_monitoring_mux);
    system_monitoring_last_rx_tick = xTaskGetTickCount();
    system_monitoring_alive = false;
    system_monitoring_timeout_logged = false;
    taskEXIT_CRITICAL(&system_monitoring_mux);

    if (system_monitoring_task_handle == NULL)
    {
        BaseType_t result = xTaskCreate(system_monitoring_task,
                                        SYSTEM_MONITORING_HEARTBEAT_TASK_NAME,
                                        SYSTEM_MONITORING_HEARTBEAT_TASK_SIZE,
                                        NULL,
                                        SYSTEM_MONITORING_HEARTBEAT_TASK_PRIORITY,
                                        &system_monitoring_task_handle);
        if (result != pdPASS)
        {
            system_monitoring_task_handle = NULL;
            return ESP_ERR_NO_MEM;
        }
    }

    LOG_SUCCESS(SYSTEM_MONITORING_TAG,
                "HEARTBEAT",
                "Connectivity heartbeat initialized on GPRS UART");
    return ESP_OK;
}

void system_monitoring_heartbeat_feed(const char *heartbeat_state)
{
    bool was_alive;

    taskENTER_CRITICAL(&system_monitoring_mux);
    was_alive = system_monitoring_alive;
    system_monitoring_last_rx_tick = xTaskGetTickCount();
    system_monitoring_alive = true;
    system_monitoring_timeout_logged = false;
    taskEXIT_CRITICAL(&system_monitoring_mux);

    if (!was_alive)
    {
        LOG_SUCCESS(SYSTEM_MONITORING_TAG,
                    "HEARTBEAT",
                    "Connectivity heartbeat received (%s)",
                    (heartbeat_state != NULL && heartbeat_state[0] != '\0')
                        ? heartbeat_state
                        : "unknown");
    }
}

bool system_monitoring_connectivity_alive(void)
{
    bool alive;

    taskENTER_CRITICAL(&system_monitoring_mux);
    alive = system_monitoring_alive;
    taskEXIT_CRITICAL(&system_monitoring_mux);
    return alive;
}

void system_monitoring_shutdown(void)
{
    if (system_monitoring_task_handle != NULL)
    {
        vTaskDelete(system_monitoring_task_handle);
        system_monitoring_task_handle = NULL;
    }

    taskENTER_CRITICAL(&system_monitoring_mux);
    system_monitoring_alive = false;
    system_monitoring_timeout_logged = false;
    taskEXIT_CRITICAL(&system_monitoring_mux);
}
