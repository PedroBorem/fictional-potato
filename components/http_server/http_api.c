/*
 * http_server.c
 *
 *  Created on: 20 de jan de 2023
 *      Author: bruno
 */

/* Self include */
#include "http_api.h"

/* HTTP include */
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "log.h"

#ifndef MIN
# define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

/* Private definitions ------------------------------------------- */
/**
 * Temporary storage during file transfer (size)
 *
 */
#define HTTP_SCRATCH_BUFSIZE  		8192

/**
 * HTTP api log tag
 *
 */
#define HTTP_API_TAG  			"http_api"

/* Private variables  -------------------------------------------- */
/**
 * data structure for HTTP transmission
 *
 */
typedef struct
{
    /* Base path of file storage */
    char base_path[ESP_VFS_PATH_MAX + 1];

    /* Scratch buffer for temporary storage during file transfer */
    char scratch[HTTP_SCRATCH_BUFSIZE];
}http_file_server_data; //todo analisar se podemos remover isso

/*
 * Structure holding server handle
 * and internal socket fd in order
 * to use out of request send
 */
struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
};

// callback pointer
static app_callback http_callback = NULL;

static httpd_handle_t server = NULL;

/* Private function prototype ------------------------------------ */
static esp_err_t http_get_handler(httpd_req_t *req);
static esp_err_t http_post_handler(httpd_req_t *req);
static esp_err_t http_delete_handler(httpd_req_t *req);

static http_file_server_data* server_data = NULL;
static httpd_req_t* server_req = NULL;

/* Public methods ------------------------------------------------ */
esp_err_t http_server_init(void)
{
    esp_err_t err = ESP_OK;

    /* Allocate memory for server data */
    server_data = calloc(1, sizeof(http_file_server_data));
    if (server_data == NULL)
    {
        ESP_LOGE(HTTP_API_TAG, "Failed to allocate memory for server data");
        err = ESP_ERR_NO_MEM;
    }

    /* Start the server for the first time */
    server = http_server_start();

    return err;
}

httpd_handle_t http_server_start(void)
{
	const char* base_path = "/data";
    httpd_handle_t http_handle = NULL;

	if (server_data == NULL)
	{
		ESP_LOGE(HTTP_API_TAG, "Failed to allocate memory for server data");
	}
	else
	{
		strlcpy(server_data->base_path, base_path, sizeof(server_data->base_path));
		httpd_config_t config = HTTPD_DEFAULT_CONFIG();
		config.stack_size = (4096 * 5);
		config.max_uri_handlers = 11;

		/* Use the URI wildcard matching function in order to
		 * allow the same handler to respond to multiple different
		 * target URIs which match the wildcard scheme */
		config.uri_match_fn = httpd_uri_match_wildcard;

		if (httpd_start(&http_handle, &config) != ESP_OK)
		{
			ESP_LOGE(HTTP_API_TAG, "Failed to start file server!");
		}
		else
		{
			/* URI handler for get api-status */
			httpd_uri_t get_api = {
				.uri       = "/*",
				.method    = HTTP_GET,
				.handler   = http_get_handler,
				.user_ctx  = server_data,
			};
			httpd_register_uri_handler(http_handle, &get_api);

			/* URI handler for uploading files to server */
			httpd_uri_t post_api = {
				.uri       = "/*",
				.method    = HTTP_POST,
				.handler   = http_post_handler,
				.user_ctx  = server_data
			};
			httpd_register_uri_handler(http_handle, &post_api);


			/* URI handler for uploading files to server */
			httpd_uri_t delete_api = {
				.uri       = "/*",
				.method    = HTTP_DELETE,
				.handler   = http_delete_handler,
				.user_ctx  = server_data
			};
			httpd_register_uri_handler(http_handle, &delete_api);

			LOG_COMM(HTTP_API_TAG, "HTTP server started on port: '%d'", config.server_port);
		}
	}

    return http_handle;
}

esp_err_t http_server_stop(httpd_handle_t http_handle)
{
	esp_err_t ret = ESP_ERR_INVALID_STATE;

	if(http_handle != NULL)
	{
		ret = httpd_stop(http_handle);
		if(ret == ESP_OK)
		{
			ESP_LOGW(HTTP_API_TAG, "HTTP server stopped");
			http_handle = NULL;
		}
	}
	else
	{
		ESP_LOGE(HTTP_API_TAG, "HTTP server not started");
	}

    return ret;
}

esp_err_t http_server_register_callback(app_callback callback)
{
	esp_err_t ret = ESP_FAIL;

	if(callback != NULL)
	{
		http_callback = callback;
		ret = ESP_OK;
	}

	return ret;
}

void http_server_send_resp(char* package)
{
	if(server_req != NULL)
	{
		httpd_resp_send(server_req, package, HTTPD_RESP_USE_STRLEN);
	}

	server_req = NULL;
}

/* Private methods ----------------------------------------------- */
/**
 * @brief	Handler to download a file kept on the server
 * @param
 *  - req[in/out] - HTTP Request Data Structure.
 * @return
 *  - ESP_OK : On success
 *  - ESP_FAIL : fail to get handler
 */
