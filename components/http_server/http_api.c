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
#include "esp_http_server.h"
#include "http_storage.h"
#include "esp_vfs.h"

/* Private definitions ------------------------------------------- */
/**
 * Max length a file path can have on storage
 *
 */
#define HTTP_FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)

/**
 * Validate file
 *
 */
#define HTTP_IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

/**
 * Max size of an individual file. Make sure this
 * value is same as that set in upload_script.html
 *
 */
#define HTTP_MAX_FILE_SIZE   		(200*1024) // 200 KB

/**
 * Max size of an individual file (string)
 *
 */
#define HTTP_MAX_FILE_SIZE_STR 		"200KB"

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

/* Max length a file path can have on storage */
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)

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
}http_file_server_data;

// handle to the driver
static httpd_handle_t http_handle = NULL;

// callback pointer
static app_callback http_callback = NULL;

static char* http_config = NULL;

/* Private function prototype ------------------------------------ */
static esp_err_t http_index_html_get_handler(httpd_req_t *req);
static esp_err_t http_index_css_get_handler(httpd_req_t *req);
static esp_err_t http_favicon_get_handler(httpd_req_t *req);
static esp_err_t http_logo_get_handler(httpd_req_t *req);
static esp_err_t http_resp_dir_html(httpd_req_t *req, const char *dirpath);
static esp_err_t http_download_get_handler(httpd_req_t *req);
static esp_err_t http_submit_post_handler(httpd_req_t *req);

static http_file_server_data *server_data = NULL;

/* Copies the full path into destination buffer and returns
 * pointer to path (skipping the preceding base path) */
static const char* http_get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize)
{
    const size_t base_pathlen = strlen(base_path);

    const char *hash = strchr(uri, '#');
    const char *quest = strchr(uri, '?');
    size_t pathlen = strlen(uri);

    if (quest)
    {
        pathlen = MIN(pathlen, quest - uri);
    }

    if (hash)
    {
        pathlen = MIN(pathlen, hash - uri);
    }

    if (base_pathlen + pathlen + 1 > destsize)
    {
        /* Full path string won't fit into destination buffer */
        return NULL;
    }

    /* Construct full path (base + path) */
    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);

    /* Return pointer to path, skipping the base */
    return dest + base_pathlen;
}

/* Public methods ------------------------------------------------ */
esp_err_t http_server_init(void)
{
    esp_err_t err = ESP_OK;
    const char* base_path = "/data";

    err = http_server_mount_storage(base_path);
    if(err != ESP_OK)
    {
        return err;
    }

    /* Allocate memory for server data */
    server_data = calloc(1, sizeof(http_file_server_data));
    if (!server_data)
    {
        ESP_LOGE(HTTP_API_TAG, "Failed to allocate memory for server data");
        err = ESP_ERR_NO_MEM;
    }

    return err;
}

esp_err_t http_server_start(void)
{
	esp_err_t err = ESP_OK;
	const char* base_path = "/data";

    if (http_handle != NULL)
    {
        ESP_LOGE(HTTP_API_TAG, "File server already started");
        err = ESP_ERR_INVALID_STATE;
    }
    else
    {
		if (!server_data)
		{
			ESP_LOGE(HTTP_API_TAG, "Failed to allocate memory for server data");
			err = ESP_ERR_NO_MEM;
		}
		else
		{
			strlcpy(server_data->base_path, base_path, sizeof(server_data->base_path));
			httpd_config_t config = HTTPD_DEFAULT_CONFIG();
			config.stack_size = (4096 * 5);

			/* Use the URI wildcard matching function in order to
			 * allow the same handler to respond to multiple different
			 * target URIs which match the wildcard scheme */
			config.uri_match_fn = httpd_uri_match_wildcard;

			if (httpd_start(&http_handle, &config) != ESP_OK)
			{
				ESP_LOGE(HTTP_API_TAG, "Failed to start file server!");
				err = ESP_FAIL;
			}
			else
			{
				/* URI handler for getting uploaded files */
				httpd_uri_t file_download = {
					.uri       = "/*",  // Match all URIs of type /path/to/file
					.method    = HTTP_GET,
					.handler   = http_download_get_handler,
					.user_ctx  = server_data    // Pass server data as context
				};
				httpd_register_uri_handler(http_handle, &file_download);

				/* URI handler for uploading files to server */
				httpd_uri_t file_submit = {
					.uri       = "/*",   // Match all URIs of type /upload/path/to/file
					.method    = HTTP_POST,
					.handler   = http_submit_post_handler,
					.user_ctx  = server_data    // Pass server data as context
				};
				httpd_register_uri_handler(http_handle, &file_submit);

				LOG_COMM(HTTP_API_TAG, "HTTP server started on port: '%d'", config.server_port);
			}
		}
    }

    return err;
}

