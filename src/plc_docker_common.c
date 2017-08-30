/*------------------------------------------------------------------------------
 *
 * Copyright (c) 2017-Present Pivotal Software, Inc
 *
 *------------------------------------------------------------------------------
 */

#include "plc_docker_common.h"

#ifdef CURL_DOCKER_API
    #include "plc_docker_curl_api.h"
#else
    #include "plc_docker_api.h"
#endif

void plc_docker_init(PLC_FunctionEntries entries){
    entries->connect        = plc_docker_connect;
    entries->create         = plc_docker_create_container;
    entries->start          = plc_docker_start_container;
    entries->kill           = plc_docker_kill_container;
    entries->inspect        = plc_docker_inspect_container;
    entries->wait           = plc_docker_wait_container;
    entries->delete_backend = plc_docker_delete_container;
    entries->disconnect     = plc_docker_disconnect;
}
