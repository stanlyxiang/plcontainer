/*------------------------------------------------------------------------------
 *
 *
 * Copyright (c) 2016, Pivotal.
 *
 *------------------------------------------------------------------------------
 */


#ifdef CURL_DOCKER_API

#include "postgres.h"
#include "regex/regex.h"

#include "plc_garden_curl_api.h"
#include "plc_configuration.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

// Default location of the Docker API unix socket
static char *plc_garden_socket = "/var/run/garden.sock";

// URL prefix specifies Docker API version
static char *plc_garden_url = "10.152.10.138:8088";

/* Static functions of the Docker API module */
static plcCurlBuffer *plcCurlBufferInit();
static void plcCurlBufferFree(plcCurlBuffer *buf);
static size_t plcCurlCallback(void *contents, size_t size, size_t nmemb, void *userp);
static plcCurlBuffer *plcCurlRESTAPICallGarden(plcCurlCallType cType,
                                         char *body,
                                         long expectedReturn,
                                         bool silent);
static int docker_parse_container_id(char *response, char **name);
static int docker_parse_port_mapping(char *response, int *port);

/* Initialize Curl response receiving buffer */
static plcCurlBuffer *plcCurlBufferInit() {
    plcCurlBuffer *buf = palloc(sizeof(plcCurlBuffer));
    buf->data = palloc(8192);   /* will be grown as needed by the realloc above */ 
    memset(buf->data, 0, 8192); /* set to zeros to avoid errors */
    buf->bufsize = 8192;        /* initial size of the buffer */
    buf->size = 0;              /* amount of data in this buffer */ 
    buf->status = 0;            /* status of the Curl call */
    return buf;
}

/* Free Curl response receiving buffer */
static void plcCurlBufferFree(plcCurlBuffer *buf) {
    pfree(buf->data);
    pfree(buf);
}

/* Curl callback for receiving a chunk of data into buffer */
static size_t plcCurlCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    plcCurlBuffer *mem = (plcCurlBuffer*)userp;

    if (mem->size + realsize + 1 > mem->bufsize) {
        mem->data = repalloc(mem->data, 3 * (mem->size + realsize + 1) / 2);
        mem->bufsize = 3 * (mem->size + realsize + 1) / 2;
        if (mem->data == NULL) {
            /* out of memory! */ 
            elog(ERROR, "not enough memory (realloc returned NULL)");
            return 0;
        }
    }

    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

/* Function for calling Docker REST API using Curl */
static plcCurlBuffer *plcCurlRESTAPICallGarden(plcCurlCallType cType,
                                         char *body,
                                         long expectedReturn,
                                         bool silent) {
    CURL *curl;
    CURLcode res;
    plcCurlBuffer *buffer = plcCurlBufferInit();
    char errbuf[CURL_ERROR_SIZE];

    memset(errbuf, 0, CURL_ERROR_SIZE);

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    if (curl) {
        struct curl_slist *headers = NULL; // init to NULL is important

        char *msg = NULL;
        msg = palloc(strlen(body)+strlen(plc_garden_url)+10);
        sprintf(msg, "%s/?%s", plc_garden_url, body);
        /* Setting up request URL */
        curl_easy_setopt(curl, CURLOPT_URL, msg);

        /* Providing a buffer to store errors in */
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);

        /* Choosing the right request type */
        switch (cType) {
            case PLC_CALL_HTTPGET:
                curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
                break;
            case PLC_CALL_POST:
                curl_easy_setopt(curl, CURLOPT_POST, 1);
                headers = curl_slist_append(headers, "Content-Type: text/plain");
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
                break;
            case PLC_CALL_DELETE:
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE"); 
                break;
            default:
                elog(ERROR, "Unsupported call type for PL/Container Docker Curl API");
                buffer->status = -1;
                break;
        }

        /* Setting up response receive callback */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, plcCurlCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)buffer);

        elog(WARNING,"hack%s",msg );
        /* Calling the API */
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            size_t len = strlen(errbuf);
            if (!silent) {
                elog(ERROR, "PL/Container libcurl return code %d, error '%s'", res,
                    (len > 0) ? errbuf : curl_easy_strerror(res));
            }
            buffer->status = -2;
        } else {
            long http_code = 0;

            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (http_code == expectedReturn) {
                if (!silent) {
                    elog(DEBUG1, "Call '%s' succeeded\n", msg);
                    elog(DEBUG1, "Returned data: %s\n", buffer->data);
                }
                buffer->status = 0;
            } else {
                if (!silent) {
                    elog(ERROR, "Curl call to '%s' returned error code %ld, error '%s'\n",
                    		msg, http_code, buffer->data);
                }
                buffer->status = -3;
            }
        }
    }

    /* cleanup curl stuff */ 
    curl_easy_cleanup(curl);

    return buffer;
}

/* Parse container ID out of JSON response */
static int docker_parse_container_id(char* response, char **name) {

    return 0;
}

/* Parse host port out of JSON response */
static int docker_parse_port_mapping(char* response, int *port) {

    return 0;
}

/* Not used in Curl API */
int plc_garden_connect() {
    return 8080;
}

int plc_garden_start_container(int sockfd UNUSED, plcContainer *cont, char **name) {
    char *opt ="query=start";
    char *messageBody = NULL;
    plcCurlBuffer *response = NULL;
    int res = 0;

    messageBody = palloc(40 + strlen(opt));
    sprintf(messageBody, opt);

    /* Make a call */
    response = plcCurlRESTAPICallGarden(PLC_CALL_HTTPGET, messageBody, 200, false);
    res = response->status;

    /* Free up intermediate data */
    pfree(messageBody);

    //get container name here.
    if (res == 0) {
    		int len = strlen(response->data);
    		*name = palloc(len + 1);
    		memcpy(*name, response->data, len);
    		(*name)[len] = '\0';
    }

    plcCurlBufferFree(response);

    return res;
}

int plc_garden_stop_container(int sockfd UNUSED, char *name) {
    plcCurlBuffer *response = NULL;
    char *opt = "query=stop&name=";
    char *messageBody = NULL;
    int res = 0;

    messageBody = palloc(strlen(opt) + strlen(name) + 2);
    sprintf(messageBody, "%s%s", opt, name);

    response = plcCurlRESTAPICallGarden(PLC_CALL_HTTPGET, messageBody, 200, false);
    res = response->status;

    pfree(messageBody);
    plcCurlBufferFree(response);

    return res;
}

int plc_garden_run_container(int sockfd UNUSED, char *name, int *port) {
    plcCurlBuffer *response = NULL;
    char *opt = "query=run&name=";
    char *messageBody = NULL;
    int res = 0;

    messageBody = palloc(strlen(opt) + strlen(name) + 2);
    sprintf(messageBody,"%s%s", opt, name);

    response = plcCurlRESTAPICallGarden(PLC_CALL_HTTPGET, messageBody, 200, false);
    res = response->status;

    //get container port here.
	if (res == 0) {
		elog(WARNING, "hackday: %s\n", response->data);
		*port = *(int*)response->data;
	}

    pfree(messageBody);
    plcCurlBufferFree(response);

    return res;
}

int plc_garden_list_container(int sockfd UNUSED, char *name) {
    int res = 0;

    return res;
}

/* Not used in Curl API */
int plc_garden_disconnect(int sockfd UNUSED) {
    return 0;
}

#endif
