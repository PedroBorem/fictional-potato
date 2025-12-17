#include "project_config.h"


void hw_test_init(const app_callback callback);

/* Chamado pelo comm_app_send_idp_pack quando communication==COMM_TEST */
void hw_test_on_tx_packet(const char *idp_pack);

bool hw_test_start_suite(void);