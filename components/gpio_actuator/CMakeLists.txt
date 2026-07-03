idf_component_register(	SRCS
							"gpio_actuator.c"
						INCLUDE_DIRS 
                   			"include"
                   		REQUIRES
							utils
							esp_driver_gpio )
                   		