esp_err_t http_server_stop(void)
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

esp_err_t http_server_set_str_config(char* str_config)
{
    esp_err_t err = ESP_OK;

    if(str_config == NULL)
    {
        err = ESP_ERR_INVALID_ARG;
    }
    else
    {
        if(http_config != NULL)
        {
            free(http_config);
        }

        http_config = strdup(str_config);

        if(http_config == NULL)
        {
            err = ESP_FAIL;
        }
    }

    return err;
}


/* Private methods ----------------------------------------------- */
/**
 * @brief	Handler to redirect incoming GET request for /index.html to /
 * This can be overridden by uploading file with same name.
 * @param	req[in/out] - HTTP Request Data Structure.
 * @return
 *  - ESP_OK : get OK.
 */
static esp_err_t http_index_html_get_handler(httpd_req_t *req)
{
    httpd_resp_set_status(req, "307 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);  // Response body can be empty
    return ESP_OK;
}

/**
 * @brief	Handler to respond with an icon file embedded in flash.
 * Browsers expect to GET website css at URI /index.css.
 * This can be overridden by uploading file with same name
 * @param	req[in/out] - HTTP Request Data Structure.
 * @return
 *  - ESP_OK : get OK.
 */
static esp_err_t http_index_css_get_handler(httpd_req_t *req)
{
    extern const unsigned char favicon_css_start[] asm("_binary_index_css_start");
    extern const unsigned char favicon_css_end[]   asm("_binary_index_css_end");
    const size_t favicon_ico_size = (favicon_css_end - favicon_css_start);
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)favicon_css_start, favicon_ico_size);
    return ESP_OK;
}


/**
 * @brief	Handler to respond with an icon file embedded in flash.
 * Browsers expect to GET website icon at URI /favicon.ico.
 * This can be overridden by uploading file with same name
 * @param	req[in/out] - HTTP Request Data Structure.
 * @return
 *  - ESP_OK : get OK.
 */
static esp_err_t http_favicon_get_handler(httpd_req_t *req)
{
    extern const unsigned char favicon_ico_start[] asm("_binary_favicon_ico_start");
    extern const unsigned char favicon_ico_end[]   asm("_binary_favicon_ico_end");
    const size_t favicon_ico_size = (favicon_ico_end - favicon_ico_start);
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_size);
    return ESP_OK;
}

/**
 * @brief	Handler to respond with an logo file embedded in flash.
 * Browsers expect to GET website logo at URI /logo.png
 * This can be overridden by uploading file with same name
 * @param	req[in/out] - HTTP Request Data Structure.
 * @return
 *  - ESP_OK : get OK.
 */
static esp_err_t http_logo_get_handler(httpd_req_t *req)
{
    extern const unsigned char logo_png_start[] asm("_binary_logo_png_start");
    extern const unsigned char logo_png_end[]   asm("_binary_logo_png_end");
    const size_t logo_png_size = (logo_png_end - logo_png_start);
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)logo_png_start, logo_png_size);
    return ESP_OK;
}

/**
 * @brief	Send HTTP response with a run-time generated html consisting of
 * a list of all files and folders under the requested path.
 * In case of SPIFFS this returns empty list when path is any
 * string other than '/', since SPIFFS doesn't support directories
 * @param
 *  - req[in/out] - HTTP Request Data Structure.
 *  - dirpath [in] - html directory
 * @return
 *  - ESP_OK : Resp OK.
 *  - ESP_FAIL : Failed to stat dir
 */
