#include <stdio.h>
#include "driver/spi_master.h"
#include "esp_log.h"

#define TAG "V9881D_SPI"

// Pinos SPI
#define PIN_NUM_MISO 40
#define PIN_NUM_MOSI 39
#define PIN_NUM_CLK  41
#define PIN_NUM_CS   42

#define SPI_HOST_USED SPI2_HOST   // Use SPI2 ou SPI3 no ESP32-S3

spi_device_handle_t spi;

void spi_init()
{
    esp_err_t ret;

    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64
    };

    ret = spi_bus_initialize(SPI_HOST_USED, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000,  
        .mode = 0,                        
        .spics_io_num = PIN_NUM_CS,        
        .queue_size = 1,
    };

    ret = spi_bus_add_device(SPI_HOST_USED, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
}

void spi_read()
{
    uint8_t tx_data[4] = {0xAA, 0xBB, 0x00, 0x00}; 
    uint8_t rx_data[4] = {0};

    spi_transaction_t trans = {
        .length = 8 * sizeof(tx_data),
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };

    esp_err_t ret = spi_device_transmit(spi, &trans);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Resposta SPI:");
        for (int i = 0; i < sizeof(rx_data); i++)
        {
            ESP_LOGI(TAG, "Byte %d: 0x%02X", i, rx_data[i]);
        }
    }
    else
    {
        ESP_LOGE(TAG, "Erro na transação SPI");
    }
}
