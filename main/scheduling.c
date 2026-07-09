/**
 * @file scheduling.c
 * @brief Date-based start/stop scheduling for the pump product.
 */

#include "scheduling.h"

#include "actuation_app.h"
#include "data_app.h"
#include "FreeRTOS_defines.h"
#include "log.h"
#include "rtc_app.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <string.h>

#define SCHEDULING_TAG "scheduling"
#define SCHEDULING_POLL_INTERVAL_MS (1000U)

typedef enum
{
    SCHEDULING_ACTION_NONE = 0,
    SCHEDULING_ACTION_START,
    SCHEDULING_ACTION_STOP,
    SCHEDULING_ACTION_EXPIRED,
} scheduling_action_type;

typedef struct
{
    scheduling_action_type type;
    idp_type idp;
    char scheduling_id[50];
    char user[50];
} scheduling_pending_action;

static pump_scheduling_date scheduling_dates[CONFIG_SCHEDULING_MAX_VALUE] = {};
static pump_scheduling_off_date scheduling_off_dates[CONFIG_SCHEDULING_MAX_VALUE] = {};
static SemaphoreHandle_t scheduling_mutex = NULL;
static TaskHandle_t scheduling_task_handle = NULL;
static scheduling_event_callback scheduling_callback = NULL;

static bool scheduling_has_id(const char *scheduling_id)
{
    return (scheduling_id != NULL && scheduling_id[0] != '\0');
}

static void scheduling_copy_text(char *destination,
                                 size_t destination_size,
                                 const char *source,
                                 const char *fallback)
{
    const char *value = (source != NULL && source[0] != '\0') ? source : fallback;

    if (destination == NULL || destination_size == 0)
    {
        return;
    }

    memset(destination, 0, destination_size);
    if (value != NULL)
    {
        strncpy(destination, value, destination_size - 1U);
    }
}

static bool scheduling_windows_overlap(time_t start_a,
                                       time_t end_a,
                                       time_t start_b,
                                       time_t end_b)
{
    return (start_a < end_b && start_b < end_a);
}

static void scheduling_issue_command(const scheduling_pending_action *pending)
{
    actuation_actions actions = {};
    const char *event = "UNKNOWN";

    if (pending == NULL || pending->type == SCHEDULING_ACTION_NONE)
    {
        return;
    }

    scheduling_copy_text(actions.user,
                         sizeof(actions.user),
                         pending->user,
                         "scheduling");

    if (pending->type == SCHEDULING_ACTION_START)
    {
        actions.channels[0] = ACTUATION_COMMAND_ON;
        event = "STARTED";
        LOG_PROCESSING(SCHEDULING_TAG,
                       "Executing start schedule %s",
                       pending->scheduling_id);
        actuation_app_set_progress_mode(comm_main_mode);
        actuation_app_set_actions(actions, true);
    }
    else if (pending->type == SCHEDULING_ACTION_STOP)
    {
        for (uint8_t channel = 0; channel < CONFIG_ACTUATION_CHANNEL_COUNT; channel++)
        {
            actions.channels[channel] = ACTUATION_COMMAND_OFF;
        }

        event = "STOPPED";
        LOG_PROCESSING(SCHEDULING_TAG,
                       "Executing stop schedule %s",
                       pending->scheduling_id);
        actuation_app_set_progress_mode(comm_main_mode);
        actuation_app_set_actions(actions, true);
    }
    else
    {
        event = "EXPIRED";
        LOG_WARNING(SCHEDULING_TAG,
                    "SCHEDULING",
                    "Schedule %s expired without execution",
                    pending->scheduling_id);
    }

    if (scheduling_callback != NULL)
    {
        scheduling_callback(pending->idp,
                            pending->scheduling_id,
                            event,
                            pending->user);
    }
}

