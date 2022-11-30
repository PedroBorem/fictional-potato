idf_component_register(	SRCS
							"i2cdev.c"
							"rtc_ds3231.c"
							"rtc_app.c"
						INCLUDE_DIRS 
                   			"include"
                   		PRIV_REQUIRES main utils )
                   		
