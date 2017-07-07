/*------------------------------------------------------------------------------
 *
 *
 * Copyright (c) 2016, Pivotal.
 *
 *------------------------------------------------------------------------------
 */


#ifndef PLC_GARDEN_API_H
#define PLC_GARDEN_API_H

#include "plc_configuration.h"

typedef enum {
    PLC_CALL_HTTPGET = 0,
    PLC_CALL_POST,
    PLC_CALL_DELETE
} plcCurlCallType;

typedef struct {
    char   *data;
    size_t  bufsize;
    size_t  size;
    int     status;
} plcCurlBuffer;

#ifdef CURL_DOCKER_API
    int plc_garden_connect(void);
    int plc_garden_start_container(int sockfd, plcContainer *cont, char **name);
    int plc_garden_stop_container(int sockfd, char *name);
    int plc_garden_run_container(int sockfd, char *name, int *port);
    int plc_garden_list_container(int sockfd, char *name);
    int plc_garden_disconnect(int sockfd);
#endif

#endif /* PLC_GARDEN_API_H */
