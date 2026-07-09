/**
 * @file scheduling.h
 * @brief Date-based pump scheduling.
 */

#ifndef MAIN_INCLUDE_SCHEDULING_H_
#define MAIN_INCLUDE_SCHEDULING_H_

#include "esp_err.h"
#include "project_config.h"

typedef void (*scheduling_event_callback)(idp_type scheduling_idp,
                                          const char *scheduling_id,
                                          const char *event,
                                          const char *user);

esp_err_t scheduling_init(scheduling_event_callback callback);

esp_err_t scheduling_add_date(time_t start_date,
                              time_t end_date,
                              const char *user,
                              char *scheduling_id_out,
                              size_t scheduling_id_out_size);

esp_err_t scheduling_add_off_date(time_t end_date,
                                  const char *user,
                                  char *scheduling_id_out,
                                  size_t scheduling_id_out_size);

esp_err_t scheduling_delete(const char *scheduling_id);

size_t scheduling_get_dates(pump_scheduling_date *schedules_out, size_t schedules_count);
size_t scheduling_get_off_dates(pump_scheduling_off_date *schedules_out, size_t schedules_count);

#endif /* MAIN_INCLUDE_SCHEDULING_H_ */