static void scheduling_scan(time_t now)
{
    scheduling_pending_action pending[CONFIG_SCHEDULING_MAX_VALUE * 2U] = {};
    size_t pending_count = 0;
    bool dates_changed = false;
    bool off_dates_changed = false;

    if (scheduling_mutex == NULL ||
        xSemaphoreTake(scheduling_mutex, portMAX_DELAY) != pdTRUE)
    {
        return;
    }

    for (size_t index = 0; index < CONFIG_SCHEDULING_MAX_VALUE; index++)
    {
        pump_scheduling_date *schedule = &scheduling_dates[index];

        if (!scheduling_has_id(schedule->scheduling_id))
        {
            continue;
        }

        if (now >= schedule->end_date)
        {
            scheduling_pending_action *action = &pending[pending_count++];
            action->type = schedule->started ? SCHEDULING_ACTION_STOP : SCHEDULING_ACTION_EXPIRED;
            action->idp = IDP_14;
            scheduling_copy_text(action->scheduling_id,
                                 sizeof(action->scheduling_id),
                                 schedule->scheduling_id,
                                 "unknown");
            scheduling_copy_text(action->user,
                                 sizeof(action->user),
                                 schedule->user,
                                 "scheduling");
            memset(schedule, 0, sizeof(*schedule));
            dates_changed = true;
            continue;
        }

        if (!schedule->started && now >= schedule->start_date)
        {
            scheduling_pending_action *action = &pending[pending_count++];
            action->type = ((now - schedule->start_date) <= CONFIG_SCHEDULING_START_GRACE_SEC)
                ? SCHEDULING_ACTION_START
                : SCHEDULING_ACTION_EXPIRED;
            action->idp = IDP_14;
            scheduling_copy_text(action->scheduling_id,
                                 sizeof(action->scheduling_id),
                                 schedule->scheduling_id,
                                 "unknown");
            scheduling_copy_text(action->user,
                                 sizeof(action->user),
                                 schedule->user,
                                 "scheduling");

            if (action->type == SCHEDULING_ACTION_START)
            {
                schedule->started = true;
            }
            else
            {
                memset(schedule, 0, sizeof(*schedule));
            }
            dates_changed = true;
        }
    }

    for (size_t index = 0; index < CONFIG_SCHEDULING_MAX_VALUE; index++)
    {
        pump_scheduling_off_date *schedule = &scheduling_off_dates[index];

        if (!scheduling_has_id(schedule->scheduling_id) || now < schedule->end_date)
        {
            continue;
        }

        scheduling_pending_action *action = &pending[pending_count++];
        action->type = SCHEDULING_ACTION_STOP;
        action->idp = IDP_16;
        scheduling_copy_text(action->scheduling_id,
                             sizeof(action->scheduling_id),
                             schedule->scheduling_id,
                             "unknown");
        scheduling_copy_text(action->user,
                             sizeof(action->user),
                             schedule->user,
                             "scheduling");
        memset(schedule, 0, sizeof(*schedule));
        off_dates_changed = true;
    }

    if (dates_changed)
    {
        data_app_save(DATA_TYPE_SCHEDULING_DATE,
                      scheduling_dates,
                      sizeof(scheduling_dates));
    }

    if (off_dates_changed)
    {
        data_app_save(DATA_TYPE_SCHEDULING_OFF_DATE,
                      scheduling_off_dates,
                      sizeof(scheduling_off_dates));
    }

    xSemaphoreGive(scheduling_mutex);

    for (size_t index = 0; index < pending_count; index++)
    {
        scheduling_issue_command(&pending[index]);
    }
}

static void scheduling_task(void *arg)
{
    UNUSED(arg);

    while (true)
    {
        scheduling_scan(rtc_app_get_timestamp(false));
        vTaskDelay(pdMS_TO_TICKS(SCHEDULING_POLL_INTERVAL_MS));
    }
}

esp_err_t scheduling_init(scheduling_event_callback callback)
{
    if (scheduling_mutex == NULL)
    {
        scheduling_mutex = xSemaphoreCreateMutex();
        if (scheduling_mutex == NULL)
        {
            return ESP_ERR_NO_MEM;
        }
    }

    scheduling_callback = callback;

    if (data_app_load(DATA_TYPE_SCHEDULING_DATE, scheduling_dates) != ESP_OK)
    {
        memset(scheduling_dates, 0, sizeof(scheduling_dates));
    }

    if (data_app_load(DATA_TYPE_SCHEDULING_OFF_DATE, scheduling_off_dates) != ESP_OK)
    {
        memset(scheduling_off_dates, 0, sizeof(scheduling_off_dates));
    }

    if (scheduling_task_handle == NULL)
    {
        BaseType_t result = xTaskCreate(scheduling_task,
                                        SCHEDULING_TASK_NAME,
                                        SCHEDULING_TASK_SIZE,
                                        NULL,
                                        SCHEDULING_TASK_PRIORITY,
                                        &scheduling_task_handle);
        if (result != pdPASS)
        {
            scheduling_task_handle = NULL;
            return ESP_ERR_NO_MEM;
        }
    }

    LOG_SUCCESS(SCHEDULING_TAG,
                "SCHEDULING",
                "Date scheduling initialized: IDP 14 and IDP 16");
    return ESP_OK;
}

