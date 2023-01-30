/*
 * crc32.h
 *
 *  Created on: 29 de jan de 2023
 *      Author: brunolima
 */
#ifndef _CRC32_H_
#define _CRC32_H_

/* Includes --------------------------------------- */
#include <stdint.h>

/* Public definitions ------------------------------- */
/**
 * @brief Polinômio padrão de cálculo do CRC32.
 *
 * Esse valor é o inverso de 0x04C11DB7.
 */
#define MSF_CRC_POLYNOM   0xEDB88320

#define MSF_CRC_INIT      0x59DFB012

/* Public types ------------------------------------- */

/* Public functions --------------------------------- */
void crc32_init_table(uint32_t *crc_table, uint32_t polynom);
uint32_t crc32_calculate(uint32_t crc_init, uint32_t *crc_table, void *data, uint32_t data_len);

#endif /* _CRC32_H_ */
