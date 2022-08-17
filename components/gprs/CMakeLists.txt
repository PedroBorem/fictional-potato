idf_component_register(	SRCS
							"gprs_uart.c"
							"gprs_module.c"
						INCLUDE_DIRS 
                   			"include"
                   		PRIV_REQUIRES main utils common_communication )
                   		
