idf_component_register(	SRCS
							"rtc_i2c_dev.c"
							"rtc_ds3231.c"
							"rtc_app.c"
						INCLUDE_DIRS 
                   			"include"
                   		PRIV_REQUIRES main utils )