static esp_err_t http_resp_dir_html(httpd_req_t *req, const char *dirpath)
{
    char entrypath[HTTP_FILE_PATH_MAX];

    /* Get handle to embedded file upload script */
	extern const unsigned char index_start[] asm("_binary_index_html_start");
	extern const unsigned char index_end[]   asm("_binary_index_html_end");
	const size_t upload_script_size = (index_end - index_start);

    DIR *dir = opendir(dirpath);

    /* Retrieve the base path of file storage to construct the full path */
    strlcpy(entrypath, dirpath, sizeof(entrypath));

    if (!dir)
    {
        ESP_LOGE(HTTP_API_TAG, "Failed to stat dir : %s", dirpath);
        /* Respond with 404 Not Found */
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Directory does not exist");
        return ESP_FAIL;
    }

    /* Add file upload form and script which on execution sends a POST request to /upload */
    httpd_resp_send_chunk(req, (const char *)index_start, upload_script_size);

    closedir(dir);

    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

/**
 * @brief	Handler to download a file kept on the server
 * @param
 *  - req[in/out] - HTTP Request Data Structure.
 * @return
 *  - ESP_OK : On success
 *  - ESP_FAIL : fail to get handler
 */
static esp_err_t http_download_get_handler(httpd_req_t *req)
{
	esp_err_t err = ESP_OK;
	struct stat file_stat = {};
    char filepath[HTTP_FILE_PATH_MAX] = {};

    if(strcmp(req->uri, "/get_config") == 0)
    {
		httpd_resp_send(req, http_config, HTTPD_RESP_USE_STRLEN);
    }
    else
    {
		const char *filename = http_get_path_from_uri(filepath, ((http_file_server_data *)req->user_ctx)->base_path,
												 req->uri, sizeof(filepath));
		if (!filename)
		{
			ESP_LOGE(HTTP_API_TAG, "Filename is too long");
			/* Respond with 500 Internal Server Error */
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
			return ESP_FAIL;
		}

		/* If name has trailing '/', respond with directory contents */
		if (filename[strlen(filename) - 1] == '/')
		{
			err = http_resp_dir_html(req, filepath);
		}
		else
		{
			if (stat(filepath, &file_stat) == -1)
			{
				/* If file not present on SPIFFS check if URI
				* corresponds to one of the hardcoded paths */
				if (strcmp(filename, "/index.html") == 0)
				{
					err = http_index_html_get_handler(req);
				}
				if (strcmp(filename, "/index.css") == 0)
				{
					err = http_index_css_get_handler(req);
				}
				else if (strcmp(filename, "/favicon.ico") == 0)
				{
					err = http_favicon_get_handler(req);
				}
				else if (strcmp(filename, "/logo.png") == 0)
				{
					err = http_logo_get_handler(req);
				}
				else
				{
					ESP_LOGE(HTTP_API_TAG, "Failed to stat file : %s (%s)", filepath, filename);
					/* Respond with 404 Not Found */
					httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File does not exist");
					err = ESP_FAIL;
				}
			}
		}
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
static esp_err_t http_submit_post_handler(httpd_req_t *req)
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
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }
    }
    else
    {
#ifdef HTTP_DEBUG
		ESP_LOGI(HTTP_API_TAG, "content_len %d", req->content_len);
		ESP_LOGI(HTTP_API_TAG, "URI %s", req->uri);
		ESP_LOGI(HTTP_API_TAG, "content %s", content);
#endif

		/* Send a simple response */
		httpd_resp_send(req, content, HTTPD_RESP_USE_STRLEN);

		if(http_callback != NULL)
		{
			http_callback(CALL_NEW_CONFIG_HTTP, content);
			err = ESP_OK;
		}
		else
		{
			ESP_LOGE(HTTP_API_TAG,"unregistered HTTP callback");
		}
    }

	return err;
}

