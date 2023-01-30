/*
 * crc32.c
 *
 *  Created on: 29 de jan de 2023
 *      Author: brunolima
 */
/* Includes --------------------------------------- */
#include "crc32.h"

/* Private definitions ---------------------------- */

/* Private types ---------------------------------- */

/* Private variables ------------------------------ */

/* Private functions prototypes --------------------- */
static uint32_t crc32_get_byte_value(uint8_t byte, uint32_t polynom);

/* Private functions -------------------------------- */
/**
 * @brief Computa o valor do CRC correspondente ao byte informado.
 *
 * O valor retornado por essa função é correspondente à lookup table
 * utilizada pelos cálculos do CRC32 padrão.
 * 
 * @param byte      Valor a ser computado.
 * @param polynom   Polinômio de cálculo.
 *
 * @return Valor calculado.
 */
static uint32_t crc32_get_byte_value(uint8_t byte, uint32_t polynom)
{
    uint32_t crc = byte;
    uint8_t i = 0;

    for (i = 0; i < 8; i++) {
        if ((crc & 0x00000001) == 0) {
            crc >>= 1;
        } else {
            crc >>= 1;
            crc ^= polynom;
        }
    }

    return crc;
}

/* Public functions --------------------------------- */
/**
 * @brief Inicializa a look up table de cálculo do CRC.
 * 
 * @param crc_table     Ponteiro para a tabela.
 * @param polynom       Polinômio usado para cálculo.
 */
void crc32_init_table(uint32_t *crc_table, uint32_t polynom)
{
    uint16_t i = 0;

    for (i = 0; i <= UINT8_MAX; i++) {
        crc_table[i] = crc32_get_byte_value(i, polynom);
    }
}

/**
 * @brief Calcula o CRC32 dos dados do buffer informado.
 *
 * @note O valor inicial é invertido pela função. Para calcular o CRC seguindo
 *       o algoritmo padrão do CRC32, ele deve ser igual a 0.
 * 
 * @param crc_init  Valor inicial.
 * @param crc_table Look up table.
 * @param data      Buffer de dados.
 * @param data_len  Tamanho do buffer em bytes.
 *
 * @return Valor do CRC32.
 */
uint32_t crc32_calculate(uint32_t crc_init, uint32_t *crc_table, void *data, uint32_t data_len)
{
    uint32_t crc = ~crc_init;
    uint32_t i = 0;

    for (i = 0; i < data_len; i++) {
        uint32_t table_val = crc_table[(crc & 0xFF) ^ ((uint8_t*)data)[i]];
        crc = (crc >> 8) ^ table_val;
    }

    return ~crc;
}
