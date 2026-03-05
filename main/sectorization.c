/**
 * @file sectorization.c
 * @brief Implementation file for sectorization functionality.
 * @author soil-dev
 * @date 31 de jul. de 2023
 */

#include "sectorization.h"
#include "FreeRTOS_defines.h"
#include "log.h"
#include "actuation_app.h"
#include <string.h>

#define SECTORIZATION_TAG	"sectorization"

static TaskHandle_t xTask_sectorization = NULL; ///< Task handle for the sectorization task.
static sector_config sectorization_config = {}; ///< Configuration for sectorization.
static uint16_t* sectorization_current_angle = &global_angle; ///< Pointer to the current angle variable.

/**
 * @brief Sectorization task.
 *
 * This task monitors the current angle and activates the pump if the angle falls within specified sectors.
 *
/**
 * @brief TODO: Describe `argument`.
 *
 * @param arg TODO.
 * @return TODO.
 */
 * @param arg Task argument (not used).
 */
static void sectorization_task(void* arg)
{
	bool pump_is_on = false;
	uint8_t pump_flag = 0;

	while(1)
	{
		if(sectorization_config.sector_number > 0)
		{
			for(uint8_t angles = 0; angles < CONFIG_SECTORS_MAX_VALUE; angles++)
			{
				if(*sectorization_current_angle  >= sectorization_config.sectors[angles].start_angle
				&& *sectorization_current_angle  <= sectorization_config.sectors[angles].end_angle)
				{
					if(sectorization_config.sectors[angles].start_angle != 0
					&& sectorization_config.sectors[angles].end_angle != 0)
					{
						if(pump_is_on == false)
						{
							ESP_LOGI(SECTORIZATION_TAG,"Pump ON (%d)", *sectorization_current_angle );
							actuation_app_set_pump(true);
							pump_is_on = true;
						}
					}
				}
				else
				{
					pump_flag++;
				}
			}

			if(pump_flag == CONFIG_SECTORS_MAX_VALUE && pump_is_on == true)
			{
				ESP_LOGI(SECTORIZATION_TAG,"Pump OFF");
				actuation_app_set_pump(false);
				pump_is_on = false;
			}

			pump_flag = 0;
		}

		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

/**
 * @brief Starts the sectorization task with the provided sector configuration.
 *
 * This function initializes the sectorization task with the specified sector configuration.
 *
 * @param sectors Configuration for sectorization.
 */
void sectorization_start(sector_config sectors)
{
	if(sectors.sector_number > 0)
	{
		memcpy(&sectorization_config, &sectors, sizeof(sectorization_config));

		xTaskCreate(&sectorization_task,
					SECTORIZATION_TASK_NAME,
					SECTORIZATION_TASK_SIZE,
					NULL,
					SECTORIZATION_TASK_PRIORITY,
					&xTask_sectorization);
	}
}

/**
 * @brief Stops the sectorization task.
 *
 * This function stops the sectorization task.
 */
void sectorization_stop(void)
{
	if(xTask_sectorization != NULL)
	{
		vTaskDelete(xTask_sectorization);
		xTask_sectorization = NULL;
	}
}

