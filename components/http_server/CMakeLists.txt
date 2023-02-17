idf_component_register(	SRCS
							"http_api.c"
							"http_config_parser.c"
							"http_storage.c"
						INCLUDE_DIRS 
							"include"
                    	EMBED_FILES 
                    		"web_page_files/favicon.ico"
                    		"web_page_files/logo.png" 
                    		"web_page_files/index.html"
                    		"web_page_files/index.css"
                    	PRIV_REQUIRES 
                    		main 
                    		utils
)
