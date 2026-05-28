idf_component_register(	SRCS 
							"main.c"
							"rush_mode.c"
							"system_manager.c"
							"system_monitoring.c"
							"sectorization.c"
							"scheduling.c"
							"data_preprocessor.c"
                    	INCLUDE_DIRS
                    		"include"
                    	PRIV_INCLUDE_DIRS
                    		"../components/embedded_model/include")