esp_err_t scheduling_add_date(time_t start_date,
                              time_t end_date,
                              const char *user,
                              char *scheduling_id_out,
                              size_t scheduling_id_out_size)
{
    size_t free_position = CONFIG_SCHEDULING_MAX_VALUE;

    if (start_date <= 0 || end_date <= start_date ||
        scheduling_id_out == NULL || scheduling_id_out_size == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (scheduling_mutex == NULL ||
        xSemaphoreTake(scheduling_mutex, portMAX_DELAY) != pdTRUE)
    {
        return ESP_ERR_INVALID_STATE;
    }

    for (size_t index = 0; index < CONFIG_SCHEDULING_MAX_VALUE; index++)
    {
        if (!scheduling_has_id(scheduling_dates[index].scheduling_id))
        {
            if (free_position == CONFIG_SCHEDULING_MAX_VALUE)
            {
                free_position = index;
            }
            continue;
        }

        if (scheduling_windows_overlap(start_date,
                                       end_date,
                                       scheduling_dates[index].start_date,
                                       scheduling_dates[index].end_date))
        {
            xSemaphoreGive(scheduling_mutex);
            return ESP_ERR_INVALID_STATE;
        }
    }

    if (free_position == CONFIG_SCHEDULING_MAX_VALUE)
    {
        xSemaphoreGive(scheduling_mutex);
        return ESP_ERR_NO_MEM;
    }

    pump_scheduling_date *schedule = &scheduling_dates[free_position];
    memset(schedule, 0, sizeof(*schedule));
    data_app_gen_scheduling_key(schedule->scheduling_id);
    schedule->start_date = start_date;
    schedule->end_date = end_date;
    scheduling_copy_text(schedule->user, sizeof(schedule->user), user, "scheduling");

    esp_err_t result = data_app_save(DATA_TYPE_SCHEDULING_DATE,
                                     scheduling_dates,
                                     sizeof(scheduling_dates));
    if (result == ESP_OK)
    {
        scheduling_copy_text(scheduling_id_out,
                             scheduling_id_out_size,
                             schedule->scheduling_id,
                             "");
    }

    xSemaphoreGive(scheduling_mutex);
    return result;
}

esp_err_t scheduling_add_off_date(time_t end_date,
                                  const char *user,
                                  char *scheduling_id_out,
                                  size_t scheduling_id_out_size)
{
    size_t free_position = CONFIG_SCHEDULING_MAX_VALUE;

    if (end_date <= 0 || scheduling_id_out == NULL || scheduling_id_out_size == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (scheduling_mutex == NULL ||
        xSemaphoreTake(scheduling_mutex, portMAX_DELAY) != pdTRUE)
    {
        return ESP_ERR_INVALID_STATE;
    }

    for (size_t index = 0; index < CONFIG_SCHEDULING_MAX_VALUE; index++)
    {
        if (!scheduling_has_id(scheduling_off_dates[index].scheduling_id))
        {
            free_position = index;
            break;
        }
    }

    if (free_position == CONFIG_SCHEDULING_MAX_VALUE)
    {
        xSemaphoreGive(scheduling_mutex);
        return ESP_ERR_NO_MEM;
    }

    pump_scheduling_off_date *schedule = &scheduling_off_dates[free_position];
    memset(schedule, 0, sizeof(*schedule));
    data_app_gen_scheduling_key(schedule->scheduling_id);
    schedule->end_date = end_date;
    scheduling_copy_text(schedule->user, sizeof(schedule->user), user, "scheduling");

    esp_err_t result = data_app_save(DATA_TYPE_SCHEDULING_OFF_DATE,
                                     scheduling_off_dates,
                                     sizeof(scheduling_off_dates));
    if (result == ESP_OK)
    {
        scheduling_copy_text(scheduling_id_out,
                             scheduling_id_out_size,
                             schedule->scheduling_id,
                             "");
    }

    xSemaphoreGive(scheduling_mutex);
    return result;
}

esp_err_t scheduling_delete(const char *scheduling_id)
{
    esp_err_t result;

    if (!scheduling_has_id(scheduling_id))
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (scheduling_mutex == NULL ||
        xSemaphoreTake(scheduling_mutex, portMAX_DELAY) != pdTRUE)
    {
        return ESP_ERR_INVALID_STATE;
    }

    result = data_app_delete_scheduling((char *)scheduling_id);
    if (result == ESP_OK)
    {
        data_app_load(DATA_TYPE_SCHEDULING_DATE, scheduling_dates);
        data_app_load(DATA_TYPE_SCHEDULING_OFF_DATE, scheduling_off_dates);
    }

    xSemaphoreGive(scheduling_mutex);
    return result;
}

size_t scheduling_get_dates(pump_scheduling_date *schedules_out, size_t schedules_count)
{
    size_t copy_count = (schedules_count < CONFIG_SCHEDULING_MAX_VALUE)
        ? schedules_count
        : CONFIG_SCHEDULING_MAX_VALUE;

    if (schedules_out == NULL || scheduling_mutex == NULL)
    {
        return 0;
    }

    if (xSemaphoreTake(scheduling_mutex, portMAX_DELAY) != pdTRUE)
    {
        return 0;
    }

    memcpy(schedules_out, scheduling_dates, copy_count * sizeof(*schedules_out));
    xSemaphoreGive(scheduling_mutex);
    return copy_count;
}

size_t scheduling_get_off_dates(pump_scheduling_off_date *schedules_out, size_t schedules_count)
{
    size_t copy_count = (schedules_count < CONFIG_SCHEDULING_MAX_VALUE)
        ? schedules_count
        : CONFIG_SCHEDULING_MAX_VALUE;

    if (schedules_out == NULL || scheduling_mutex == NULL)
    {
        return 0;
    }

    if (xSemaphoreTake(scheduling_mutex, portMAX_DELAY) != pdTRUE)
    {
        return 0;
    }

    memcpy(schedules_out, scheduling_off_dates, copy_count * sizeof(*schedules_out));
    xSemaphoreGive(scheduling_mutex);
    return copy_count;
}
