/**
 * @file main.c
 * @brief Main source file for the application.
 * @author brunolima
 * @date 31 de mai de 2022
 */

/* Applications include */
#include "FreeRTOS_defines.h"
#include "system_manager.h"

/**
 * @brief Main function of the application.
 *
 * This is the main entry point of the application. It initializes the system manager
 * and enters a loop with a delay of 2000 milliseconds.
 */
void app_main(void)
{
    /**
     * @note The application runs an infinite loop with a delay of 2000 milliseconds,
     * effectively keeping the system_manager running.
     */
    system_manager_init();
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
