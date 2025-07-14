/**
 * @file v9881d_spi.c
 * @brief SPI control for V9881D
 */

/* Self include */
#include "v9881d_spi.h"

#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

/* Private definitions ------------------------------------------- */
/**
 * @def SPI_TASK_NAME
 * @brief Name of the SPI read task
 */
#define SPI_TASK_NAME "spi_read_task"

/**
 * @def SPI_TASK_STACK_SIZE
 * @brief Stack size for the SPI read task
 */
#define SPI_TASK_STACK_SIZE 2048

/**
 * @def SPI_TASK_PRIORITY
 * @brief Priority for the SPI read task
 */
#define SPI_TASK_PRIORITY 5

/**
 * @def TAG
 * @brief Tag used for logging in the SPI module
 */
#define TAG "V9881D_SPI"

/**
 * @def SPI_HOST_USED
 * @brief SPI host used for communication
 */
#define SPI_HOST_USED SPI2_HOST

/**
 * @def PIN_NUM_CLK
 * @brief GPIO number for SPI clock
 */
#define PIN_NUM_CLK 39

/**
 * @def PIN_NUM_MISO
 * @brief GPIO number for SPI MISO
 */
#define PIN_NUM_MISO 40

/**
 * @def PIN_NUM_MOSI
 * @brief GPIO number for SPI MOSI
 */
#define PIN_NUM_MOSI 41

/**
 * @def PIN_NUM_CS
 * @brief GPIO number for SPI chip select
 */
#define PIN_NUM_CS 42

/* Private variables -------------------------------------------- */
/**
 * @brief SPI device handle
 */
static spi_device_handle_t spi;

/**
 * @brief Callback function for SPI events
 */
static app_callback spi_callback = NULL;

/* Private function prototypes ----------------------------------- */
/**
 * @brief Task to handle SPI read operations
 * @param arg User-defined argument passed to the task
 */
static void spi_read_task(void* arg);

/* Public methods ------------------------------------------------ */
/**
 * @brief Initialize the SPI module.
 *
 * This function initializes the SPI module, setting up the SPI communication.
 *
 * @param callback The callback function to be called when data is received.
 *
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Fail to initialize
 */
esp_err_t spi_init(const app_callback callback)
{
    esp_err_t ret = ESP_FAIL;

    // Configure parameters of the SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64
    };

    ret = spi_bus_initialize(SPI_HOST_USED, &buscfg, SPI_DMA_CH_AUTO);
    if (ret == ESP_OK)
    {
        spi_device_interface_config_t devcfg = {
            .clock_speed_hz = 1 * 1000 * 1000,  
            .mode = 0,                        
            .spics_io_num = PIN_NUM_CS,        
            .queue_size = 1,
        };

        ret = spi_bus_add_device(SPI_HOST_USED, &devcfg, &spi);
        if (ret == ESP_OK)
        {
            // Create a task to handle SPI read operations
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
            else
            {
                ESP_LOGE(TAG, "%s, invalid argument", __func__);
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

/* Private methods ----------------------------------------------- */
/**
 * @brief SPI read task
 * @param arg[in] : task argument (default NULL)
 */
static void spi_read_task(void* arg)
{
    uint8_t tx_data[4] = {0xAA, 0xBB, 0x00, 0x00}; 
    uint8_t rx_data[4] = {0};

    while (1)
    {
        spi_transaction_t trans = {
            .length = 8 * sizeof(tx_data),
            .tx_buffer = tx_data,
            .rx_buffer = rx_data,
        };

        esp_err_t ret = spi_device_transmit(spi, &trans);
        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "SPI Response:");
            for (int i = 0; i < sizeof(rx_data); i++)
            {
                ESP_LOGI(TAG, "Byte %d: 0x%02X", i, rx_data[i]);
            }
        }
        else
        {
            ESP_LOGE(TAG, "SPI transaction error");
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // 1-second delay
    }
}
