/*------------------------------------------------------------------------------
 *
 * Copyright (c) 2016-Present Pivotal Software, Inc
 *
 *------------------------------------------------------------------------------
 */

#ifndef PLC_BACKEND_API_H
#define PLC_BACKEND_API_H

#include "postgres.h"

#include "plc_configuration.h"

#define FUNC_RETURN_OK 0

typedef int ( * PLC_FPTR_connect)    (void);
typedef int ( * PLC_FPTR_create)     (int sockfd, plcContainerConf *conf, char **name);
typedef int ( * PLC_FPTR_start)      (int sockfd, char *name);
typedef int ( * PLC_FPTR_kill)       (int sockfd, char *name);
typedef int ( * PLC_FPTR_inspect)    (int sockfd, char *name, int *port);
typedef int ( * PLC_FPTR_wait)       (int sockfd, char *name);
typedef int ( * PLC_FPTR_delete)     (int sockfd, char *name);
typedef int ( * PLC_FPTR_disconnect) (int sockfd);

struct PLC_FunctionEntriesData{
    PLC_FPTR_connect     connect;
    PLC_FPTR_create      create;
    PLC_FPTR_start       start;
    PLC_FPTR_kill        kill;
    PLC_FPTR_inspect     inspect;
    PLC_FPTR_wait        wait;
    PLC_FPTR_delete      delete_backend;
    PLC_FPTR_disconnect  disconnect;
};

typedef struct PLC_FunctionEntriesData  PLC_FunctionEntriesData;
typedef struct PLC_FunctionEntriesData *PLC_FunctionEntries;

enum PLC_BACKEND_TYPE {
    DOCKER_CONTAINER = 0,
    UNIMPLEMENT_TYPE
};

void plc_prepareImplementation(enum PLC_BACKEND_TYPE imptype);

/* interface for plc backend*/
int plc_connect(void);
int plc_create(int sockfd, plcContainerConf *conf, char **name);
int plc_start(int sockfd, char *name);
int plc_kill(int sockfd, char *name);
int plc_inspect(int sockfd, char *name, int *port);
int plc_wait(int sockfd, char *name);
int plc_delete(int sockfd, char *name);
int plc_disconnect(int sockfd);

#endif /* PLC_BACKEND_API_H */