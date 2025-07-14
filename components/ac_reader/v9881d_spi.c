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

static uint16_t read_register(uint8_t reg_addr, uint8_t* tx_data, uint8_t* rx_data);

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

static uint16_t read_register(uint8_t reg_addr, uint8_t* tx_data, uint8_t* rx_data)
{
    tx_data[0] = 0x80 | (reg_addr & 0x7F);
    tx_data[1] = 0x00;
    tx_data[2] = 0x00;

    spi_transaction_t trans = {
        .length = 24,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };

    if (spi_device_transmit(spi, &trans) == ESP_OK)
    {
        return (rx_data[1] << 8) | rx_data[2];
    }
    else
    {
        ESP_LOGE(TAG, "Erro ao ler registrador 0x%02X", reg_addr);
        return 0xFFFF;
    }
}


/* Private methods ----------------------------------------------- */
static void spi_read_task(void* arg)
{
    const uint8_t REG_URMS = 0x49;
    const uint8_t REG_IRMS = 0x48;
    const uint8_t REG_PMEAN = 0x4A;

    uint8_t tx_data[3];
    uint8_t rx_data[3];

    while (1)
    {
        uint16_t urms = read_register(REG_URMS, tx_data, rx_data);
        ESP_LOGI(TAG, "Urms = %.2f V", urms / 100.0f);

        uint16_t irms = read_register(REG_IRMS, tx_data, rx_data);
        ESP_LOGI(TAG, "Irms = %.3f A", irms / 1000.0f);

        uint16_t pmean = read_register(REG_PMEAN, tx_data, rx_data);
        float watts = (int16_t)pmean / 1000.0f;
        ESP_LOGI(TAG, "Pmean = %.3f kW", watts);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
