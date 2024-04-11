/**
 * @file system_manager.h
 * @brief Header file for system manager module.
 * @author Bruno
 * @date 17 de jul. de 2023
 */

#ifndef MAIN_INCLUDE_SYSTEM_MANAGER_H_
#define MAIN_INCLUDE_SYSTEM_MANAGER_H_

/**
 * @brief Initializes the system manager.
 *
 * This function initializes the system manager module.
 */
void system_manager_init(void);

uint16_t get_start_angle_barrier_status();
uint16_t get_end_angle_barrier_status();

#endif /* MAIN_INCLUDE_SYSTEM_MANAGER_H_ */
