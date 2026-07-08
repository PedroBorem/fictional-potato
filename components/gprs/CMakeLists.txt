idf_component_register(	SRCS
							"gprs_uart.c"
						INCLUDE_DIRS 
                   			"include"
						REQUIRES
							utils
							esp_driver_uart
							esp_driver_gpio )
                   		
