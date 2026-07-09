idf_component_register(	SRCS 
							"main.c"
							"system_manager.c"
							"scheduling.c"
							"system_monitoring.c"
                    	INCLUDE_DIRS
                    		"include"
						REQUIRES
							applications
							utils
							idp_protocol)
