#ifndef _PROJECT_COMMONS_INCLUDE_H_
#define _PROJECT_COMMONS_INCLUDE_H_

/* Includes ----------------------------------------------------------------- */
#include <stdint.h>

/* Public definitions ------------------------------------------------------- */
#define PROJECT_VERSION_MAJOR   0
#define PROJECT_VERSION_MINOR   0
#define PROJECT_VERSION_PATCH   0
#define MAC_ADDRESS_LEN         6
/* Public functions --------------------------------------------------------- */
uint8_t *get_sta_mac_bytes(void);
char *get_sta_mac_dash_str(void);

#endif  /* _PROJECT_COMMONS_INCLUDE_H_ */
