/**
 * @file ota_update.h
 * @brief Header file for OTA update task management.
 *
 * This file contains declarations for functions that manage FreeRTOS tasks
 * during OTA updates, ensuring safe suspension and resumption of tasks.
 */

#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

// #include "FreeRTOS_defines.h"
#include <string.h>

/**
 * @brief Suspends all FreeRTOS tasks except the main task.
 *
 * This function iterates through all active tasks and suspends them,
 * ensuring that only the main task remains active.
 *
 * @note The main task is identified by its name "main".
 */
void suspend_all_tasks_except_main(void);

/**
 * @brief Resumes all FreeRTOS tasks except the main task.
 *
 * This function iterates through all suspended tasks and resumes them,
 * ensuring that the main task remains unaffected.
 *
 * @note The main task is identified by its name "main".
 */
void resume_all_tasks_except_main(void);

void print_all_tasks(void);

#endif // OTA_UPDATE_H