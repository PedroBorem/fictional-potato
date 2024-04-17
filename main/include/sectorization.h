/**
 * @file sectorization.h
 * @brief Header file for sectorization functionality.
 * @author soil-dev
 * @date 31 de jul. de 2023
 */

#ifndef MAIN_INCLUDE_SECTORIZATION_H_
#define MAIN_INCLUDE_SECTORIZATION_H_

#include "project_config.h"

/**
 * @brief Starts the sectorization task with the provided sector configuration.
 *
 * This function initializes the sectorization task with the specified sector configuration.
 *
 * @param sectors Configuration for sectorization.
 */
void sectorization_start(sector_config sectors);

/**
 * @brief Stops the sectorization task.
 *
 * This function stops the sectorization task.
 */
void sectorization_stop(void);

#endif /* MAIN_INCLUDE_SECTORIZATION_H_ */
