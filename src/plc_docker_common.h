/*------------------------------------------------------------------------------
 *
 * Copyright (c) 2017-Present Pivotal Software, Inc
 *
 *------------------------------------------------------------------------------
 */

#ifndef PLC_DOCKER_COMMON_H
#define PLC_DOCKER_COMMON_H

#include "postgres.h"

#include "plc_backend_api.h"

#ifdef CURL_DOCKER_API
    #include "plc_docker_curl_api.h"
#else
    #include "plc_docker_api.h"
#endif

void plc_docker_init(PLC_FunctionEntries entries);

#endif /* PLC_DOCKER_COMMON_H */
