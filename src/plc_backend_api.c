/*-----------------------------------------------------------------------------
 *
 * Copyright (c) 2016-Present Pivotal Software, Inc
 *
 *------------------------------------------------------------------------------
 */

#include "plc_backend_api.h"

#include "plc_docker_common.h"

static PLC_FunctionEntriesData CurrentPLCImp;

void plc_backend_prepareImplementation(enum PLC_BACKEND_TYPE imptype) {
    /* Initialize plc backend implement handlers. */
    CurrentPLCImp.connect = NULL;
    CurrentPLCImp.create = NULL;
    CurrentPLCImp.start = NULL;
    CurrentPLCImp.kill = NULL;
    CurrentPLCImp.inspect = NULL;
    CurrentPLCImp.wait = NULL;
    CurrentPLCImp.delete_backend = NULL;
    CurrentPLCImp.disconnect = NULL;

    switch (imptype) {
        case BACKEND_DOCKER:
            plc_docker_init(&CurrentPLCImp);
            break;
        default:
            elog(ERROR, "Unsupported plc backend type");
    }
}

int plc_backend_connect(void){
    return CurrentPLCImp.connect != NULL ? CurrentPLCImp.connect() : FUNC_RETURN_OK;
}

int plc_backend_create(int sockfd, plcContainerConf *conf, char **name, int container_slot){
    return CurrentPLCImp.create != NULL ? CurrentPLCImp.create(sockfd, conf, name, container_slot) : FUNC_RETURN_OK;
}

int plc_backend_start(int sockfd, char *name){
    return CurrentPLCImp.start != NULL ? CurrentPLCImp.start(sockfd, name) : FUNC_RETURN_OK;
}

int plc_backend_kill(int sockfd, char *name){
    return CurrentPLCImp.kill != NULL ? CurrentPLCImp.kill(sockfd, name) : FUNC_RETURN_OK;
}

int plc_backend_inspect(int sockfd, char *name, int *port){
    return CurrentPLCImp.inspect != NULL ? CurrentPLCImp.inspect(sockfd, name, port) : FUNC_RETURN_OK;
}

int plc_backend_wait(int sockfd, char *name){
    return CurrentPLCImp.wait != NULL ? CurrentPLCImp.wait(sockfd, name) : FUNC_RETURN_OK;
}

int plc_backend_delete(int sockfd, char *name){
    return CurrentPLCImp.delete_backend != NULL ? CurrentPLCImp.delete_backend(sockfd, name) : FUNC_RETURN_OK;
}

int plc_backend_disconnect(int sockfd){
    return CurrentPLCImp.disconnect != NULL ? CurrentPLCImp.disconnect(sockfd) : FUNC_RETURN_OK;
}
