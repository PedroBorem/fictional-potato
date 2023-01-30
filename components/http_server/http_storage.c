/*
 * http_storage.c
 *
 *  Created on: 20 de ago de 2022
 *      Author: bruno
 */

/* Self include */
#include "http_storage.h"

/* storage include */
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "soc/soc_caps.h"
#include "esp_netif.h"
#include "nvs_flash.h"

/* Private definitions ------------------------------------------- */
#define HTTP_STORAGE_TAG "http_storage"

/* Public methods ------------------------------------------------ */
esp_err_t http_server_mount_storage(const char* base_path)
{
	size_t total = 0;
	size_t used = 0;

    ESP_LOGI(HTTP_STORAGE_TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = base_path,
        .partition_label = NULL,
        .max_files = 5,   // This sets the maximum number of files that can be open at the same time
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(HTTP_STORAGE_TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(HTTP_STORAGE_TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(HTTP_STORAGE_TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(HTTP_STORAGE_TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(HTTP_STORAGE_TAG, "Partition size: total: %d, used: %d", total, used);
    return ESP_OK;
}
