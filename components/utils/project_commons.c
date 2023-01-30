/* Includes ----------------------------------------------------------------- */
// Standard
#include <stdbool.h>

// Self include
#include "project_commons.h"

// System
#include "esp_mac.h"
#include "esp_netif.h"

/* Public functions --------------------------------------------------------- */
/**
 * @brief Get the raw byte stream of the station MAC address of the device
 * 
 * @return uint_8t*     Statically allocated byte stream for MAC bytes
 * (do not free it)
 */
uint8_t *get_sta_mac_bytes(void)
{
    static uint8_t mac_addr_bytes[MAC_ADDRESS_LEN] = {0};
    static bool mac_already_known = false;

    // Initialization check
    if (!mac_already_known) {
        esp_read_mac(mac_addr_bytes, ESP_MAC_WIFI_STA);
        mac_already_known = true;
    }

    return mac_addr_bytes;
}

/**
 * @brief Get a string representing station MAC address
 * at format XX-XX-XX-XX-XX-XX
 * 
 * @return char*        Statically allocated string for MAC bytes
 * (do not free it)
 */
char *get_sta_mac_dash_str(void)
{
    static char mac_str_with_dashes[MAC_ADDRESS_LEN * 3] = {'\0'};
    static bool mac_already_known = false;

    // Initialization check
    if (!mac_already_known) {
        uint8_t *mac_addr_bytes = get_sta_mac_bytes();
        sprintf(mac_str_with_dashes, "%02X-%02X-%02X-%02X-%02X-%02X", MAC2STR(mac_addr_bytes));
        mac_already_known = true;
    }

    return mac_str_with_dashes;

}
