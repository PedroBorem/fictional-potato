idf_component_register(SRCS
                           "rtc_i2c_dev.c"
                           "rtc_ds3231.c"
                       INCLUDE_DIRS
                           "include"
                       REQUIRES esp_driver_i2c
                       PRIV_REQUIRES main utils)
