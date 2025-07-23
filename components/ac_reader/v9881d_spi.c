/**
 * @file v9881d_spi.c
 * @brief SPI control for V9881D (Master com CS manual)
 */

#include "v9881d_spi.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

/* ---------------- Private definitions ---------------- */
#define SPI_TASK_NAME "spi_read_task"
#define SPI_TASK_STACK_SIZE 4096
#define SPI_TASK_PRIORITY 5
#define TAG "V9881D_SPI"

#define SPI_HOST_USED SPI2_HOST

#define PIN_NUM_CLK 39
#define PIN_NUM_MISO 40
#define PIN_NUM_MOSI 41
#define PIN_NUM_CS 42

/* ---------------- Private variables ------------------ */
static spi_device_handle_t spi;
static app_callback spi_callback = NULL;

/* Buffers DMA-alinhados */
WORD_ALIGNED_ATTR static uint8_t tx_buffer[3];
WORD_ALIGNED_ATTR static uint8_t rx_buffer[3];

/* ---------------- Private prototypes ----------------- */
static void spi_read_task(void *arg);
static uint16_t read_register(uint8_t reg_addr);

/* ---------------- Public methods --------------------- */
esp_err_t spi_init(const app_callback callback)
{
    esp_err_t ret = ESP_FAIL;

    // Configuração do barramento SPI
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64};

    // Inicializa o barramento SPI
    ret = spi_bus_initialize(SPI_HOST_USED, &buscfg, SPI_DMA_CH_AUTO);
    if (ret == ESP_OK)
    {
        // Configuração do dispositivo SPI com CS manual
        spi_device_interface_config_t devcfg = {
            .clock_speed_hz = 1 * 1000 * 1000,
            .mode = 0,
            .spics_io_num = -1, // CS será controlado manualmente
            .queue_size = 1,
        };

        // Adiciona o dispositivo SPI
        ret = spi_bus_add_device(SPI_HOST_USED, &devcfg, &spi);
        if (ret == ESP_OK)
        {
            // Configura o pino CS como GPIO manual
            gpio_reset_pin(PIN_NUM_CS);
            gpio_set_direction(PIN_NUM_CS, GPIO_MODE_OUTPUT);
            gpio_set_level(PIN_NUM_CS, 1); // Mantém CS inativo

            // Cria a task de leitura periódica
            BaseType_t xReturn = xTaskCreate(spi_read_task,
                                             SPI_TASK_NAME,
                                             SPI_TASK_STACK_SIZE,
                                             NULL,
                                             SPI_TASK_PRIORITY,
                                             NULL);

            if (callback != NULL && xReturn == pdPASS)
            {
                spi_callback = callback;
            }
            else if (xReturn != pdPASS)
            {
                ESP_LOGE(TAG, "%s, failed to create task", __func__);
                ret = ESP_ERR_INVALID_ARG;
            }
        }
        else
        {
            ESP_LOGE(TAG, "%s, failed to add SPI device", __func__);
        }
    }
    else
    {
        ESP_LOGE(TAG, "%s, failed to initialize SPI bus", __func__);
    }

    return ret;
}

/* ---------------- Private methods -------------------- */

/**
 * @brief Lê um registrador do Slave (M90E26 emulado)
 *
 * @param reg_addr Endereço do registrador (7 bits)
 * @return Valor de 16 bits lido
 */
static uint16_t read_register(uint8_t reg_addr)
{
    // 1ª transação: envia comando (prepara valor no slave)
    tx_buffer[0] = 0x80 | (reg_addr & 0x7F);
    tx_buffer[1] = 0x00;
    tx_buffer[2] = 0x00;

    spi_transaction_t trans = {};
    trans.length = 32;
    trans.rxlength = 32;
    trans.tx_buffer = tx_buffer;
    trans.rx_buffer = rx_buffer;

    gpio_set_level(PIN_NUM_CS, 0);
    spi_device_transmit(spi, &trans);
    gpio_set_level(PIN_NUM_CS, 1);

    // Delay mínimo
    vTaskDelay(1);

    // Segunda transação: recebe dados válidos
    memset(rx_buffer, 0, sizeof(rx_buffer));
    tx_buffer[0] = 0x00;
    tx_buffer[1] = 0x00;
    tx_buffer[2] = 0x00;

    gpio_set_level(PIN_NUM_CS, 0);
    spi_device_transmit(spi, &trans);
    gpio_set_level(PIN_NUM_CS, 1);

    // Pega os dois últimos bytes (índice 2 e 3)
    uint16_t value = ((uint16_t)rx_buffer[1] << 8) | rx_buffer[2];

    value <<= 1;
    value /= 2; // Corrige escala

    ESP_LOGI(TAG, "RX BYTES: %02X %02X %02X %02X (val=%u)",
             rx_buffer[0], rx_buffer[1], rx_buffer[2], rx_buffer[3], value);

    return value;
}

/**
 * @brief Task periódica para leitura dos registradores
 */
static void spi_read_task(void *arg)
{
    const uint8_t REG_URMS = 0x49;
    const uint8_t REG_IRMS = 0x48;
    const uint8_t REG_PMEAN = 0x4A;

    while (1)
    {
        uint16_t urms = read_register(REG_URMS);
        ESP_LOGI(TAG, "Urms = %.2f V", urms / 100.0f); // Mantém escala /100 para URMS (22000 -> 220.00 V)

        uint16_t irms = read_register(REG_IRMS);
        ESP_LOGI(TAG, "Irms = %.3f A", irms / 1000.0f); // Mantém escala /1000 para IRMS (5000 -> 5.000 A)

        uint16_t pmean = read_register(REG_PMEAN);
        float watts = pmean / 1000.0f; // Mantém escala /1000 para PMEAM (1100 -> 1.1 kW)
        ESP_LOGI(TAG, "Pmean = %.3f kW", watts);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
