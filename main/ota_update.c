/**
 * @file ota_update.c
 * @brief Implementation of task management functions for OTA updates.
 *
 * This file contains the implementation of functions to safely suspend and
 * resume FreeRTOS tasks during OTA updates, ensuring the main task remains active.
 */

#include "ota_update.h"
#include "project_config.h"
#include "FreeRTOS_defines.h"
#include "log.h"
#include <string.h>

#define TASK_LIST_BUFFER_SIZE 1024

void print_all_tasks(void)
{
    char buffer[TASK_LIST_BUFFER_SIZE];

    // Gera a lista de tasks
    vTaskList(buffer);

    // Exibe cabeçalho explicativo
    ESP_LOGI("task_list", "Name          State   Prio    Stack   Num");
    ESP_LOGI("task_list", "---------------------------------------------");

    // Imprime o conteúdo gerado pela vTaskList
    ESP_LOGI("task_list", "\n%s", buffer);
}

/**
 * @brief Suspends all FreeRTOS tasks except the one named "main".
 *
 * This function retrieves the status of all tasks in the system and suspends each one,
 * except the task whose name is exactly "main".
 *
 * @note The task you intend to keep running must be explicitly named "main"
 *       when created using xTaskCreate().
 *       Also, make sure configUSE_TRACE_FACILITY is enabled in FreeRTOSConfig.h.
 *       If more than max_tasks tasks are active, some tasks may not be suspended.
 */
void suspend_all_tasks_except_main(void)
{
    const UBaseType_t max_tasks = 20;
    TaskStatus_t task_status[max_tasks];
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();

    UBaseType_t task_count = uxTaskGetSystemState(task_status, max_tasks, NULL);

    for (UBaseType_t i = 0; i < task_count; i++)
    {
        if (task_status[i].xHandle != current_task)
        {
            // Log warning before suspending the task
            ESP_LOGW("task_manager", "Suspending task: %s", task_status[i].pcTaskName);
            vTaskSuspend(task_status[i].xHandle);
        }
    }
}

/**
 * @brief Resumes all FreeRTOS tasks except the main task.
 *
 * This function iterates through all suspended tasks and resumes them,
 * ensuring that the main task remains unaffected.
 *
 * @note The main task is identified by its name "main".
 */
void resume_all_tasks_except_main(void)
{
    const UBaseType_t max_tasks = 10; 
    TaskStatus_t task_status[max_tasks];
    UBaseType_t task_count = uxTaskGetSystemState(task_status, max_tasks, NULL);

    for (UBaseType_t i = 0; i < task_count; i++)
    {
        if (strcmp(task_status[i].pcTaskName, "main") != 0)
        {
            vTaskResume(task_status[i].xHandle); 
        }
    }
}