static esp_err_t http_get_handler(httpd_req_t *req)
{
	esp_err_t err = ESP_OK;

	server_req = req;

    if(strcmp(req->uri, "/api-status") == 0)
    {
    	LOG_COMM(HTTP_API_TAG, "get /api-status : %s", "{status_soil:200}");
		httpd_resp_send(req, "{status_soil:200}", HTTPD_RESP_USE_STRLEN);
		server_req = NULL;
    }
    else if (strcmp(req->uri, "/states") == 0)
	{
		if(http_callback != NULL)
		{
			http_callback("#00$", COMM_HTTP_GET);
		}
		else
		{
			ESP_LOGE(HTTP_API_TAG,"unregistered HTTP callback");
			err = ESP_FAIL;
		}
	}
    else if(strcmp(req->uri, "/config/network") == 0)
	{
		if(http_callback != NULL)
		{
			http_callback("#02$", COMM_HTTP_GET);
		}
		else
		{
			ESP_LOGE(HTTP_API_TAG,"unregistered HTTP callback");
			err = ESP_FAIL;
		}
	}
    else if(strcmp(req->uri, "/config/pivot") == 0)
	{
		if(http_callback != NULL)
		{
			http_callback("#03$", COMM_HTTP_GET);
		}
		else
		{
			ESP_LOGE(HTTP_API_TAG,"unregistered HTTP callback");
			err = ESP_FAIL;
		}
	}
    else if(strcmp(req->uri, "/config/eco") == 0)
	{
		if(http_callback != NULL)
		{
			http_callback("#04$", COMM_HTTP_GET);
		}
		else
		{
			ESP_LOGE(HTTP_API_TAG,"unregistered HTTP callback");
			err = ESP_FAIL;
		}
	}
	else if(strcmp(req->uri, "/config/sector") == 0)
	{
		if(http_callback != NULL)
		{
			http_callback("#05$", COMM_HTTP_GET);
		}
		else
		{
			ESP_LOGE(HTTP_API_TAG,"unregistered HTTP callback");
			err = ESP_FAIL;
		}
	}
	else if (strcmp(req->uri, "/cycles") == 0)
	{

		if(http_callback != NULL)
		{
			http_callback("#12$", COMM_HTTP_GET);
		}
		else
		{
			ESP_LOGE(HTTP_API_TAG,"unregistered HTTP callback");
			err = ESP_FAIL;
		}
	}

    else if (strcmp(req->uri, "/scheduling/date") == 0)
   	{
    	if(http_callback != NULL)
		{
    		http_callback("#14$", COMM_HTTP_GET);
		}
		else
		{
			ESP_LOGE(HTTP_API_TAG,"unregistered HTTP callback");
			err = ESP_FAIL;
		}
   	}
    else if (strcmp(req->uri, "/scheduling/angle") == 0)
	{
    	if(http_callback != NULL)
		{
    		http_callback("#15$", COMM_HTTP_GET);
		}
		else
		{
			ESP_LOGE(HTTP_API_TAG,"unregistered HTTP callback");
			err = ESP_FAIL;
		}
	}
    else if (strcmp(req->uri, "/scheduling/off") == 0)
	{
		if(http_callback != NULL)
		{
			http_callback("#16$", COMM_HTTP_GET);
		}
		else
		{
			ESP_LOGE(HTTP_API_TAG,"unregistered HTTP callback");
			err = ESP_FAIL;
		}
	}
    else
    {
    	ESP_LOGE(HTTP_API_TAG,"invalid HTTP uri (%s)", req->uri);
    	httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid HTTP uri");
		server_req = NULL;

		err = ESP_FAIL;
    }

    return err;
}

/**
 * @brief	Handler to  a file from the server
 * @param
 *  - req[in/out] - HTTP Request Data Structure.
 * @return
 *  - ESP_OK : On success
 *  - ESP_FAIL : fail to get submit
 */
static esp_err_t http_post_handler(httpd_req_t *req)
{
	esp_err_t err = ESP_FAIL;

	char content[1000] = {};

    /* Truncate if content length larger than the buffer */
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) // 0 return value indicates connection closed
    {
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
    }
    else
    {
		LOG_COMM(HTTP_API_TAG, "content_len %d", req->content_len);
		LOG_COMM(HTTP_API_TAG, "URI %s", req->uri);
		LOG_COMM(HTTP_API_TAG, "content %s", content);

		server_req = req;
		http_callback(content, COMM_HTTP_POST);
    }

	return err;
}

static esp_err_t http_delete_handler(httpd_req_t *req)
{
	esp_err_t err = ESP_FAIL;

	char content[100] = {};

	/* Truncate if content length larger than the buffer */
	size_t recv_size = MIN(req->content_len, sizeof(content));

	int ret = httpd_req_recv(req, content, recv_size);
	if (ret <= 0) // 0 return value indicates connection closed
	{
		/* Check if timeout occurred */
		if (ret == HTTPD_SOCK_ERR_TIMEOUT)
		{
			httpd_resp_send_408(req);
		}
	}
	else
	{
		LOG_COMM(HTTP_API_TAG, "content_len %d", req->content_len);
		LOG_COMM(HTTP_API_TAG, "URI %s", req->uri);
		LOG_COMM(HTTP_API_TAG, "content %s", content);

		server_req = req;
		http_callback(content, COMM_HTTP_POST);
	}

	return err;
}
